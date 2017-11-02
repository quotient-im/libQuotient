/******************************************************************************
 * Copyright (C) 2015 Felix Rohrbach <kde@fxrh.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "connection.h"
#include "connectiondata.h"
#include "user.h"
#include "events/event.h"
#include "room.h"
#include "jobs/generated/login.h"
#include "jobs/generated/logout.h"
#include "jobs/sendeventjob.h"
#include "jobs/postreceiptjob.h"
#include "jobs/joinroomjob.h"
#include "jobs/roommessagesjob.h"
#include "jobs/syncjob.h"
#include "jobs/mediathumbnailjob.h"

#include <QtNetwork/QDnsLookup>
#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QStandardPaths>
#include <QtCore/QStringBuilder>
#include <QtCore/QElapsedTimer>

using namespace QMatrixClient;

class Connection::Private
{
    public:
        explicit Private(const QUrl& serverUrl)
            : q(nullptr)
            , data(new ConnectionData(serverUrl))
            , syncJob(nullptr)
        { }
        Q_DISABLE_COPY(Private)
        Private(Private&&) = delete;
        Private operator=(Private&&) = delete;
        ~Private() { delete data; }

        Connection* q;
        ConnectionData* data;
        // A complex key below is a pair of room name and whether its
        // state is Invited. The spec mandates to keep Invited room state
        // separately so we should, e.g., keep objects for Invite and
        // Leave state of the same room.
        QHash<QPair<QString, bool>, Room*> roomMap;
        QHash<QString, User*> userMap;
        QString userId;

        SyncJob* syncJob;

        bool cacheState = true;
};

Connection::Connection(const QUrl& server, QObject* parent)
    : QObject(parent)
    , d(new Private(server))
{
    d->q = this; // All d initialization should occur before this line
}

Connection::Connection()
    : Connection(QUrl("https://matrix.org"))
{
}

Connection::~Connection()
{
    qCDebug(MAIN) << "deconstructing connection object for" << d->userId;
    stopSync();
    delete d;
}

void Connection::resolveServer(const QString& domain)
{
    // Find the Matrix server for the given domain.
    QScopedPointer<QDnsLookup, QScopedPointerDeleteLater> dns { new QDnsLookup() };
    dns->setType(QDnsLookup::SRV);
    dns->setName("_matrix._tcp." + domain);

    dns->lookup();
    connect(dns.data(), &QDnsLookup::finished, [&]() {
        // Check the lookup succeeded.
        if (dns->error() != QDnsLookup::NoError ||
                dns->serviceRecords().isEmpty()) {
            emit resolveError("DNS lookup failed");
            return;
        }

        // Handle the results.
        auto record = dns->serviceRecords().front();
        d->data->setHost(record.target());
        d->data->setPort(record.port());
        emit resolved();
    });
}

void Connection::connectToServer(const QString& user, const QString& password,
                                 const QString& initialDeviceName,
                                 const QString& deviceId)
{
    auto loginJob = callApi<LoginJob>(QStringLiteral("m.login.password"), user,
            /*medium*/ "", /*address*/ "", password, /*token*/ "",
            deviceId, initialDeviceName);
    connect( loginJob, &BaseJob::success, [=] () {
        connectWithToken(loginJob->user_id(), loginJob->access_token(),
                         loginJob->device_id());
    });
    connect( loginJob, &BaseJob::failure, [=] () {
        emit loginError(loginJob->errorString());
    });
}

void Connection::connectWithToken(const QString& userId,
        const QString& accessToken, const QString& deviceId)
{
    d->userId = userId;
    d->data->setToken(accessToken.toLatin1());
    d->data->setDeviceId(deviceId);
    qCDebug(MAIN) << "Using server" << d->data->baseUrl() << "by user" << userId
                  << "from device" << deviceId;
    emit connected();
}

void Connection::logout()
{
    auto job = callApi<LogoutJob>();
    connect( job, &LogoutJob::success, [=] {
        stopSync();
        emit loggedOut();
    });
}

void Connection::sync(int timeout)
{
    if (d->syncJob)
        return;

    // Raw string: http://en.cppreference.com/w/cpp/language/string_literal
    const QString filter { R"({"room": { "timeline": { "limit": 100 } } })" };
    auto job = d->syncJob =
            callApi<SyncJob>(d->data->lastEvent(), filter, timeout);
    connect( job, &SyncJob::success, [=] () {
        onSyncSuccess(job->takeData());
        d->syncJob = nullptr;
        emit syncDone();
    });
    connect( job, &SyncJob::retryScheduled, this, &Connection::networkError);
    connect( job, &SyncJob::failure, [=] () {
        d->syncJob = nullptr;
        if (job->error() == BaseJob::ContentAccessError)
            emit loginError(job->errorString());
        else
            emit syncError(job->errorString());
    });
}

void Connection::onSyncSuccess(SyncData &&data) {
    d->data->setLastEvent(data.nextBatch());
    for( auto&& roomData: data.takeRoomData() )
    {
        if ( auto* r = provideRoom(roomData.roomId, roomData.joinState) )
            r->updateData(std::move(roomData));
    }

}

void Connection::stopSync()
{
    if (d->syncJob)
    {
        d->syncJob->abandon();
        d->syncJob = nullptr;
    }
}

void Connection::postMessage(Room* room, const QString& type, const QString& message) const
{
    callApi<SendEventJob>(room->id(), type, message);
}

PostReceiptJob* Connection::postReceipt(Room* room, RoomEvent* event) const
{
    return callApi<PostReceiptJob>(room->id(), event->id());
}

JoinRoomJob* Connection::joinRoom(const QString& roomAlias)
{
    return callApi<JoinRoomJob>(roomAlias);
}

void Connection::leaveRoom(Room* room)
{
    callApi<LeaveRoomJob>(room->id());
}

RoomMessagesJob* Connection::getMessages(Room* room, const QString& from) const
{
    return callApi<RoomMessagesJob>(room->id(), from);
}

MediaThumbnailJob* Connection::getThumbnail(const QUrl& url, QSize requestedSize) const
{
    return callApi<MediaThumbnailJob>(url, requestedSize);
}

MediaThumbnailJob* Connection::getThumbnail(const QUrl& url, int requestedWidth,
                                            int requestedHeight) const
{
    return getThumbnail(url, QSize(requestedWidth, requestedHeight));
}

ForgetRoomJob* Connection::forgetRoom(const QString& id)
{
    // To forget is hard :) First we should ensure the local user is not
    // in the room (by leaving it, if necessary); once it's done, the /forget
    // endpoint can be called; and once this is through, the local Room object
    // (if any existed) is deleted. At the same time, we still have to
    // (basically immediately) return a pointer to ForgetRoomJob. Therefore
    // a ForgetRoomJob is created in advance and can be returned in a probably
    // not-yet-started state (it will start once /leave completes).
    auto forgetJob = new ForgetRoomJob(id);
    auto joinedRoom = d->roomMap.value({id, false});
    if (joinedRoom && joinedRoom->joinState() == JoinState::Join)
    {
        auto leaveJob = joinedRoom->leaveRoom();
        connect(leaveJob, &BaseJob::success,
                this, [=] { forgetJob->start(connectionData()); });
        connect(leaveJob, &BaseJob::failure,
                this, [=] { forgetJob->abandon(); });
    }
    else
        forgetJob->start(connectionData());
    connect(forgetJob, &BaseJob::success, this, [=]
    {
        // If the room happens to be in the map (possible in both forms),
        // delete the found object(s).
        for (auto f: {false, true})
            if (auto r = d->roomMap.take({ id, f }))
            {
                emit aboutToDeleteRoom(r);
                qCDebug(MAIN) << "Room" << id
                              << "in join state" << toCString(r->joinState())
                              << "will be deleted";
                r->deleteLater();
            }
    });
    return forgetJob;
}

QUrl Connection::homeserver() const
{
    return d->data->baseUrl();
}

User* Connection::user(const QString& userId)
{
    if( d->userMap.contains(userId) )
        return d->userMap.value(userId);
    auto* user = createUser(this, userId);
    d->userMap.insert(userId, user);
    return user;
}

User *Connection::user()
{
    if( d->userId.isEmpty() )
        return nullptr;
    return user(d->userId);
}

QString Connection::userId() const
{
    return d->userId;
}

QString Connection::deviceId() const
{
    return d->data->deviceId();
}

QString Connection::token() const
{
    return accessToken();
}

QString Connection::accessToken() const
{
    return d->data->accessToken();
}

SyncJob* Connection::syncJob() const
{
    return d->syncJob;
}

int Connection::millisToReconnect() const
{
    return d->syncJob ? d->syncJob->millisToRetry() : 0;
}

QHash< QPair<QString, bool>, Room* > Connection::roomMap() const
{
    // Copy-on-write-and-remove-elements is faster than copying elements one by one.
    QHash< QPair<QString, bool>, Room* > roomMap = d->roomMap;
    for (auto it = roomMap.begin(); it != roomMap.end(); )
    {
        if (it.value()->joinState() == JoinState::Leave)
            it = roomMap.erase(it);
        else
            ++it;
    }
    return roomMap;
}

const ConnectionData* Connection::connectionData() const
{
    return d->data;
}

Room* Connection::provideRoom(const QString& id, JoinState joinState)
{
    // TODO: This whole function is a strong case for a RoomManager class.
    if (id.isEmpty())
    {
        qCDebug(MAIN) << "Connection::provideRoom() with empty id, doing nothing";
        return nullptr;
    }

    const auto roomKey = qMakePair(id, joinState == JoinState::Invite);
    auto* room = d->roomMap.value(roomKey, nullptr);
    if (room)
    {
        // Leave is a special case because in transition (5a) (see the .h file)
        // joinState == room->joinState but we still have to preempt the Invite
        // and emit a signal. For Invite and Join, there's no such problem.
        if (room->joinState() == joinState && joinState != JoinState::Leave)
            return room;
    }
    else
    {
        room = createRoom(this, id, joinState);
        if (!room)
        {
            qCCritical(MAIN) << "Failed to create a room" << id;
            return nullptr;
        }
        d->roomMap.insert(roomKey, room);
        qCDebug(MAIN) << "Created Room" << id << ", invited:" << roomKey.second;
        emit newRoom(room);
    }
    if (joinState == JoinState::Invite)
    {
        // prev is either Leave or nullptr
        auto* prev = d->roomMap.value({id, false}, nullptr);
        emit invitedRoom(room, prev);
    }
    else
    {
        room->setJoinState(joinState);
        // Preempt the Invite room (if any) with a room in Join/Leave state.
        auto* prevInvite = d->roomMap.take({id, true});
        if (joinState == JoinState::Join)
            emit joinedRoom(room, prevInvite);
        else if (joinState == JoinState::Leave)
            emit leftRoom(room, prevInvite);
        if (prevInvite)
        {
            qCDebug(MAIN) << "Deleting Invite state for room" << prevInvite->id();
            emit aboutToDeleteRoom(prevInvite);
            delete prevInvite;
        }
    }

    return room;
}

Connection::room_factory_t Connection::createRoom =
    [](Connection* c, const QString& id, JoinState joinState)
    { return new Room(c, id, joinState); };

Connection::user_factory_t Connection::createUser =
    [](Connection* c, const QString& id) { return new User(id, c); };
QByteArray Connection::generateTxnId()
{
    return d->data->generateTxnId();
}

void Connection::saveState(const QUrl &toFile) const
{
    if (!d->cacheState)
        return;

    QElapsedTimer et; et.start();

    QFileInfo stateFile {
        toFile.isEmpty() ? stateCachePath() : toFile.toLocalFile()
    };
    if (!stateFile.dir().exists())
        stateFile.dir().mkpath(".");

    QFile outfile { stateFile.absoluteFilePath() };
    if (!outfile.open(QFile::WriteOnly))
    {
        qCWarning(MAIN) << "Error opening" << stateFile.absoluteFilePath()
                        << ":" << outfile.errorString();
        qCWarning(MAIN) << "Caching the rooms state disabled";
        d->cacheState = false;
        return;
    }

    QJsonObject roomObj;
    {
        QJsonObject rooms;
        QJsonObject inviteRooms;
        for (auto i : roomMap()) // Pass on rooms in Leave state
        {
            if (i->joinState() == JoinState::Invite)
                inviteRooms.insert(i->id(), i->toJson());
            else
                rooms.insert(i->id(), i->toJson());
        }

        if (!rooms.isEmpty())
            roomObj.insert("join", rooms);
        if (!inviteRooms.isEmpty())
            roomObj.insert("invite", inviteRooms);
    }

    QJsonObject rootObj;
    rootObj.insert("next_batch", d->data->lastEvent());
    rootObj.insert("rooms", roomObj);

    QByteArray data = QJsonDocument(rootObj).toJson(QJsonDocument::Compact);

    qCDebug(MAIN) << "Writing state to file" << outfile.fileName();
    outfile.write(data.data(), data.size());
    qCDebug(PROFILER) << "*** Cached state for" << userId()
                      << "saved in" << et.elapsed() << "ms";
}

void Connection::loadState(const QUrl &fromFile)
{
    if (!d->cacheState)
        return;

    QElapsedTimer et; et.start();
    QFile file {
        fromFile.isEmpty() ? stateCachePath() : fromFile.toLocalFile()
    };
    if (!file.exists())
    {
        qCDebug(MAIN) << "No state cache file found";
        return;
    }
    file.open(QFile::ReadOnly);
    QByteArray data = file.readAll();

    SyncData sync;
    sync.parseJson(QJsonDocument::fromJson(data));
    onSyncSuccess(std::move(sync));
    qCDebug(PROFILER) << "*** Cached state for" << userId()
                      << "loaded in" << et.elapsed() << "ms";
}

QString Connection::stateCachePath() const
{
    auto safeUserId = userId();
    safeUserId.replace(':', '_');
    return QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
            % '/' % safeUserId % "_state.json";
}

bool Connection::cacheState() const
{
    return d->cacheState;
}

void Connection::setCacheState(bool newValue)
{
    if (d->cacheState != newValue)
    {
        d->cacheState = newValue;
        emit cacheStateChanged();
    }
}
