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
#include "events/directchatevent.h"
#include "room.h"
#include "settings.h"
#include "jobs/generated/login.h"
#include "jobs/generated/logout.h"
#include "jobs/generated/receipts.h"
#include "jobs/generated/leaving.h"
#include "jobs/generated/account-data.h"
#include "jobs/sendeventjob.h"
#include "jobs/joinroomjob.h"
#include "jobs/roommessagesjob.h"
#include "jobs/syncjob.h"
#include "jobs/mediathumbnailjob.h"
#include "jobs/downloadfilejob.h"

#include <QtNetwork/QDnsLookup>
#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QStandardPaths>
#include <QtCore/QStringBuilder>
#include <QtCore/QElapsedTimer>
#include <QtCore/QRegularExpression>
#include <QtCore/QCoreApplication>

using namespace QMatrixClient;

// This is very much Qt-specific; STL iterators don't have key() and value()
template <typename HashT, typename Pred>
HashT erase_if(HashT& hashMap, Pred pred)
{
    HashT removals;
    for (auto it = hashMap.begin(); it != hashMap.end();)
    {
        if (pred(it))
        {
            removals.insert(it.key(), it.value());
            it = hashMap.erase(it);
        } else
            ++it;
    }
    return removals;
}

class Connection::Private
{
    public:
        explicit Private(std::unique_ptr<ConnectionData>&& connection)
            : data(move(connection))
        { }
        Q_DISABLE_COPY(Private)
        Private(Private&&) = delete;
        Private operator=(Private&&) = delete;

        Connection* q = nullptr;
        std::unique_ptr<ConnectionData> data;
        // A complex key below is a pair of room name and whether its
        // state is Invited. The spec mandates to keep Invited room state
        // separately so we should, e.g., keep objects for Invite and
        // Leave state of the same room.
        QHash<QPair<QString, bool>, Room*> roomMap;
        QVector<QString> roomIdsToForget;
        QMap<QString, User*> userMap;
        DirectChatsMap directChats;
        QHash<QString, AccountDataMap> accountData;
        QString userId;

        SyncJob* syncJob = nullptr;

        bool cacheState = true;
        bool cacheToBinary = SettingsGroup("libqmatrixclient")
                             .value("cache_type").toString() != "json";

        void connectWithToken(const QString& user, const QString& accessToken,
                              const QString& deviceId);
        void broadcastDirectChatUpdates(const DirectChatsMap& additions,
                                        const DirectChatsMap& removals);
};

Connection::Connection(const QUrl& server, QObject* parent)
    : QObject(parent)
    , d(std::make_unique<Private>(std::make_unique<ConnectionData>(server)))
{
    d->q = this; // All d initialization should occur before this line
}

Connection::Connection(QObject* parent)
    : Connection({}, parent)
{ }

Connection::~Connection()
{
    qCDebug(MAIN) << "deconstructing connection object for" << d->userId;
    stopSync();
}

void Connection::resolveServer(const QString& mxidOrDomain)
{
    // At this point we may have something as complex as
    // @username:[IPv6:address]:port, or as simple as a plain domain name.

    // Try to parse as an FQID; if there's no @ part, assume it's a domain name.
    QRegularExpression parser(
        "^(@.+?:)?" // Optional username (allow everything for compatibility)
        "(\\[[^]]+\\]|[^:@]+)" // Either IPv6 address or hostname/IPv4 address
        "(:\\d{1,5})?$", // Optional port
        QRegularExpression::UseUnicodePropertiesOption); // Because asian digits
    auto match = parser.match(mxidOrDomain);

    QUrl maybeBaseUrl = QUrl::fromUserInput(match.captured(2));
    maybeBaseUrl.setScheme("https"); // Instead of the Qt-default "http"
    if (!match.hasMatch() || !maybeBaseUrl.isValid())
    {
        emit resolveError(
            tr("%1 is not a valid homeserver address")
                    .arg(maybeBaseUrl.toString()));
        return;
    }

    setHomeserver(maybeBaseUrl);
    emit resolved();
    return;

    // FIXME, #178: The below code is incorrect and is no more executed. The
    // correct server resolution should be done from .well-known/matrix/client
    auto domain = maybeBaseUrl.host();
    qCDebug(MAIN) << "Finding the server" << domain;
    // Check if the Matrix server has a dedicated service record.
    auto* dns = new QDnsLookup();
    dns->setType(QDnsLookup::SRV);
    dns->setName("_matrix._tcp." + domain);

    connect(dns, &QDnsLookup::finished, [this,dns,maybeBaseUrl]() {
        QUrl baseUrl { maybeBaseUrl };
        if (dns->error() == QDnsLookup::NoError &&
                dns->serviceRecords().isEmpty())
        {
            auto record = dns->serviceRecords().front();
            baseUrl.setHost(record.target());
            baseUrl.setPort(record.port());
            qCDebug(MAIN) << "SRV record for" << maybeBaseUrl.host()
                          << "is" << baseUrl.authority();
        } else {
            qCDebug(MAIN) << baseUrl.host() << "doesn't have SRV record"
                          << dns->name() << "- using the hostname as is";
        }
        setHomeserver(baseUrl);
        emit resolved();
        dns->deleteLater();
    });
    dns->lookup();
}

void Connection::connectToServer(const QString& user, const QString& password,
                                 const QString& initialDeviceName,
                                 const QString& deviceId)
{
    checkAndConnect(user,
        [=] {
            doConnectToServer(user, password, initialDeviceName, deviceId);
        });
}
void Connection::doConnectToServer(const QString& user, const QString& password,
                                   const QString& initialDeviceName,
                                   const QString& deviceId)
{
    auto loginJob = callApi<LoginJob>(QStringLiteral("m.login.password"),
            user, /*medium*/ "", /*address*/ "", password, /*token*/ "",
            deviceId, initialDeviceName);
    connect(loginJob, &BaseJob::success, this,
        [this, loginJob] {
            d->connectWithToken(loginJob->userId(), loginJob->accessToken(),
                                loginJob->deviceId());
        });
    connect(loginJob, &BaseJob::failure, this,
        [this, loginJob] {
            emit loginError(loginJob->errorString());
        });
}

void Connection::connectWithToken(const QString& userId,
                                  const QString& accessToken,
                                  const QString& deviceId)
{
    checkAndConnect(userId,
        [=] { d->connectWithToken(userId, accessToken, deviceId); });
}

void Connection::Private::connectWithToken(const QString& user,
                                           const QString& accessToken,
                                           const QString& deviceId)
{
    userId = user;
    data->setToken(accessToken.toLatin1());
    data->setDeviceId(deviceId);
    qCDebug(MAIN) << "Using server" << data->baseUrl().toDisplayString()
                  << "by user" << userId << "from device" << deviceId;
    emit q->connected();

}

void Connection::checkAndConnect(const QString& userId,
                                 std::function<void()> connectFn)
{
    if (d->data->baseUrl().isValid())
    {
        connectFn();
        return;
    }
    // Not good to go, try to fix the homeserver URL.
    if (userId.startsWith('@') && userId.indexOf(':') != -1)
    {
        // The below construct makes a single-shot connection that triggers
        // on the signal and then self-disconnects.
        // NB: doResolveServer can emit resolveError, so this is a part of
        // checkAndConnect function contract.
        QMetaObject::Connection connection;
        connection = connect(this, &Connection::homeserverChanged,
                        this, [=] { connectFn(); disconnect(connection); });
        resolveServer(userId);
    } else
        emit resolveError(
            tr("%1 is an invalid homeserver URL")
                .arg(d->data->baseUrl().toString()));
}

void Connection::logout()
{
    auto job = callApi<LogoutJob>();
    connect( job, &LogoutJob::success, this, [this] {
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
    connect( job, &SyncJob::success, this, [this, job] {
        onSyncSuccess(job->takeData());
        d->syncJob = nullptr;
        emit syncDone();
    });
    connect( job, &SyncJob::retryScheduled, this, &Connection::networkError);
    connect( job, &SyncJob::failure, this, [this, job] {
        d->syncJob = nullptr;
        if (job->error() == BaseJob::ContentAccessError)
            emit loginError(job->errorString());
        else
            emit syncError(job->errorString());
    });
}

void Connection::onSyncSuccess(SyncData &&data) {
    d->data->setLastEvent(data.nextBatch());
    for (auto&& roomData: data.takeRoomData())
    {
        const auto forgetIdx = d->roomIdsToForget.indexOf(roomData.roomId);
        if (forgetIdx != -1)
        {
            d->roomIdsToForget.removeAt(forgetIdx);
            if (roomData.joinState == JoinState::Leave)
            {
                qDebug(MAIN) << "Room" << roomData.roomId
                    << "has been forgotten, ignoring /sync response for it";
                continue;
            }
            qWarning(MAIN) << "Room" << roomData.roomId
                 << "has just been forgotten but /sync returned it in"
                 << toCString(roomData.joinState)
                 << "state - suspiciously fast turnaround";
        }
        if ( auto* r = provideRoom(roomData.roomId, roomData.joinState) )
            r->updateData(std::move(roomData));
        QCoreApplication::processEvents();
    }
    for (auto&& accountEvent: data.takeAccountData())
    {
        if (accountEvent->type() == EventType::DirectChat)
        {
            const auto usersToDCs = ptrCast<DirectChatEvent>(move(accountEvent))
                                        ->usersToDirectChats();
            DirectChatsMap removals =
                erase_if(d->directChats, [&usersToDCs] (auto it) {
                    return !usersToDCs.contains(it.key()->id(), it.value());
                });
            if (MAIN().isDebugEnabled())
                for (auto it = removals.begin(); it != removals.end(); ++it)
                    qCDebug(MAIN) << it.value()
                        << "is no more a direct chat with" << it.key()->id();

            DirectChatsMap additions;
            for (auto it = usersToDCs.begin(); it != usersToDCs.end(); ++it)
            {
                const auto* u = user(it.key());
                if (!d->directChats.contains(u, it.value()))
                {
                    additions.insert(u, it.value());
                    d->directChats.insert(u, it.value());
                    qCDebug(MAIN) << "Marked room" << it.value()
                                  << "as a direct chat with" << u->id();
                }
            }
            if (!additions.isEmpty() || !removals.isEmpty())
                emit directChatsListChanged(additions, removals);

            continue;
        }
        d->accountData[accountEvent->jsonType()] =
                fromJson<AccountDataMap>(accountEvent->contentJson());
        emit accountDataChanged(accountEvent->jsonType());
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
    return callApi<PostReceiptJob>(room->id(), "m.read", event->id());
}

JoinRoomJob* Connection::joinRoom(const QString& roomAlias)
{
    auto job = callApi<JoinRoomJob>(roomAlias);
    connect(job, &JoinRoomJob::success,
            this, [this, job] { provideRoom(job->roomId(), JoinState::Join); });
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

inline auto splitMediaId(const QString& mediaId)
{
    auto idParts = mediaId.split('/');
    Q_ASSERT_X(idParts.size() == 2, __FUNCTION__,
               ("'" + mediaId +
                "' doesn't look like 'serverName/localMediaId'").toLatin1());
    return idParts;
}

MediaThumbnailJob* Connection::getThumbnail(const QString& mediaId, QSize requestedSize) const
{
    auto idParts = splitMediaId(mediaId);
    return callApi<MediaThumbnailJob>(idParts.front(), idParts.back(),
                                      requestedSize);
}

MediaThumbnailJob* Connection::getThumbnail(const QUrl& url, QSize requestedSize) const
{
    return getThumbnail(url.authority() + url.path(), requestedSize);
}

MediaThumbnailJob* Connection::getThumbnail(const QUrl& url, int requestedWidth,
                                            int requestedHeight) const
{
    return getThumbnail(url, QSize(requestedWidth, requestedHeight));
}

UploadContentJob* Connection::uploadContent(QIODevice* contentSource,
        const QString& filename, const QString& contentType) const
{
    return callApi<UploadContentJob>(contentSource, filename, contentType);
}

UploadContentJob* Connection::uploadFile(const QString& fileName,
                                         const QString& contentType)
{
    auto sourceFile = new QFile(fileName);
    if (sourceFile->open(QIODevice::ReadOnly))
    {
        qCWarning(MAIN) << "Couldn't open" << sourceFile->fileName()
                        << "for reading";
        return nullptr;
    }
    return uploadContent(sourceFile, QFileInfo(*sourceFile).fileName(),
                         contentType);
}

GetContentJob* Connection::getContent(const QString& mediaId) const
{
    auto idParts = splitMediaId(mediaId);
    return callApi<GetContentJob>(idParts.front(), idParts.back());
}

GetContentJob* Connection::getContent(const QUrl& url) const
{
    return getContent(url.authority() + url.path());
}

DownloadFileJob* Connection::downloadFile(const QUrl& url,
                                          const QString& localFilename) const
{
    auto mediaId = url.authority() + url.path();
    auto idParts = splitMediaId(mediaId);
    auto* job = callApi<DownloadFileJob>(idParts.front(), idParts.back(),
                                         localFilename);
    return job;
}

CreateRoomJob* Connection::createRoom(RoomVisibility visibility,
    const QString& alias, const QString& name, const QString& topic,
    const QVector<QString>& invites, const QString& presetName,
    bool isDirect, bool guestsCanJoin,
    const QVector<CreateRoomJob::StateEvent>& initialState,
    const QVector<CreateRoomJob::Invite3pid>& invite3pids,
    const QJsonObject& creationContent)
{
    auto job = callApi<CreateRoomJob>(
            visibility == PublishRoom ? "public" : "private", alias, name,
            topic, invites, invite3pids, creationContent, initialState,
            presetName, isDirect, guestsCanJoin);
    connect(job, &BaseJob::success, this, [this,job] {
        emit createdRoom(provideRoom(job->roomId(), JoinState::Join));
    });
    return job;
}

void Connection::requestDirectChat(const QString& userId)
{
    doInDirectChat(userId, [this] (Room* r) { emit directChatAvailable(r); });
}

void Connection::doInDirectChat(const QString& userId,
                                std::function<void (Room*)> operation)
{
    // There can be more than one DC; find the first valid, and delete invalid
    // (left/forgotten) ones along the way.
    const auto* u = user(userId);
    for (auto it = d->directChats.find(u);
         it != d->directChats.end() && it.key() == u; ++it)
    {
        const auto& roomId = *it;
        if (auto r = room(roomId, JoinState::Join))
        {
            Q_ASSERT(r->id() == roomId);
            qCDebug(MAIN) << "Requested direct chat with" << userId
                          << "is already available as" << r->id();
            operation(r);
            return;
        }
        if (auto ir = invitation(roomId))
        {
            Q_ASSERT(ir->id() == roomId);
            auto j = joinRoom(ir->id());
            connect(j, &BaseJob::success, this, [this,roomId,userId,operation] {
                qCDebug(MAIN) << "Joined the already invited direct chat with"
                              << userId << "as" << roomId;
                operation(room(roomId, JoinState::Join));
            });
        }
        qCWarning(MAIN) << "Direct chat with" << userId << "known as room"
                        << roomId << "is not valid, discarding it";
        removeFromDirectChats(roomId);
    }

    auto j = createDirectChat(userId);
    connect(j, &BaseJob::success, this, [this,j,userId,operation] {
        qCDebug(MAIN) << "Direct chat with" << userId
                      << "has been created as" << j->roomId();
        operation(room(j->roomId(), JoinState::Join));
    });
}

CreateRoomJob* Connection::createDirectChat(const QString& userId,
    const QString& topic, const QString& name)
{
    return createRoom(UnpublishRoom, "", name, topic, {userId},
                      "trusted_private_chat", true);
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
    auto room = d->roomMap.value({id, false});
    if (!room)
        room = d->roomMap.value({id, true});
    if (room && room->joinState() != JoinState::Leave)
    {
        auto leaveJob = room->leaveRoom();
        connect(leaveJob, &BaseJob::success, this, [this, forgetJob, room] {
            forgetJob->start(connectionData());
            // If the matching /sync response hasn't arrived yet, mark the room
            // for explicit deletion
            if (room->joinState() != JoinState::Leave)
                d->roomIdsToForget.push_back(room->id());
        });
        connect(leaveJob, &BaseJob::failure, forgetJob, &BaseJob::abandon);
    }
    else
        forgetJob->start(connectionData());
    connect(forgetJob, &BaseJob::success, this, [this, id]
    {
        // If the room is in the map (possibly in both forms), delete all forms.
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

Room* Connection::room(const QString& roomId, JoinStates states) const
{
    Room* room = d->roomMap.value({roomId, false}, nullptr);
    if (states.testFlag(JoinState::Join) &&
            room && room->joinState() == JoinState::Join)
        return room;

    if (states.testFlag(JoinState::Invite))
        if (Room* invRoom = invitation(roomId))
            return invRoom;

    if (states.testFlag(JoinState::Leave) &&
            room && room->joinState() == JoinState::Leave)
        return room;

    return nullptr;
}

Room* Connection::invitation(const QString& roomId) const
{
    return d->roomMap.value({roomId, true}, nullptr);
}

User* Connection::user(const QString& userId)
{
    Q_ASSERT(userId.startsWith('@') && userId.contains(':'));
    if( d->userMap.contains(userId) )
        return d->userMap.value(userId);
    auto* user = userFactory(this, userId);
    d->userMap.insert(userId, user);
    emit newUser(user);
    return user;
}

const User* Connection::user() const
{
    return d->userId.isEmpty() ? nullptr : d->userMap.value(d->userId, nullptr);
}

User* Connection::user()
{
    return d->userId.isEmpty() ? nullptr : user(d->userId);
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

QByteArray Connection::accessToken() const
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

bool Connection::hasAccountData(const QString& type) const
{
    return d->accountData.contains(type);
}

Connection::AccountDataMap Connection::accountData(const QString& type) const
{
    return d->accountData.value(type);
}

QHash<QString, QVector<Room*>> Connection::tagsToRooms() const
{
    QHash<QString, QVector<Room*>> result;
    for (auto* r: qAsConst(d->roomMap))
    {
        for (const auto& tagName: r->tagNames())
            result[tagName].push_back(r);
    }
    for (auto it = result.begin(); it != result.end(); ++it)
        std::sort(it->begin(), it->end(),
            [t=it.key()] (Room* r1, Room* r2) {
                return r1->tags().value(t).order < r2->tags().value(t).order;
            });
    return result;
}

QStringList Connection::tagNames() const
{
    QStringList tags ({FavouriteTag});
    for (auto* r: qAsConst(d->roomMap))
        for (const auto& tag: r->tagNames())
            if (tag != LowPriorityTag && !tags.contains(tag))
                tags.push_back(tag);
    tags.push_back(LowPriorityTag);
    return tags;
}

QVector<Room*> Connection::roomsWithTag(const QString& tagName) const
{
    QVector<Room*> rooms;
    std::copy_if(d->roomMap.begin(), d->roomMap.end(), std::back_inserter(rooms),
                 [&tagName] (Room* r) { return r->tags().contains(tagName); });
    return rooms;
}

Connection::DirectChatsMap Connection::directChats() const
{
    return d->directChats;
}

QJsonObject toJson(const Connection::DirectChatsMap& directChats)
{
    QJsonObject json;
    for (auto it = directChats.begin(); it != directChats.end();)
    {
        QJsonArray roomIds;
        const auto* user = it.key();
        for (; it != directChats.end() && it.key() == user; ++it)
            roomIds.append(*it);
        json.insert(user->id(), roomIds);
    }
    return json;
}

void Connection::Private::broadcastDirectChatUpdates(const DirectChatsMap& additions,
                                                     const DirectChatsMap& removals)
{
    q->callApi<SetAccountDataJob>(userId, QStringLiteral("m.direct"),
                                  toJson(directChats));
    emit q->directChatsListChanged(additions, removals);
}

void Connection::addToDirectChats(const Room* room, const User* user)
{
    Q_ASSERT(room != nullptr && user != nullptr);
    if (d->directChats.contains(user, room->id()))
        return;
    d->directChats.insert(user, room->id());
    DirectChatsMap additions { { user, room->id() } };
    d->broadcastDirectChatUpdates(additions, {});
}

void Connection::removeFromDirectChats(const QString& roomId, const User* user)
{
    Q_ASSERT(!roomId.isEmpty());
    if ((user != nullptr && !d->directChats.contains(user, roomId)) ||
            d->directChats.key(roomId) == nullptr)
        return;

    DirectChatsMap removals;
    if (user != nullptr)
    {
        removals.insert(user, roomId);
        d->directChats.remove(user, roomId);
    }
    else
        removals = erase_if(d->directChats,
                            [&roomId] (auto it) { return it.value() == roomId; });
    d->broadcastDirectChatUpdates({}, removals);
}

bool Connection::isDirectChat(const QString& roomId) const
{
    return d->directChats.key(roomId) != nullptr;
}

QList<const User*> Connection::directChatUsers(const Room* room) const
{
    Q_ASSERT(room != nullptr);
    return d->directChats.keys(room->id());
}

QMap<QString, User*> Connection::users() const
{
    return d->userMap;
}

const ConnectionData* Connection::connectionData() const
{
    return d->data.get();
}

Room* Connection::provideRoom(const QString& id, JoinState joinState)
{
    // TODO: This whole function is a strong case for a RoomManager class.
    Q_ASSERT_X(!id.isEmpty(), __FUNCTION__, "Empty room id");

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
        room = roomFactory(this, id, joinState);
        if (!room)
        {
            qCCritical(MAIN) << "Failed to create a room" << id;
            return nullptr;
        }
        d->roomMap.insert(roomKey, room);
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
            prevInvite->deleteLater();
        }
    }

    return room;
}

Connection::room_factory_t Connection::roomFactory =
    [](Connection* c, const QString& id, JoinState joinState)
    { return new Room(c, id, joinState); };

Connection::user_factory_t Connection::userFactory =
    [](Connection* c, const QString& id) { return new User(id, c); };

QByteArray Connection::generateTxnId()
{
    return d->data->generateTxnId();
}

void Connection::setHomeserver(const QUrl& url)
{
    if (homeserver() == url)
        return;

    d->data->setBaseUrl(url);
    emit homeserverChanged(homeserver());
}

static constexpr int CACHE_VERSION_MAJOR = 8;
static constexpr int CACHE_VERSION_MINOR = 0;

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

    QJsonObject rootObj;
    {
        QJsonObject rooms;
        QJsonObject inviteRooms;
        for (const auto* i : roomMap()) // Pass on rooms in Leave state
        {
            if (i->joinState() == JoinState::Invite)
                inviteRooms.insert(i->id(), i->toJson());
            else
                rooms.insert(i->id(), i->toJson());
            QElapsedTimer et1; et1.start();
            QCoreApplication::processEvents();
            if (et1.elapsed() > 1)
                qCDebug(PROFILER) << "processEvents() borrowed" << et1;
        }

        QJsonObject roomObj;
        if (!rooms.isEmpty())
            roomObj.insert("join", rooms);
        if (!inviteRooms.isEmpty())
            roomObj.insert("invite", inviteRooms);

        rootObj.insert("next_batch", d->data->lastEvent());
        rootObj.insert("rooms", roomObj);
    }
    {
        QJsonArray accountDataEvents {
            QJsonObject {
                { QStringLiteral("type"), QStringLiteral("m.direct") },
                { QStringLiteral("content"), toJson(d->directChats) }
            }
        };

        for (auto it = d->accountData.begin(); it != d->accountData.end(); ++it)
            accountDataEvents.append(QJsonObject {
                {"type", it.key()},
                {"content", QMatrixClient::toJson(it.value())}
            });
        rootObj.insert("account_data",
            QJsonObject {{ QStringLiteral("events"), accountDataEvents }});
    }

    QJsonObject versionObj;
    versionObj.insert("major", CACHE_VERSION_MAJOR);
    versionObj.insert("minor", CACHE_VERSION_MINOR);
    rootObj.insert("cache_version", versionObj);

    QJsonDocument json { rootObj };
    auto data = d->cacheToBinary ? json.toBinaryData() :
                                   json.toJson(QJsonDocument::Compact);
    qCDebug(PROFILER) << "Cache for" << userId() << "generated in" << et;

    outfile.write(data.data(), data.size());
    qCDebug(MAIN) << "State cache saved to" << outfile.fileName();
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
    if(!file.open(QFile::ReadOnly))
    {
        qCWarning(MAIN) << "file " << file.fileName() << "failed to open for read";
        return;
    }
    QByteArray data = file.readAll();

    auto jsonDoc = d->cacheToBinary ? QJsonDocument::fromBinaryData(data) :
                                      QJsonDocument::fromJson(data);
    if (jsonDoc.isNull())
    {
        qCWarning(MAIN) << "Cache file broken, discarding";
        return;
    }
    auto actualCacheVersionMajor =
            jsonDoc.object()
            .value("cache_version").toObject()
            .value("major").toInt();
    if (actualCacheVersionMajor < CACHE_VERSION_MAJOR)
    {
        qCWarning(MAIN)
            << "Major version of the cache file is" << actualCacheVersionMajor
            << "but" << CACHE_VERSION_MAJOR << "required; discarding the cache";
        return;
    }

    SyncData sync;
    sync.parseJson(jsonDoc);
    onSyncSuccess(std::move(sync));
    qCDebug(PROFILER) << "*** Cached state for" << userId() << "loaded in" << et;
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

