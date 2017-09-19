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
#include "jobs/passwordlogin.h"
#include "jobs/logoutjob.h"
#include "jobs/sendeventjob.h"
#include "jobs/postreceiptjob.h"
#include "jobs/joinroomjob.h"
#include "jobs/leaveroomjob.h"
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
        QHash<QString, Room*> roomMap;
        QHash<QString, User*> userMap;
        QString username;
        QString password;
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

void Connection::connectToServer(const QString& user, const QString& password)
{
    auto loginJob = callApi<PasswordLogin>(user, password);
    connect( loginJob, &PasswordLogin::success, [=] () {
        connectWithToken(loginJob->id(), loginJob->token());
    });
    connect( loginJob, &PasswordLogin::failure, [=] () {
        emit loginError(loginJob->errorString());
    });
    d->username = user; // to be able to reconnect
    d->password = password;
}

void Connection::connectWithToken(const QString& userId, const QString& token)
{
    d->userId = userId;
    d->data->setToken(token);
    qCDebug(MAIN) << "Accessing" << d->data->baseUrl()
             << "by user" << userId
             << "with the following access token:";
    qCDebug(MAIN) << token;
    emit connected();
}

void Connection::reconnect()
{
    auto loginJob = callApi<PasswordLogin>(d->username, d->password);
    connect( loginJob, &PasswordLogin::success, [=] () {
        d->userId = loginJob->id();
        emit reconnected();
    });
    connect( loginJob, &PasswordLogin::failure, [=] () {
        emit loginError(loginJob->errorString());
    });
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
        if ( auto* r = provideRoom(roomData.roomId) )
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
    auto job = callApi<JoinRoomJob>(roomAlias);
    connect( job, &BaseJob::success, [=] () {
        if ( Room* r = provideRoom(job->roomId()) )
            emit joinedRoom(r);
    });
    return job;
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

QUrl Connection::homeserver() const
{
    return d->data->baseUrl();
}

User* Connection::user(const QString& userId)
{
    if( d->userMap.contains(userId) )
        return d->userMap.value(userId);
    User* user = createUser(userId);
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

QHash< QString, Room* > Connection::roomMap() const
{
    return d->roomMap;
}

const ConnectionData* Connection::connectionData() const
{
    return d->data;
}

Room* Connection::provideRoom(const QString& id)
{
    if (id.isEmpty())
    {
        qCDebug(MAIN) << "Connection::provideRoom() with empty id, doing nothing";
        return nullptr;
    }

    if (d->roomMap.contains(id))
        return d->roomMap.value(id);

    // Not yet in the map, create a new one.
    Room* room = createRoom(id);
    if (room)
    {
        d->roomMap.insert( id, room );
        emit newRoom(room);
    } else {
        qCritical() << "Failed to create a room!!!" << id;
    }

    return room;
}

User* Connection::createUser(const QString& userId)
{
    return new User(userId, this);
}

Room* Connection::createRoom(const QString& roomId)
{
    return new Room(this, roomId);
}

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
