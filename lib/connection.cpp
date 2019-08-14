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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include "connection.h"

#include "connectiondata.h"
#include "encryptionmanager.h"
#include "room.h"
#include "settings.h"
#include "user.h"

#include "csapi/account-data.h"
#include "csapi/capabilities.h"
#include "csapi/joining.h"
#include "csapi/leaving.h"
#include "csapi/login.h"
#include "csapi/logout.h"
#include "csapi/receipts.h"
#include "csapi/room_send.h"
#include "csapi/to_device.h"
#include "csapi/versions.h"
#include "csapi/voip.h"
#include "csapi/wellknown.h"

#include "events/directchatevent.h"
#include "events/eventloader.h"
#include "jobs/downloadfilejob.h"
#include "jobs/mediathumbnailjob.h"
#include "jobs/syncjob.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QElapsedTimer>
#include <QtCore/QFile>
#include <QtCore/QMimeDatabase>
#include <QtCore/QRegularExpression>
#include <QtCore/QStandardPaths>
#include <QtCore/QStringBuilder>
#include <QtNetwork/QDnsLookup>

using namespace Quotient;

// This is very much Qt-specific; STL iterators don't have key() and value()
template <typename HashT, typename Pred>
HashT erase_if(HashT& hashMap, Pred pred)
{
    HashT removals;
    for (auto it = hashMap.begin(); it != hashMap.end();) {
        if (pred(it)) {
            removals.insert(it.key(), it.value());
            it = hashMap.erase(it);
        } else
            ++it;
    }
    return removals;
}

class Connection::Private {
public:
    explicit Private(std::unique_ptr<ConnectionData>&& connection)
        : data(move(connection))
    {}
    Q_DISABLE_COPY(Private)
    DISABLE_MOVE(Private)

    Connection* q = nullptr;
    std::unique_ptr<ConnectionData> data;
    // A complex key below is a pair of room name and whether its
    // state is Invited. The spec mandates to keep Invited room state
    // separately; specifically, we should keep objects for Invite and
    // Leave state of the same room if the two happen to co-exist.
    QHash<QPair<QString, bool>, Room*> roomMap;
    /// Mapping from serverparts to alias/room id mappings,
    /// as of the last sync
    QHash<QString, QHash<QString, QString>> roomAliasMap;
    QVector<QString> roomIdsToForget;
    QVector<Room*> firstTimeRooms;
    QVector<QString> pendingStateRoomIds;
    QMap<QString, User*> userMap;
    DirectChatsMap directChats;
    DirectChatUsersMap directChatUsers;
    // The below two variables track local changes between sync completions.
    // See https://github.com/quotient-im/libQuotient/wiki/Handling-direct-chat-events
    DirectChatsMap dcLocalAdditions;
    DirectChatsMap dcLocalRemovals;
    UnorderedMap<QString, EventPtr> accountData;
    QMetaObject::Connection syncLoopConnection {};
    int syncTimeout = -1;

    GetCapabilitiesJob* capabilitiesJob = nullptr;
    GetCapabilitiesJob::Capabilities capabilities;

    QScopedPointer<EncryptionManager> encryptionManager;

    SyncJob* syncJob = nullptr;

    bool cacheState = true;
    bool cacheToBinary =
        SettingsGroup("libQuotient").get("cache_type",
                 SettingsGroup("libQMatrixClient").get<QString>("cache_type"))
        != "json";
    bool lazyLoading = false;

    void connectWithToken(const QString& userId, const QString& accessToken,
                          const QString& deviceId);
    void removeRoom(const QString& roomId);

    template <typename EventT>
    EventT* unpackAccountData() const
    {
        const auto& eventIt = accountData.find(EventT::matrixTypeId());
        return eventIt == accountData.end()
                   ? nullptr
                   : weakPtrCast<EventT>(eventIt->second);
    }

    void packAndSendAccountData(EventPtr&& event)
    {
        const auto eventType = event->matrixType();
        q->callApi<SetAccountDataJob>(data->userId(), eventType,
                                      event->contentJson());
        accountData[eventType] = std::move(event);
        emit q->accountDataChanged(eventType);
    }

    template <typename EventT, typename ContentT>
    void packAndSendAccountData(ContentT&& content)
    {
        packAndSendAccountData(
            makeEvent<EventT>(std::forward<ContentT>(content)));
    }
    QString topLevelStatePath() const
    {
        return q->stateCacheDir().filePath("state.json");
    }
};

Connection::Connection(const QUrl& server, QObject* parent)
    : QObject(parent), d(new Private(std::make_unique<ConnectionData>(server)))
{
    d->q = this; // All d initialization should occur before this line
}

Connection::Connection(QObject* parent) : Connection({}, parent) {}

Connection::~Connection()
{
    qCDebug(MAIN) << "deconstructing connection object for" << userId();
    stopSync();
}

void Connection::resolveServer(const QString& mxid)
{
    auto maybeBaseUrl = QUrl::fromUserInput(serverPart(mxid));
    maybeBaseUrl.setScheme("https"); // Instead of the Qt-default "http"
    if (maybeBaseUrl.isEmpty() || !maybeBaseUrl.isValid()) {
        emit resolveError(tr("%1 is not a valid homeserver address")
                              .arg(maybeBaseUrl.toString()));
        return;
    }

    setHomeserver(maybeBaseUrl);

    auto domain = maybeBaseUrl.host();
    qCDebug(MAIN) << "Finding the server" << domain;

    auto getWellKnownJob = callApi<GetWellknownJob>();
    connect(
        getWellKnownJob, &BaseJob::finished,
        [this, getWellKnownJob, maybeBaseUrl] {
            if (getWellKnownJob->status() == BaseJob::NotFoundError)
                qCDebug(MAIN) << "No .well-known file, IGNORE";
            else {
                if (getWellKnownJob->status() != BaseJob::Success) {
                    qCDebug(MAIN)
                        << "Fetching .well-known file failed, FAIL_PROMPT";
                    emit resolveError(tr("Fetching .well-known file failed"));
                    return;
                }
                QUrl baseUrl(getWellKnownJob->data().homeserver.baseUrl);
                if (baseUrl.isEmpty()) {
                    qCDebug(MAIN) << "base_url not provided, FAIL_PROMPT";
                    emit resolveError(tr("base_url not provided"));
                    return;
                }
                if (!baseUrl.isValid()) {
                    qCDebug(MAIN) << "base_url invalid, FAIL_ERROR";
                    emit resolveError(tr("base_url invalid"));
                    return;
                }

                qCDebug(MAIN) << ".well-known for" << maybeBaseUrl.host()
                              << "is" << baseUrl.toString();
                setHomeserver(baseUrl);
            }

            auto getVersionsJob = callApi<GetVersionsJob>();

            connect(getVersionsJob, &BaseJob::finished, [this, getVersionsJob] {
                if (getVersionsJob->status() == BaseJob::Success) {
                    qCDebug(MAIN) << "homeserver url is valid";
                    emit resolved();
                } else {
                    qCDebug(MAIN) << "homeserver url invalid";
                    emit resolveError(tr("homeserver url invalid"));
                }
            });
        });
}

void Connection::connectToServer(const QString& user, const QString& password,
                                 const QString& initialDeviceName,
                                 const QString& deviceId)
{
    checkAndConnect(user, [=] {
        doConnectToServer(user, password, initialDeviceName, deviceId);
    });
}
void Connection::doConnectToServer(const QString& user, const QString& password,
                                   const QString& initialDeviceName,
                                   const QString& deviceId)
{
    auto loginJob =
        callApi<LoginJob>(QStringLiteral("m.login.password"),
                          UserIdentifier { QStringLiteral("m.id.user"),
                                           { { QStringLiteral("user"), user } } },
                          password, /*token*/ "", deviceId, initialDeviceName);
    connect(loginJob, &BaseJob::success, this, [this, loginJob] {
        d->connectWithToken(loginJob->userId(), loginJob->accessToken(),
                            loginJob->deviceId());
        d->encryptionManager->uploadIdentityKeys(this);
        d->encryptionManager->uploadOneTimeKeys(this);
    });
    connect(loginJob, &BaseJob::failure, this, [this, loginJob] {
        emit loginError(loginJob->errorString(), loginJob->rawDataSample());
    });
}

void Connection::connectWithToken(const QString& userId,
                                  const QString& accessToken,
                                  const QString& deviceId)
{
    checkAndConnect(userId,
                    [=] { d->connectWithToken(userId, accessToken, deviceId); });
}

void Connection::reloadCapabilities()
{
    d->capabilitiesJob = callApi<GetCapabilitiesJob>(BackgroundRequest);
    connect(d->capabilitiesJob, &BaseJob::finished, this, [this] {
        if (d->capabilitiesJob->error() == BaseJob::Success)
            d->capabilities = d->capabilitiesJob->capabilities();
        else if (d->capabilitiesJob->error() == BaseJob::IncorrectRequestError)
            qCDebug(MAIN) << "Server doesn't support /capabilities";

        if (!d->capabilities.roomVersions) {
            qCWarning(MAIN) << "Pinning supported room version to 1";
            d->capabilities.roomVersions = { "1", { { "1", "stable" } } };
        } else {
            qCDebug(MAIN) << "Room versions:" << defaultRoomVersion()
                          << "is default, full list:" << availableRoomVersions();
        }
        Q_ASSERT(d->capabilities.roomVersions.has_value());
        emit capabilitiesLoaded();
        for (auto* r : qAsConst(d->roomMap))
            r->checkVersion();
    });
}

bool Connection::loadingCapabilities() const
{
    // (Ab)use the fact that room versions cannot be omitted after
    // the capabilities have been loaded (see reloadCapabilities() above).
    return !d->capabilities.roomVersions;
}

void Connection::Private::connectWithToken(const QString& userId,
                                           const QString& accessToken,
                                           const QString& deviceId)
{
    data->setUserId(userId);
    q->user(); // Creates a User object for the local user
    data->setToken(accessToken.toLatin1());
    data->setDeviceId(deviceId);
    q->setObjectName(userId % '/' % deviceId);
    qCDebug(MAIN) << "Using server" << data->baseUrl().toDisplayString()
                  << "by user" << userId << "from device" << deviceId;
    AccountSettings accountSettings(userId);
    encryptionManager.reset(
        new EncryptionManager(accountSettings.encryptionAccountPickle()));
    if (accountSettings.encryptionAccountPickle().isEmpty()) {
        accountSettings.setEncryptionAccountPickle(
            encryptionManager->olmAccountPickle());
    }
    emit q->stateChanged();
    emit q->connected();
    q->reloadCapabilities();
}

void Connection::checkAndConnect(const QString& userId,
                                 std::function<void()> connectFn)
{
    if (d->data->baseUrl().isValid()) {
        connectFn();
        return;
    }
    // Not good to go, try to fix the homeserver URL.
    if (userId.startsWith('@') && userId.indexOf(':') != -1) {
        connectSingleShot(this, &Connection::homeserverChanged, this, connectFn);
        // NB: doResolveServer can emit resolveError, so this is a part of
        // checkAndConnect function contract.
        resolveServer(userId);
    } else
        emit resolveError(tr("%1 is an invalid homeserver URL")
                              .arg(d->data->baseUrl().toString()));
}

void Connection::logout()
{
    // If there's an ongoing sync job, stop it but don't break the sync loop yet
    const auto syncWasRunning = bool(d->syncJob);
    if (syncWasRunning)
    {
        d->syncJob->abandon();
        d->syncJob = nullptr;
    }
    const auto* job = callApi<LogoutJob>();
    connect(job, &LogoutJob::finished, this, [this, job, syncWasRunning] {
        if (job->status().good() || job->error() == BaseJob::Unauthorised
            || job->error() == BaseJob::ContentAccessError) {
            if (d->syncLoopConnection)
                disconnect(d->syncLoopConnection);
            d->data->setToken({});
            emit stateChanged();
            emit loggedOut();
        } else if (syncWasRunning)
            syncLoopIteration(); // Resume sync loop (or a single sync)
    });
}

void Connection::sync(int timeout)
{
    if (d->syncJob) {
        qCInfo(MAIN) << d->syncJob << "is already running";
        return;
    }

    d->syncTimeout = timeout;
    Filter filter;
    filter.room.edit().timeline.edit().limit.emplace(100);
    filter.room.edit().state.edit().lazyLoadMembers.emplace(d->lazyLoading);
    auto job = d->syncJob =
        callApi<SyncJob>(BackgroundRequest, d->data->lastEvent(), filter,
                         timeout);
    connect(job, &SyncJob::success, this, [this, job] {
        onSyncSuccess(job->takeData());
        d->syncJob = nullptr;
        emit syncDone();
    });
    connect(job, &SyncJob::retryScheduled, this,
            [this, job](int retriesTaken, int nextInMilliseconds) {
                emit networkError(job->errorString(), job->rawDataSample(),
                                  retriesTaken, nextInMilliseconds);
            });
    connect(job, &SyncJob::failure, this, [this, job] {
        d->syncJob = nullptr;
        if (job->error() == BaseJob::Unauthorised) {
            qCWarning(SYNCJOB)
                << "Sync job failed with Unauthorised - login expired?";
            emit loginError(job->errorString(), job->rawDataSample());
        } else
            emit syncError(job->errorString(), job->rawDataSample());
    });
}

void Connection::syncLoop(int timeout)
{
    if (d->syncLoopConnection && d->syncTimeout == timeout) {
        qCInfo(MAIN) << "Attempt to run sync loop but there's one already "
                        "running; nothing will be done";
        return;
    }
    std::swap(d->syncTimeout, timeout);
    if (d->syncLoopConnection) {
        qCInfo(MAIN) << "Timeout for next syncs changed from"
                        << timeout << "to" << d->syncTimeout;
    } else {
        d->syncLoopConnection = connect(this, &Connection::syncDone,
                                        this, &Connection::syncLoopIteration,
                                        Qt::QueuedConnection);
        syncLoopIteration(); // initial sync to start the loop
    }
}

void Connection::syncLoopIteration() { sync(d->syncTimeout); }

QJsonObject toJson(const DirectChatsMap& directChats)
{
    QJsonObject json;
    for (auto it = directChats.begin(); it != directChats.end();) {
        QJsonArray roomIds;
        const auto* user = it.key();
        for (; it != directChats.end() && it.key() == user; ++it)
            roomIds.append(*it);
        json.insert(user->id(), roomIds);
    }
    return json;
}

void Connection::onSyncSuccess(SyncData&& data, bool fromCache)
{
    d->data->setLastEvent(data.nextBatch());
    for (auto&& roomData : data.takeRoomData()) {
        const auto forgetIdx = d->roomIdsToForget.indexOf(roomData.roomId);
        if (forgetIdx != -1) {
            d->roomIdsToForget.removeAt(forgetIdx);
            if (roomData.joinState == JoinState::Leave) {
                qDebug(MAIN)
                    << "Room" << roomData.roomId
                    << "has been forgotten, ignoring /sync response for it";
                continue;
            }
            qWarning(MAIN) << "Room" << roomData.roomId
                           << "has just been forgotten but /sync returned it in"
                           << toCString(roomData.joinState)
                           << "state - suspiciously fast turnaround";
        }
        if (auto* r = provideRoom(roomData.roomId, roomData.joinState)) {
            d->pendingStateRoomIds.removeOne(roomData.roomId);
            r->updateData(std::move(roomData), fromCache);
            if (d->firstTimeRooms.removeOne(r)) {
                emit loadedRoomState(r);
                if (d->capabilities.roomVersions)
                    r->checkVersion();
                // Otherwise, the version will be checked in reloadCapabilities()
            }
        }
        // Let UI update itself after updating each room
        QCoreApplication::processEvents();
    }
    // After running this loop, the account data events not saved in
    // d->accountData (see the end of the loop body) are auto-cleaned away
    for (auto& eventPtr : data.takeAccountData()) {
        visit(
            *eventPtr,
            [this](const DirectChatEvent& dce) {
                // https://github.com/quotient-im/libQuotient/wiki/Handling-direct-chat-events
                const auto& usersToDCs = dce.usersToDirectChats();
                DirectChatsMap remoteRemovals =
                    erase_if(d->directChats, [&usersToDCs, this](auto it) {
                        return !(usersToDCs.contains(it.key()->id(), it.value())
                                 || d->dcLocalAdditions.contains(it.key(),
                                                                 it.value()));
                    });
                erase_if(d->directChatUsers, [&remoteRemovals](auto it) {
                    return remoteRemovals.contains(it.value(), it.key());
                });
                // Remove from dcLocalRemovals what the server already has.
                erase_if(d->dcLocalRemovals, [&remoteRemovals](auto it) {
                    return remoteRemovals.contains(it.key(), it.value());
                });
                if (MAIN().isDebugEnabled())
                    for (auto it = remoteRemovals.begin();
                         it != remoteRemovals.end(); ++it) {
                        qCDebug(MAIN)
                            << it.value() << "is no more a direct chat with"
                            << it.key()->id();
                    }

                DirectChatsMap remoteAdditions;
                for (auto it = usersToDCs.begin(); it != usersToDCs.end(); ++it) {
                    if (auto* u = user(it.key())) {
                        if (!d->directChats.contains(u, it.value())
                            && !d->dcLocalRemovals.contains(u, it.value())) {
                            Q_ASSERT(!d->directChatUsers.contains(it.value(), u));
                            remoteAdditions.insert(u, it.value());
                            d->directChats.insert(u, it.value());
                            d->directChatUsers.insert(it.value(), u);
                            qCDebug(MAIN) << "Marked room" << it.value()
                                          << "as a direct chat with" << u->id();
                        }
                    } else
                        qCWarning(MAIN)
                            << "Couldn't get a user object for" << it.key();
                }
                // Remove from dcLocalAdditions what the server already has.
                erase_if(d->dcLocalAdditions, [&remoteAdditions](auto it) {
                    return remoteAdditions.contains(it.key(), it.value());
                });
                if (!remoteAdditions.isEmpty() || !remoteRemovals.isEmpty())
                    emit directChatsListChanged(remoteAdditions, remoteRemovals);
            },
            // catch-all, passing eventPtr for a possible take-over
            [this, &eventPtr](const Event& accountEvent) {
                if (is<IgnoredUsersEvent>(accountEvent))
                    qCDebug(MAIN)
                        << "Users ignored by" << userId() << "updated:"
                        << QStringList::fromSet(ignoredUsers()).join(',');

                auto& currentData = d->accountData[accountEvent.matrixType()];
                // A polymorphic event-specific comparison might be a bit
                // more efficient; maaybe do it another day
                if (!currentData
                    || currentData->contentJson() != accountEvent.contentJson()) {
                    currentData = std::move(eventPtr);
                    qCDebug(MAIN) << "Updated account data of type"
                                  << currentData->matrixType();
                    emit accountDataChanged(currentData->matrixType());
                }
            });
    }
    if (!d->dcLocalAdditions.isEmpty() || !d->dcLocalRemovals.isEmpty()) {
        qDebug(MAIN) << "Sending updated direct chats to the server:"
                     << d->dcLocalRemovals.size() << "removal(s),"
                     << d->dcLocalAdditions.size() << "addition(s)";
        callApi<SetAccountDataJob>(userId(), QStringLiteral("m.direct"),
                                   toJson(d->directChats));
        d->dcLocalAdditions.clear();
        d->dcLocalRemovals.clear();
    }
}

void Connection::stopSync()
{
    // If there's a sync loop, break it
    disconnect(d->syncLoopConnection);
    if (d->syncJob) // If there's an ongoing sync job, stop it too
    {
        d->syncJob->abandon();
        d->syncJob = nullptr;
    }
}

QString Connection::nextBatchToken() const { return d->data->lastEvent(); }

PostReceiptJob* Connection::postReceipt(Room* room, RoomEvent* event) const
{
    return callApi<PostReceiptJob>(room->id(), "m.read", event->id());
}

JoinRoomJob* Connection::joinRoom(const QString& roomAlias,
                                  const QStringList& serverNames)
{
    auto job = callApi<JoinRoomJob>(roomAlias, serverNames);
    // Upon completion, ensure a room object in Join state is created
    // (or it might already be there due to a sync completing earlier).
    // finished() is used here instead of success() to overtake clients
    // that may add their own slots to finished().
    connect(job, &BaseJob::finished, this, [this, job] {
        if (job->status().good())
            provideRoom(job->roomId());
    });
    return job;
}

LeaveRoomJob* Connection::leaveRoom(Room* room)
{
    const auto& roomId = room->id();
    const auto job = callApi<LeaveRoomJob>(roomId);
    if (room->joinState() == JoinState::Invite) {
        // Workaround matrix-org/synapse#2181 - if the room is in invite state
        // the invite may have been cancelled but Synapse didn't send it in
        // `/sync`. See also #273 for the discussion in the library context.
        d->pendingStateRoomIds.push_back(roomId);
        connect(job, &LeaveRoomJob::success, this, [this, roomId] {
            if (d->pendingStateRoomIds.removeOne(roomId)) {
                qCDebug(MAIN) << "Forcing the room to Leave status";
                provideRoom(roomId, JoinState::Leave);
            }
        });
    }
    return job;
}

inline auto splitMediaId(const QString& mediaId)
{
    auto idParts = mediaId.split('/');
    Q_ASSERT_X(idParts.size() == 2, __FUNCTION__,
               ("'" + mediaId + "' doesn't look like 'serverName/localMediaId'")
                   .toLatin1());
    return idParts;
}

MediaThumbnailJob* Connection::getThumbnail(const QString& mediaId,
                                            QSize requestedSize,
                                            RunningPolicy policy) const
{
    auto idParts = splitMediaId(mediaId);
    return callApi<MediaThumbnailJob>(policy, idParts.front(), idParts.back(),
                                      requestedSize);
}

MediaThumbnailJob* Connection::getThumbnail(const QUrl& url, QSize requestedSize,
                                            RunningPolicy policy) const
{
    return getThumbnail(url.authority() + url.path(), requestedSize, policy);
}

MediaThumbnailJob* Connection::getThumbnail(const QUrl& url, int requestedWidth,
                                            int requestedHeight,
                                            RunningPolicy policy) const
{
    return getThumbnail(url, QSize(requestedWidth, requestedHeight), policy);
}

UploadContentJob*
Connection::uploadContent(QIODevice* contentSource, const QString& filename,
                          const QString& overrideContentType) const
{
    Q_ASSERT(contentSource != nullptr);
    auto contentType = overrideContentType;
    if (contentType.isEmpty()) {
        contentType = QMimeDatabase()
                          .mimeTypeForFileNameAndData(filename, contentSource)
                          .name();
        if (!contentSource->open(QIODevice::ReadOnly)) {
            qCWarning(MAIN) << "Couldn't open content source" << filename
                            << "for reading:" << contentSource->errorString();
            return nullptr;
        }
    }
    return callApi<UploadContentJob>(contentSource, filename, contentType);
}

UploadContentJob* Connection::uploadFile(const QString& fileName,
                                         const QString& overrideContentType)
{
    auto sourceFile = new QFile(fileName);
    return uploadContent(sourceFile, QFileInfo(*sourceFile).fileName(),
                         overrideContentType);
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
    auto* job =
        callApi<DownloadFileJob>(idParts.front(), idParts.back(), localFilename);
    return job;
}

CreateRoomJob*
Connection::createRoom(RoomVisibility visibility, const QString& alias,
                       const QString& name, const QString& topic,
                       QStringList invites, const QString& presetName,
                       const QString& roomVersion, bool isDirect,
                       const QVector<CreateRoomJob::StateEvent>& initialState,
                       const QVector<CreateRoomJob::Invite3pid>& invite3pids,
                       const QJsonObject& creationContent)
{
    invites.removeOne(userId()); // The creator is by definition in the room
    auto job = callApi<CreateRoomJob>(visibility == PublishRoom
                                          ? QStringLiteral("public")
                                          : QStringLiteral("private"),
                                      alias, name, topic, invites, invite3pids,
                                      roomVersion, creationContent,
                                      initialState, presetName, isDirect);
    connect(job, &BaseJob::success, this, [this, job, invites, isDirect] {
        auto* room = provideRoom(job->roomId(), JoinState::Join);
        if (!room) {
            Q_ASSERT_X(room, "Connection::createRoom",
                       "Failed to create a room");
            return;
        }
        emit createdRoom(room);
        if (isDirect)
            for (const auto& i : invites)
                addToDirectChats(room, user(i));
    });
    return job;
}

void Connection::requestDirectChat(const QString& userId)
{
    doInDirectChat(userId, [this](Room* r) { emit directChatAvailable(r); });
}

void Connection::requestDirectChat(User* u)
{
    doInDirectChat(u, [this](Room* r) { emit directChatAvailable(r); });
}

void Connection::doInDirectChat(const QString& userId,
                                const std::function<void(Room*)>& operation)
{
    if (auto* u = user(userId))
        doInDirectChat(u, operation);
    else
        qCCritical(MAIN)
            << "Connection::doInDirectChat: Couldn't get a user object for"
            << userId;
}

void Connection::doInDirectChat(User* u,
                                const std::function<void(Room*)>& operation)
{
    Q_ASSERT(u);
    const auto& otherUserId = u->id();
    // There can be more than one DC; find the first valid (existing and
    // not left), and delete inexistent (forgotten?) ones along the way.
    DirectChatsMap removals;
    for (auto it = d->directChats.find(u);
         it != d->directChats.end() && it.key() == u; ++it) {
        const auto& roomId = *it;
        if (auto r = room(roomId, JoinState::Join)) {
            Q_ASSERT(r->id() == roomId);
            // A direct chat with yourself should only involve yourself :)
            if (otherUserId == userId() && r->totalMemberCount() > 1)
                continue;
            qCDebug(MAIN) << "Requested direct chat with" << otherUserId
                          << "is already available as" << r->id();
            operation(r);
            return;
        }
        if (auto ir = invitation(roomId)) {
            Q_ASSERT(ir->id() == roomId);
            auto j = joinRoom(ir->id());
            connect(j, &BaseJob::success, this,
                    [this, roomId, otherUserId, operation] {
                        qCDebug(MAIN)
                            << "Joined the already invited direct chat with"
                            << otherUserId << "as" << roomId;
                        operation(room(roomId, JoinState::Join));
                    });
            return;
        }
        // Avoid reusing previously left chats but don't remove them
        // from direct chat maps, either.
        if (room(roomId, JoinState::Leave))
            continue;

        qCWarning(MAIN) << "Direct chat with" << otherUserId << "known as room"
                        << roomId << "is not valid and will be discarded";
        // Postpone actual deletion until we finish iterating d->directChats.
        removals.insert(it.key(), it.value());
        // Add to the list of updates to send to the server upon the next sync.
        d->dcLocalRemovals.insert(it.key(), it.value());
    }
    if (!removals.isEmpty()) {
        for (auto it = removals.cbegin(); it != removals.cend(); ++it) {
            d->directChats.remove(it.key(), it.value());
            d->directChatUsers.remove(it.value(),
                                      const_cast<User*>(it.key())); // FIXME
        }
        emit directChatsListChanged({}, removals);
    }

    auto j = createDirectChat(otherUserId);
    connect(j, &BaseJob::success, this, [this, j, otherUserId, operation] {
        qCDebug(MAIN) << "Direct chat with" << otherUserId << "has been created as"
                      << j->roomId();
        operation(room(j->roomId(), JoinState::Join));
    });
}

CreateRoomJob* Connection::createDirectChat(const QString& userId,
                                            const QString& topic,
                                            const QString& name)
{
    return createRoom(UnpublishRoom, {}, name, topic, { userId },
                      QStringLiteral("trusted_private_chat"), {}, true);
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
    auto room = d->roomMap.value({ id, false });
    if (!room)
        room = d->roomMap.value({ id, true });
    if (room && room->joinState() != JoinState::Leave) {
        auto leaveJob = leaveRoom(room);
        connect(leaveJob, &BaseJob::result, this,
                [this, leaveJob, forgetJob, room] {
                    if (leaveJob->error() == BaseJob::Success
                        || leaveJob->error() == BaseJob::NotFoundError) {
                        run(forgetJob);
                        // If the matching /sync response hasn't arrived yet,
                        // mark the room for explicit deletion
                        if (room->joinState() != JoinState::Leave)
                            d->roomIdsToForget.push_back(room->id());
                    } else {
                        qCWarning(MAIN).nospace()
                            << "Error leaving room " << room->objectName()
                            << ": " << leaveJob->errorString();
                        forgetJob->abandon();
                    }
                });
        connect(leaveJob, &BaseJob::failure, forgetJob, &BaseJob::abandon);
    } else
        run(forgetJob);
    connect(forgetJob, &BaseJob::result, this, [this, id, forgetJob] {
        // Leave room in case of success, or room not known by server
        if (forgetJob->error() == BaseJob::Success
            || forgetJob->error() == BaseJob::NotFoundError)
            d->removeRoom(id); // Delete the room from roomMap
        else
            qCWarning(MAIN).nospace() << "Error forgetting room " << id << ": "
                                      << forgetJob->errorString();
    });
    return forgetJob;
}

SendToDeviceJob*
Connection::sendToDevices(const QString& eventType,
                          const UsersToDevicesToEvents& eventsMap) const
{
    QHash<QString, QHash<QString, QJsonObject>> json;
    json.reserve(int(eventsMap.size()));
    std::for_each(eventsMap.begin(), eventsMap.end(),
                  [&json](const auto& userTodevicesToEvents) {
                      auto& jsonUser = json[userTodevicesToEvents.first];
                      const auto& devicesToEvents = userTodevicesToEvents.second;
                      std::for_each(devicesToEvents.begin(),
                                    devicesToEvents.end(),
                                    [&jsonUser](const auto& deviceToEvents) {
                                        jsonUser.insert(
                                            deviceToEvents.first,
                                            deviceToEvents.second.contentJson());
                                    });
                  });
    return callApi<SendToDeviceJob>(BackgroundRequest, eventType,
                                    generateTxnId(), json);
}

SendMessageJob* Connection::sendMessage(const QString& roomId,
                                        const RoomEvent& event) const
{
    const auto txnId = event.transactionId().isEmpty() ? generateTxnId()
                                                       : event.transactionId();
    return callApi<SendMessageJob>(roomId, event.matrixType(), txnId,
                                   event.contentJson());
}

QUrl Connection::homeserver() const { return d->data->baseUrl(); }

QString Connection::domain() const { return userId().section(':', 1); }

Room* Connection::room(const QString& roomId, JoinStates states) const
{
    Room* room = d->roomMap.value({ roomId, false }, nullptr);
    if (states.testFlag(JoinState::Join) && room
        && room->joinState() == JoinState::Join)
        return room;

    if (states.testFlag(JoinState::Invite))
        if (Room* invRoom = invitation(roomId))
            return invRoom;

    if (states.testFlag(JoinState::Leave) && room
        && room->joinState() == JoinState::Leave)
        return room;

    return nullptr;
}

Room* Connection::roomByAlias(const QString& roomAlias, JoinStates states) const
{
    const auto id = d->roomAliasMap.value(serverPart(roomAlias)).value(roomAlias);
    if (!id.isEmpty())
        return room(id, states);

    qCWarning(MAIN) << "Room for alias" << roomAlias
                    << "is not found under account" << userId();
    return nullptr;
}

void Connection::updateRoomAliases(const QString& roomId,
                                   const QString& aliasServer,
                                   const QStringList& previousRoomAliases,
                                   const QStringList& roomAliases)
{
    auto& aliasMap = d->roomAliasMap[aliasServer]; // Allocate if necessary
    for (const auto& a : previousRoomAliases)
        if (aliasMap.remove(a) == 0)
            qCWarning(MAIN) << "Alias" << a << "is not found (already deleted?)";

    for (const auto& a : roomAliases) {
        auto& mappedId = aliasMap[a];
        if (!mappedId.isEmpty()) {
            if (mappedId == roomId)
                qCDebug(MAIN)
                    << "Alias" << a << "is already mapped to" << roomId;
            else
                qCWarning(MAIN) << "Alias" << a << "will be force-remapped from"
                                << mappedId << "to" << roomId;
        }
        mappedId = roomId;
    }
}

Room* Connection::invitation(const QString& roomId) const
{
    return d->roomMap.value({ roomId, true }, nullptr);
}

User* Connection::user(const QString& uId)
{
    if (uId.isEmpty())
        return nullptr;
    if (!uId.startsWith('@') || !uId.contains(':')) {
        qCCritical(MAIN) << "Malformed userId:" << uId;
        return nullptr;
    }
    if (d->userMap.contains(uId))
        return d->userMap.value(uId);
    auto* user = userFactory()(this, uId);
    d->userMap.insert(uId, user);
    emit newUser(user);
    return user;
}

const User* Connection::user() const
{
    return d->userMap.value(userId(), nullptr);
}

User* Connection::user() { return user(userId()); }

QString Connection::userId() const { return d->data->userId(); }

QString Connection::deviceId() const { return d->data->deviceId(); }

QByteArray Connection::accessToken() const { return d->data->accessToken(); }

QtOlm::Account* Connection::olmAccount() const
{
    return d->encryptionManager->account();
}

SyncJob* Connection::syncJob() const { return d->syncJob; }

int Connection::millisToReconnect() const
{
    return d->syncJob ? d->syncJob->millisToRetry() : 0;
}

QHash<QPair<QString, bool>, Room*> Connection::roomMap() const
{
    // Copy-on-write-and-remove-elements is faster than copying elements one by
    // one.
    QHash<QPair<QString, bool>, Room*> roomMap = d->roomMap;
    for (auto it = roomMap.begin(); it != roomMap.end();) {
        if (it.value()->joinState() == JoinState::Leave)
            it = roomMap.erase(it);
        else
            ++it;
    }
    return roomMap;
}

QVector<Room*> Connection::allRooms() const
{
    QVector<Room*> result;
    result.resize(d->roomMap.size());
    std::copy(d->roomMap.cbegin(), d->roomMap.cend(), result.begin());
    return result;
}

QVector<Room*> Connection::rooms(JoinStates joinStates) const
{
    QVector<Room*> result;
    for (auto* r: qAsConst(d->roomMap))
        if (joinStates.testFlag(r->joinState()))
            result.push_back(r);
    return result;
}

int Connection::roomsCount(JoinStates joinStates) const
{
    // Using int to maintain compatibility with QML
    // (consider also that QHash<>::size() returns int anyway).
    return int(std::count_if(d->roomMap.begin(), d->roomMap.end(),
                             [joinStates](Room* r) {
                                 return joinStates.testFlag(r->joinState());
                             }));
}

bool Connection::hasAccountData(const QString& type) const
{
    return d->accountData.find(type) != d->accountData.cend();
}

const EventPtr& Connection::accountData(const QString& type) const
{
    static EventPtr NoEventPtr {};
    auto it = d->accountData.find(type);
    return it == d->accountData.end() ? NoEventPtr : it->second;
}

QJsonObject Connection::accountDataJson(const QString& type) const
{
    const auto& eventPtr = accountData(type);
    return eventPtr ? eventPtr->contentJson() : QJsonObject();
}

void Connection::setAccountData(EventPtr&& event)
{
    d->packAndSendAccountData(std::move(event));
}

void Connection::setAccountData(const QString& type, const QJsonObject& content)
{
    d->packAndSendAccountData(loadEvent<Event>(type, content));
}

QHash<QString, QVector<Room*>> Connection::tagsToRooms() const
{
    QHash<QString, QVector<Room*>> result;
    for (auto* r : qAsConst(d->roomMap)) {
        const auto& tagNames = r->tagNames();
        for (const auto& tagName : tagNames)
            result[tagName].push_back(r);
    }
    for (auto it = result.begin(); it != result.end(); ++it)
        std::sort(it->begin(), it->end(), [t = it.key()](Room* r1, Room* r2) {
            return r1->tags().value(t) < r2->tags().value(t);
        });
    return result;
}

QStringList Connection::tagNames() const
{
    QStringList tags({ FavouriteTag });
    for (auto* r : qAsConst(d->roomMap)) {
        const auto& tagNames = r->tagNames();
        for (const auto& tag : tagNames)
            if (tag != LowPriorityTag && !tags.contains(tag))
                tags.push_back(tag);
    }
    tags.push_back(LowPriorityTag);
    return tags;
}

QVector<Room*> Connection::roomsWithTag(const QString& tagName) const
{
    QVector<Room*> rooms;
    std::copy_if(d->roomMap.begin(), d->roomMap.end(), std::back_inserter(rooms),
                 [&tagName](Room* r) { return r->tags().contains(tagName); });
    return rooms;
}

DirectChatsMap Connection::directChats() const
{
    return d->directChats;
}

// Removes room with given id from roomMap
void Connection::Private::removeRoom(const QString& roomId)
{
    for (auto f : { false, true })
        if (auto r = roomMap.take({ roomId, f })) {
            qCDebug(MAIN) << "Room" << r->objectName() << "in state"
                          << toCString(r->joinState()) << "will be deleted";
            emit r->beforeDestruction(r);
            r->deleteLater();
        }
}

void Connection::addToDirectChats(const Room* room, User* user)
{
    Q_ASSERT(room != nullptr && user != nullptr);
    if (d->directChats.contains(user, room->id()))
        return;
    Q_ASSERT(!d->directChatUsers.contains(room->id(), user));
    d->directChats.insert(user, room->id());
    d->directChatUsers.insert(room->id(), user);
    d->dcLocalAdditions.insert(user, room->id());
    emit directChatsListChanged({ { user, room->id() } }, {});
}

void Connection::removeFromDirectChats(const QString& roomId, User* user)
{
    Q_ASSERT(!roomId.isEmpty());
    if ((user != nullptr && !d->directChats.contains(user, roomId))
        || d->directChats.key(roomId) == nullptr)
        return;

    DirectChatsMap removals;
    if (user != nullptr) {
        d->directChats.remove(user, roomId);
        d->directChatUsers.remove(roomId, user);
        removals.insert(user, roomId);
        d->dcLocalRemovals.insert(user, roomId);
    } else {
        removals = erase_if(d->directChats,
                            [&roomId](auto it) { return it.value() == roomId; });
        d->directChatUsers.remove(roomId);
        d->dcLocalRemovals += removals;
    }
    emit directChatsListChanged({}, removals);
}

bool Connection::isDirectChat(const QString& roomId) const
{
    return d->directChatUsers.contains(roomId);
}

QList<User*> Connection::directChatUsers(const Room* room) const
{
    Q_ASSERT(room != nullptr);
    return d->directChatUsers.values(room->id());
}

bool Connection::isIgnored(const User* user) const
{
    return ignoredUsers().contains(user->id());
}

IgnoredUsersList Connection::ignoredUsers() const
{
    const auto* event = d->unpackAccountData<IgnoredUsersEvent>();
    return event ? event->ignored_users() : IgnoredUsersList();
}

void Connection::addToIgnoredUsers(const User* user)
{
    Q_ASSERT(user != nullptr);

    auto ignoreList = ignoredUsers();
    if (!ignoreList.contains(user->id())) {
        ignoreList.insert(user->id());
        d->packAndSendAccountData<IgnoredUsersEvent>(ignoreList);
        emit ignoredUsersListChanged({ { user->id() } }, {});
    }
}

void Connection::removeFromIgnoredUsers(const User* user)
{
    Q_ASSERT(user != nullptr);

    auto ignoreList = ignoredUsers();
    if (ignoreList.remove(user->id()) != 0) {
        d->packAndSendAccountData<IgnoredUsersEvent>(ignoreList);
        emit ignoredUsersListChanged({}, { { user->id() } });
    }
}

QMap<QString, User*> Connection::users() const { return d->userMap; }

const ConnectionData* Connection::connectionData() const
{
    return d->data.get();
}

Room* Connection::provideRoom(const QString& id, Omittable<JoinState> joinState)
{
    // TODO: This whole function is a strong case for a RoomManager class.
    Q_ASSERT_X(!id.isEmpty(), __FUNCTION__, "Empty room id");

    // If joinState is empty, all joinState == comparisons below are false.
    const auto roomKey = qMakePair(id, joinState == JoinState::Invite);
    auto* room = d->roomMap.value(roomKey, nullptr);
    if (room) {
        // Leave is a special case because in transition (5a) (see the .h file)
        // joinState == room->joinState but we still have to preempt the Invite
        // and emit a signal. For Invite and Join, there's no such problem.
        if (room->joinState() == joinState && joinState != JoinState::Leave)
            return room;
    } else if (!joinState) {
        // No Join and Leave, maybe Invite?
        room = d->roomMap.value({ id, true }, nullptr);
        if (room)
            return room;
        // No Invite either, setup a new room object below
    }

    if (!room) {
        room = roomFactory()(this, id, joinState.value_or(JoinState::Join));
        if (!room) {
            qCCritical(MAIN) << "Failed to create a room" << id;
            return nullptr;
        }
        d->roomMap.insert(roomKey, room);
        d->firstTimeRooms.push_back(room);
        connect(room, &Room::beforeDestruction, this,
                &Connection::aboutToDeleteRoom);
        emit newRoom(room);
    }
    if (!joinState)
        return room;

    if (*joinState == JoinState::Invite) {
        // prev is either Leave or nullptr
        auto* prev = d->roomMap.value({ id, false }, nullptr);
        emit invitedRoom(room, prev);
    } else {
        room->setJoinState(*joinState);
        // Preempt the Invite room (if any) with a room in Join/Leave state.
        auto* prevInvite = d->roomMap.take({ id, true });
        if (*joinState == JoinState::Join)
            emit joinedRoom(room, prevInvite);
        else if (*joinState == JoinState::Leave)
            emit leftRoom(room, prevInvite);
        if (prevInvite) {
            const auto dcUsers = prevInvite->directChatUsers();
            for (auto* u : dcUsers)
                addToDirectChats(room, u);
            qCDebug(MAIN) << "Deleting Invite state for room"
                          << prevInvite->id();
            emit prevInvite->beforeDestruction(prevInvite);
            prevInvite->deleteLater();
        }
    }

    return room;
}

void Connection::setRoomFactory(room_factory_t f)
{
    _roomFactory = std::move(f);
}

void Connection::setUserFactory(user_factory_t f)
{
    _userFactory = std::move(f);
}

room_factory_t Connection::roomFactory() { return _roomFactory; }

user_factory_t Connection::userFactory() { return _userFactory; }

room_factory_t Connection::_roomFactory = defaultRoomFactory<>();
user_factory_t Connection::_userFactory = defaultUserFactory<>();

QByteArray Connection::generateTxnId() const
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

void Connection::saveRoomState(Room* r) const
{
    Q_ASSERT(r);
    if (!d->cacheState)
        return;

    QFile outRoomFile { stateCacheDir().filePath(
        SyncData::fileNameForRoom(r->id())) };
    if (outRoomFile.open(QFile::WriteOnly)) {
        QJsonDocument json { r->toJson() };
        auto data = d->cacheToBinary ? json.toBinaryData()
                                     : json.toJson(QJsonDocument::Compact);
        outRoomFile.write(data.data(), data.size());
        qCDebug(MAIN) << "Room state cache saved to" << outRoomFile.fileName();
    } else {
        qCWarning(MAIN) << "Error opening" << outRoomFile.fileName() << ":"
                        << outRoomFile.errorString();
    }
}

void Connection::saveState() const
{
    if (!d->cacheState)
        return;

    QElapsedTimer et;
    et.start();

    QFile outFile { d->topLevelStatePath() };
    if (!outFile.open(QFile::WriteOnly)) {
        qCWarning(MAIN) << "Error opening" << outFile.fileName() << ":"
                        << outFile.errorString();
        qCWarning(MAIN) << "Caching the rooms state disabled";
        d->cacheState = false;
        return;
    }

    QJsonObject rootObj {
        { QStringLiteral("cache_version"),
          QJsonObject {
              { QStringLiteral("major"), SyncData::cacheVersion().first },
              { QStringLiteral("minor"), SyncData::cacheVersion().second } } }
    };
    {
        QJsonObject roomsJson;
        QJsonObject inviteRoomsJson;
        for (const auto* r: qAsConst(d->roomMap)) {
            if (r->joinState() == JoinState::Leave)
                continue;
            (r->joinState() == JoinState::Invite ? inviteRoomsJson : roomsJson)
                .insert(r->id(), QJsonValue::Null);
        }

        QJsonObject roomObj;
        if (!roomsJson.isEmpty())
            roomObj.insert(QStringLiteral("join"), roomsJson);
        if (!inviteRoomsJson.isEmpty())
            roomObj.insert(QStringLiteral("invite"), inviteRoomsJson);

        rootObj.insert(QStringLiteral("next_batch"), d->data->lastEvent());
        rootObj.insert(QStringLiteral("rooms"), roomObj);
    }
    {
        QJsonArray accountDataEvents {
            basicEventJson(QStringLiteral("m.direct"), toJson(d->directChats))
        };
        for (const auto& e : d->accountData)
            accountDataEvents.append(
                basicEventJson(e.first, e.second->contentJson()));

        rootObj.insert(QStringLiteral("account_data"),
                       QJsonObject {
                           { QStringLiteral("events"), accountDataEvents } });
    }

    QJsonDocument json { rootObj };
    auto data = d->cacheToBinary ? json.toBinaryData()
                                 : json.toJson(QJsonDocument::Compact);
    qCDebug(PROFILER) << "Cache for" << userId() << "generated in" << et;

    outFile.write(data.data(), data.size());
    qCDebug(MAIN) << "State cache saved to" << outFile.fileName();
}

void Connection::loadState()
{
    if (!d->cacheState)
        return;

    QElapsedTimer et;
    et.start();

    SyncData sync { d->topLevelStatePath() };
    if (sync.nextBatch().isEmpty()) // No token means no cache by definition
        return;

    if (!sync.unresolvedRooms().isEmpty()) {
        qCWarning(MAIN) << "State cache incomplete, discarding";
        return;
    }
    // TODO: to handle load failures, instead of the above block:
    // 1. Do initial sync on failed rooms without saving the nextBatch token
    // 2. Do the sync across all rooms as normal
    onSyncSuccess(std::move(sync), true);
    qCDebug(PROFILER) << "*** Cached state for" << userId() << "loaded in" << et;
}

QString Connection::stateCachePath() const
{
    return stateCacheDir().path() % '/';
}

QDir Connection::stateCacheDir() const
{
    auto safeUserId = userId();
    safeUserId.replace(':', '_');
    return cacheLocation(safeUserId);
}

bool Connection::cacheState() const { return d->cacheState; }

void Connection::setCacheState(bool newValue)
{
    if (d->cacheState != newValue) {
        d->cacheState = newValue;
        emit cacheStateChanged();
    }
}

bool Connection::lazyLoading() const { return d->lazyLoading; }

void Connection::setLazyLoading(bool newValue)
{
    if (d->lazyLoading != newValue) {
        d->lazyLoading = newValue;
        emit lazyLoadingChanged();
    }
}

void Connection::run(BaseJob* job, RunningPolicy runningPolicy) const
{
    connect(job, &BaseJob::failure, this, &Connection::requestFailed);
    job->initiate(d->data.get(), runningPolicy & BackgroundRequest);
}

void Connection::getTurnServers()
{
    auto job = callApi<GetTurnServerJob>();
    connect(job, &GetTurnServerJob::success, this,
            [=] { emit turnServersChanged(job->data()); });
}

const QString Connection::SupportedRoomVersion::StableTag =
    QStringLiteral("stable");

QString Connection::defaultRoomVersion() const
{
    Q_ASSERT(d->capabilities.roomVersions.has_value());
    return d->capabilities.roomVersions->defaultVersion;
}

QStringList Connection::stableRoomVersions() const
{
    Q_ASSERT(d->capabilities.roomVersions.has_value());
    QStringList l;
    const auto& allVersions = d->capabilities.roomVersions->available;
    for (auto it = allVersions.begin(); it != allVersions.end(); ++it)
        if (it.value() == SupportedRoomVersion::StableTag)
            l.push_back(it.key());
    return l;
}

inline bool roomVersionLess(const Connection::SupportedRoomVersion& v1,
                            const Connection::SupportedRoomVersion& v2)
{
    bool ok1 = false, ok2 = false;
    const auto vNum1 = v1.id.toFloat(&ok1);
    const auto vNum2 = v2.id.toFloat(&ok2);
    return ok1 && ok2 ? vNum1 < vNum2 : v1.id < v2.id;
}

QVector<Connection::SupportedRoomVersion> Connection::availableRoomVersions() const
{
    Q_ASSERT(d->capabilities.roomVersions.has_value());
    QVector<SupportedRoomVersion> result;
    result.reserve(d->capabilities.roomVersions->available.size());
    for (auto it = d->capabilities.roomVersions->available.begin();
         it != d->capabilities.roomVersions->available.end(); ++it)
        result.push_back({ it.key(), it.value() });
    // Put stable versions over unstable; within each group,
    // sort numeric versions as numbers, the rest as strings.
    const auto mid = std::partition(result.begin(), result.end(),
                                    std::mem_fn(&SupportedRoomVersion::isStable));
    std::sort(result.begin(), mid, roomVersionLess);
    std::sort(mid, result.end(), roomVersionLess);

    return result;
}
