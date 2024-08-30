#include "connectionencryptiondata_p.h"

#include "logging_categories_p.h"
#include "qt_connection_util.h"
#include "room.h"
#include "syncdata.h"
#include "user.h"

#include "e2ee/qolmutility.h"

#include "events/encryptedevent.h"
#include "events/roomkeyevent.h"

#include <qt6keychain/keychain.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QPromise>

using namespace Quotient;
using namespace Quotient::_impl;

// Below, encryptionData gets filled inside setupPicklingKey() instead of returning the future for
// a pickling key and then, in CED::setup(), another future for ConnectionEncryptionData because
// Qt versions before 6.5.2 don't handle QFutures with move-only data quite well (see QTBUG-112513).
// Oh, and unwrap() doesn't work with move-only types at all (QTBUG-127423). So it is a bit more
// verbose and repetitive than it should be.

inline QFuture<QKeychain::Job*> runKeychainJob(QKeychain::Job* j, const QString& keychainId)
{
    j->setAutoDelete(true);
    j->setKey(keychainId);
    auto ft = QtFuture::connect(j, &QKeychain::Job::finished);
    j->start();
    return ft;
}

QFuture<void> setupPicklingKey(Connection* connection, bool mock,
                               std::unique_ptr<ConnectionEncryptionData>& encryptionData)
{
    if (mock) {
        qInfo(E2EE) << "Using a mock pickling key";
        encryptionData =
            std::make_unique<ConnectionEncryptionData>(connection, PicklingKey::generate());
        return QtFuture::makeReadyFuture<void>();
    }

    using namespace QKeychain;
    const auto keychainId = connection->userId() + "-Pickle"_L1;
    qCInfo(MAIN) << "Keychain request: app" << qAppName() << "id" << keychainId;

    return runKeychainJob(new ReadPasswordJob(qAppName()), keychainId)
        .then([keychainId, &encryptionData, connection](const Job* j) -> QFuture<Job*> {
            // The future will hold nullptr if the existing pickling key was found and no write is
            // pending; a pointer to the write job if if a new key was made and is being written;
            // be cancelled in case of an error.
            switch (const auto readJob = static_cast<const ReadPasswordJob*>(j); readJob->error()) {
            case Error::NoError: {
                auto&& data = readJob->binaryData();
                if (data.size() == PicklingKey::extent) {
                    qDebug(E2EE) << "Successfully loaded pickling key from keychain";
                    encryptionData = std::make_unique<ConnectionEncryptionData>(
                        connection, PicklingKey::fromByteArray(std::move(data)));
                    return QtFuture::makeReadyFuture<Job*>(nullptr);
                }
                qCritical(E2EE)
                    << "The pickling key loaded from" << keychainId << "has length"
                    << data.size() << "but the library expected" << PicklingKey::extent;
                return {};
            }
            case Error::EntryNotFound: {
                auto&& picklingKey = PicklingKey::generate();
                auto writeJob = new WritePasswordJob(qAppName());
                writeJob->setBinaryData(picklingKey.viewAsByteArray());
                encryptionData = std::make_unique<ConnectionEncryptionData>(
                    connection, std::move(picklingKey)); // the future may still get cancelled
                qDebug(E2EE) << "Saving a new pickling key to the keychain";
                return runKeychainJob(writeJob, keychainId);
            }
            default:
                qWarning(E2EE) << "Error loading pickling key - please fix your keychain:"
                               << readJob->errorString();
            }
            return {};
        })
        .unwrap()
        .then([](QFuture<Job*> writeFuture) {
            if (const Job* const writeJob = writeFuture.result();
                writeJob && writeJob->error() != Error::NoError) //
            {
                qCritical(E2EE) << "Could not save pickling key to keychain: "
                                << writeJob->errorString();
                writeFuture.cancel();
            }
        });
}

QFuture<bool> ConnectionEncryptionData::setup(Connection* connection, bool mock,
                                              std::unique_ptr<ConnectionEncryptionData>& result)
{
    return setupPicklingKey(connection, mock, result)
        .then([connection, mock, &result] {
            if (mock) {
                result->database.clear();
                result->olmAccount.setupNewAccount();
                return true;
            }
            if (const auto outcome = result->database.setupOlmAccount(result->olmAccount)) {
                if (outcome == OLM_SUCCESS) {
                    qCDebug(E2EE) << "The existing Olm account successfully unpickled";
                    return true;
                }

                qCritical(E2EE) << "Could not unpickle Olm account for" << connection->objectName();
                return false;
            }
            qCDebug(E2EE) << "A new Olm account has been created, uploading device keys";
            connection->callApi<UploadKeysJob>(result->olmAccount.deviceKeys())
                .then(connection,
                    [connection, &result] {
                        result->trackedUsers += connection->userId();
                        result->outdatedUsers += connection->userId();
                        result->encryptionUpdateRequired = true;
                    },
                    [](auto* job) {
                        qCWarning(E2EE) << "Failed to upload device keys:" << job->errorString();
                    });
            return true;
        })
        .onCanceled([connection] {
            qCritical(E2EE) << "Could not setup E2EE for" << connection->objectName();
            return false;
        });
}

void ConnectionEncryptionData::saveDevicesList()
{
    database.transaction();
    auto query = database.prepareQuery(u"DELETE FROM tracked_users"_s);
    database.execute(query);
    query.prepare(u"INSERT INTO tracked_users(matrixId) VALUES(:matrixId);"_s);
    for (const auto& user : trackedUsers) {
        query.bindValue(u":matrixId"_s, user);
        database.execute(query);
    }

    query.prepare(u"DELETE FROM outdated_users"_s);
    database.execute(query);
    query.prepare(u"INSERT INTO outdated_users(matrixId) VALUES(:matrixId);"_s);
    for (const auto& user : outdatedUsers) {
        query.bindValue(u":matrixId"_s, user);
        database.execute(query);
    }

    query.prepare(
        u"INSERT INTO tracked_devices"
        "(matrixId, deviceId, curveKeyId, curveKey, edKeyId, edKey, verified, selfVerified) "
        "VALUES (:matrixId, :deviceId, :curveKeyId, :curveKey, :edKeyId, :edKey, :verified, :selfVerified);"_s);
    for (const auto& [user, devices] : deviceKeys.asKeyValueRange()) {
        auto deleteQuery =
            database.prepareQuery(u"DELETE FROM tracked_devices WHERE matrixId=:matrixId;"_s);
        deleteQuery.bindValue(u":matrixId"_s, user);
        database.execute(deleteQuery);
        for (const auto& device : devices) {
            const auto keys = device.keys.asKeyValueRange();
            deleteQuery.prepare(
                u"DELETE FROM tracked_devices WHERE matrixId=:matrixId AND deviceId=:deviceId;"_s);
            deleteQuery.bindValue(u":matrixId"_s, user);
            deleteQuery.bindValue(u":deviceId"_s, device.deviceId);
            database.execute(deleteQuery);

            const auto curveKeyIt = std::ranges::find_if(keys, [](const auto& p) {
                return p.first.startsWith("curve"_L1);
            });
            Q_ASSERT(curveKeyIt != keys.end());
            const auto edKeyIt = std::ranges::find_if(keys, [](const auto& p) {
                return p.first.startsWith("ed"_L1);
            });
            Q_ASSERT(edKeyIt != keys.end());

            query.bindValue(u":matrixId"_s, user);
            query.bindValue(u":deviceId"_s, device.deviceId);
            query.bindValue(u":curveKeyId"_s, curveKeyIt->first);
            query.bindValue(u":curveKey"_s, curveKeyIt->second);
            query.bindValue(u":edKeyId"_s, edKeyIt->first);
            query.bindValue(u":edKey"_s, edKeyIt->second);
            // If the device gets saved here, it can't be verified
            query.bindValue(u":verified"_s, verifiedDevices[user][device.deviceId]);
            query.bindValue(u":selfVerified"_s, selfVerifiedDevices[user][device.deviceId]);

            database.execute(query);
        }
    }
    database.commit();
}

void ConnectionEncryptionData::loadDevicesList()
{
    auto query =
        database.prepareQuery(QStringLiteral("SELECT * FROM tracked_users;"));
    database.execute(query);
    while (query.next()) {
        trackedUsers += query.value(0).toString();
    }

    query =
        database.prepareQuery(QStringLiteral("SELECT * FROM outdated_users;"));
    database.execute(query);
    while (query.next()) {
        outdatedUsers += query.value(0).toString();
    }

    static const QStringList Algorithms{ SupportedAlgorithms.cbegin(),
                                         SupportedAlgorithms.cend() };
    query =
        database.prepareQuery(QStringLiteral("SELECT * FROM tracked_devices;"));
    database.execute(query);
    while (query.next()) {
        deviceKeys[query.value("matrixId"_L1).toString()].insert(
            query.value("deviceId"_L1).toString(),
            {
                .userId = query.value("matrixId"_L1).toString(),
                .deviceId = query.value("deviceId"_L1).toString(),
                .algorithms = Algorithms,
                .keys{ { query.value("curveKeyId"_L1).toString(),
                         query.value("curveKey"_L1).toString() },
                       { query.value("edKeyId"_L1).toString(),
                         query.value("edKey"_L1).toString() } },
                .signatures{} // not needed after initial validation so not saved
            });
        selfVerifiedDevices[query.value("matrixId"_L1).toString()][query.value("deviceId"_L1).toString()] = query.value("selfVerified"_L1).toBool();
        verifiedDevices[query.value("matrixId"_L1).toString()][query.value("deviceId"_L1).toString()] = query.value("verified"_L1).toBool();
    }
}

QString ConnectionEncryptionData::curveKeyForUserDevice(
    const QString& userId, const QString& device) const
{
    return deviceKeys[userId][device].keys["curve25519:"_L1 + device];
}

bool ConnectionEncryptionData::isKnownCurveKey(const QString& userId,
                                               const QString& curveKey) const
{
    auto query = database.prepareQuery(
        QStringLiteral("SELECT * FROM tracked_devices WHERE matrixId=:matrixId "
                       "AND curveKey=:curveKey"));
    query.bindValue(":matrixId"_L1, userId);
    query.bindValue(":curveKey"_L1, curveKey);
    database.execute(query);
    return query.next();
}

bool ConnectionEncryptionData::hasOlmSession(const QString& user,
                                             const QString& deviceId) const
{
    const auto& curveKey = curveKeyForUserDevice(user, deviceId).toLatin1();
    const auto sessionIt = olmSessions.find(curveKey);
    return sessionIt != olmSessions.cend() && !sessionIt->second.empty();
}

void ConnectionEncryptionData::onSyncSuccess(SyncData& syncResponse)
{
    oneTimeKeysCount = syncResponse.deviceOneTimeKeysCount();
    if (oneTimeKeysCount[SignedCurve25519Key]
            < 0.4 * olmAccount.maxNumberOfOneTimeKeys()
        && !isUploadingKeys) {
        isUploadingKeys = true;
        olmAccount.generateOneTimeKeys(olmAccount.maxNumberOfOneTimeKeys() / 2
                                       - oneTimeKeysCount[SignedCurve25519Key]);
        auto keys = olmAccount.oneTimeKeys();
        auto job = olmAccount.createUploadKeyRequest(keys);
        q->run(job, ForegroundRequest);
        QObject::connect(job, &BaseJob::success, q,
                         [this] { olmAccount.markKeysAsPublished(); });
        QObject::connect(job, &BaseJob::result, q,
                         [this] { isUploadingKeys = false; });
    }
    if(firstSync) {
        loadDevicesList();
        firstSync = false;
    }

    consumeDevicesList(syncResponse.takeDevicesList());

    auto checkQuery = database.prepareQuery("SELECT * FROM master_keys WHERE userId=:userId"_L1);
    checkQuery.bindValue(":userId"_L1, q->userId());
    database.execute(checkQuery);
    const auto haveMasterKey = checkQuery.next();
    if (trackedUsers.contains(q->userId()) && !outdatedUsers.contains(q->userId()) && !haveMasterKey) {
        emit q->crossSigningSetupRequired();
    }

}

void ConnectionEncryptionData::consumeDevicesList(const DevicesList& devicesList)
{
    bool hasNewOutdatedUser = false;
    for(const auto &changed : devicesList.changed) {
        if(trackedUsers.contains(changed)) {
            outdatedUsers += changed;
            hasNewOutdatedUser = true;
        }
    }
    for(const auto &left : devicesList.left) {
        trackedUsers -= left;
        outdatedUsers -= left;
        deviceKeys.remove(left);
    }
    if(hasNewOutdatedUser)
        loadOutdatedUserDevices();
}

void ConnectionEncryptionData::loadOutdatedUserDevices()
{
    QHash<QString, QStringList> users;
    for(const auto &user : outdatedUsers) {
        users[user] += QStringList();
    }
    currentQueryKeysJob.abandon(); // Cancel network request explicitly
    currentQueryKeysJob = q->callApi<QueryKeysJob>(users).onResult(q, [this](QueryKeysJob* job) {
        if (job->status().good())
            handleQueryKeys(collectResponse(job));

        emit q->finishedQueryingKeys();
    });
}

void ConnectionEncryptionData::consumeToDeviceEvent(EventPtr toDeviceEvent)
{
    if (processIfVerificationEvent(*toDeviceEvent, false))
        return;
    if (auto&& event = eventCast<EncryptedEvent>(std::move(toDeviceEvent))) {
        if (event->algorithm() != OlmV1Curve25519AesSha2AlgoKey) {
            qCDebug(E2EE) << "Unsupported algorithm" << event->id()
                          << "for event" << event->algorithm();
            return;
        }
        if (isKnownCurveKey(event->senderId(), event->senderKey())) {
            handleEncryptedToDeviceEvent(*event);
            return;
        }
        trackedUsers += event->senderId();
        outdatedUsers += event->senderId();
        encryptionUpdateRequired = true;
        pendingEncryptedEvents.push_back(std::move(event));
    }
}

bool ConnectionEncryptionData::processIfVerificationEvent(const Event& evt,
                                                          bool encrypted)
{
    return switchOnType(
        evt,
        [this, encrypted](const KeyVerificationRequestEvent& reqEvt) {
            setupKeyVerificationSession(reqEvt.fullJson()[SenderKey].toString(),
                                        reqEvt, q, encrypted);
            return true;
        },
        [](const KeyVerificationDoneEvent&) {
            qCDebug(E2EE) << "Ignoring m.key.verification.done";
            return true;
        },
        [this](const KeyVerificationEvent& kvEvt) {
            if (auto* const session =
                    verificationSessions.value(kvEvt.transactionId())) {
                qCDebug(E2EE) << "Handling" << kvEvt.matrixType();
                session->handleEvent(kvEvt);
                emit q->keyVerificationStateChanged(session, session->state());
            }
            return true;
        },
        false);
}

class SecretSendEvent : public Event {
public:
    using Event::Event;
    QUO_EVENT(SecretSendEvent, "m.secret.send")
    QUO_CONTENT_GETTER(QString, requestId)
    QUO_CONTENT_GETTER(QString, secret)
};

void ConnectionEncryptionData::handleEncryptedToDeviceEvent(const EncryptedEvent& event)
{
    const auto [decryptedEvent, olmSessionId] = sessionDecryptMessage(event);
    if (!decryptedEvent) {
        qCWarning(E2EE) << "Failed to decrypt to-device event from device"
                        << event.deviceId();
        return;
    }

    if (processIfVerificationEvent(*decryptedEvent, true))
        return;
    decryptedEvent->switchOnType(
        [this, &event, olmSessionId](const RoomKeyEvent& roomKeyEvent) {
            if (auto* detectedRoom = q->room(roomKeyEvent.roomId())) {
                detectedRoom->handleRoomKeyEvent(roomKeyEvent, event.senderId(),
                                                 olmSessionId, event.senderKey().toLatin1(), q->edKeyForUserDevice(event.senderId(), event.deviceId()).toLatin1());
            } else {
                qCDebug(E2EE)
                    << "Encrypted event room id" << roomKeyEvent.roomId()
                    << "is not found at the connection" << q->objectName();
            }
        },
        [this](const SecretSendEvent& sse) {
            emit q->secretReceived(sse.requestId(), sse.secret());
        },
        [](const Event& evt) {
            qCWarning(E2EE) << "Skipping encrypted to_device event, type" << evt.matrixType();
        });
}

void ConnectionEncryptionData::handleMasterKeys(const QHash<QString, CrossSigningKey>& masterKeys)
{
    for (const auto &[userId, key] : asKeyValueRange(masterKeys)) {
        if (key.userId != userId) {
            qCWarning(E2EE) << "Master key: userId mismatch" << key.userId << userId;
            continue;
        }
        if (!key.usage.contains("master"_L1)) {
            qCWarning(E2EE) << "Master key: invalid usage" << key.usage;
            continue;
        }
        auto checkQuery = database.prepareQuery("SELECT * FROM master_keys WHERE userId=:userId"_L1);
        checkQuery.bindValue(":userId"_L1, key.userId);
        database.execute(checkQuery);
        if (checkQuery.next()) {
            if (checkQuery.value("key"_L1).toString() == key.keys.values()[0]) {
                continue;
            }
            qCWarning(E2EE) << "New master key for" << key.userId;
            database.transaction();
            auto query = database.prepareQuery(
                "UPDATE tracked_devices SET verified=0, selfVerified=0 WHERE matrixId=:matrixId;"_L1);
            query.bindValue(":matrixId"_L1, userId);
            database.execute(query);
            query = database.prepareQuery("DELETE FROM self_signing_keys WHERE userId=:userId;"_L1);
            query.bindValue(":userId"_L1, userId);
            database.execute(query);
            database.commit();
        }

        auto query = database.prepareQuery("DELETE FROM master_keys WHERE userId=:userId;"_L1);
        query.bindValue(":userId"_L1, userId);
        database.execute(query);
        query = database.prepareQuery("INSERT INTO master_keys(userId, key, verified) VALUES(:userId, :key, false);"_L1);
        query.bindValue(":userId"_L1, userId);
        query.bindValue(":key"_L1, key.keys.values()[0]);
        database.execute(query);
    }
}

namespace {
QString getEd25519Signature(const CrossSigningKey& keyObject, const QString& userId,
                            const QString& masterKey)
{
    return keyObject.signatures[userId]["ed25519:"_L1 + masterKey].toString();
}
}

void ConnectionEncryptionData::handleSelfSigningKeys(const QHash<QString, CrossSigningKey>& selfSigningKeys)
{
    for (const auto &[userId, key] : asKeyValueRange(selfSigningKeys)) {
        if (key.userId != userId) {
            qCWarning(E2EE) << "Self signing key: userId mismatch"<< key.userId << userId;
            continue;
        }
        if (!key.usage.contains("self_signing"_L1)) {
            qCWarning(E2EE) << "Self signing key: invalid usage" << key.usage;
            continue;
        }
        const auto masterKey = q->masterKeyForUser(userId);
        if (masterKey.isEmpty())
            continue;

        auto checkQuery = database.prepareQuery("SELECT key FROM self_signing_keys WHERE userId=:userId;"_L1);
        checkQuery.bindValue(":userId"_L1, userId);
        database.execute(checkQuery);
        if (checkQuery.next()) {
            auto oldKey = checkQuery.value("key"_L1).toString();
            if (oldKey != key.keys.values()[0]) {
                qCWarning(E2EE) << "New self-signing key for" << userId << ". Marking all devices as unverified.";
                database.transaction();
                auto query = database.prepareQuery(
                    "UPDATE tracked_devices SET verified=0, selfVerified=0 WHERE matrixId=:matrixId;"_L1);
                query.bindValue(":matrixId"_L1, userId);
                database.execute(query);
                database.commit();
            }
        }

        if (!ed25519VerifySignature(masterKey, toJson(key),
                                    getEd25519Signature(key, userId, masterKey))) {
            qCWarning(E2EE) << "Self signing key: failed signature verification" << userId;
            continue;
        }
        auto query = database.prepareQuery("DELETE FROM self_signing_keys WHERE userId=:userId;"_L1);
        query.bindValue(":userId"_L1, userId);
        database.execute(query);
        query = database.prepareQuery("INSERT INTO self_signing_keys(userId, key) VALUES(:userId, :key);"_L1);
        query.bindValue(":userId"_L1, userId);
        query.bindValue(":key"_L1, key.keys.values()[0]);
        database.execute(query);
    }
}

void ConnectionEncryptionData::handleUserSigningKeys(const QHash<QString, CrossSigningKey>& userSigningKeys)
{
    for (const auto &[userId, key] : asKeyValueRange(userSigningKeys)) {
        if (key.userId != userId) {
            qWarning() << "User signing key: userId mismatch" << key.userId << userId;
            continue;
        }
        if (!key.usage.contains("user_signing"_L1)) {
            qWarning() << "User signing key: invalid usage" << key.usage;
            continue;
        }
        const auto masterKey = q->masterKeyForUser(userId);
        if (masterKey.isEmpty())
            continue;

        auto checkQuery = database.prepareQuery("SELECT key FROM user_signing_keys WHERE userId=:userId"_L1);
        checkQuery.bindValue(":userId"_L1, userId);
        database.execute(checkQuery);
        if (checkQuery.next()) {
            auto oldKey = checkQuery.value("key"_L1).toString();
            if (oldKey != key.keys.values()[0]) {
                qCWarning(E2EE) << "New user signing key; marking all master signing keys as unverified" << userId;
                database.transaction();
                auto query = database.prepareQuery(
                    "UPDATE master_keys SET verified=0;"_L1);
                database.execute(query);
                database.commit();
            }
        }

        if (!ed25519VerifySignature(masterKey, toJson(key),
                                    getEd25519Signature(key, userId, masterKey))) {
            qWarning() << "User signing key: failed signature verification" << userId;
            continue;
        }
        auto query = database.prepareQuery("DELETE FROM user_signing_keys WHERE userId=:userId;"_L1);
        query.bindValue(":userId"_L1, userId);
        database.execute(query);
        query = database.prepareQuery("INSERT INTO user_signing_keys(userId, key) VALUES(:userId, :key);"_L1);
        query.bindValue(":userId"_L1, userId);
        query.bindValue(":key"_L1, key.keys.values()[0]);
        database.execute(query);
    }
}

void ConnectionEncryptionData::checkVerifiedMasterKeys(const QHash<QString, CrossSigningKey>& masterKeys)
{
    if (!q->isUserVerified(q->userId())) {
        return;
    }
    auto query = database.prepareQuery("SELECT key FROM user_signing_keys WHERE userId=:userId;"_L1);
    query.bindValue(":userId"_L1, q->userId());
    database.execute(query);
    if (!query.next()) {
        return;
    }
    const auto userSigningKey = query.value("key"_L1).toString();
    for (const auto& masterKey : masterKeys) {
        auto signature = getEd25519Signature(masterKey, q->userId(), userSigningKey);
        if (signature.isEmpty()) {
            continue;
        }
        if (ed25519VerifySignature(userSigningKey, toJson(masterKey), signature)) {
            database.setMasterKeyVerified(masterKey.keys.values()[0]);
            emit q->userVerified(masterKey.userId);
        } else {
            qCWarning(E2EE) << "Master key signature verification failed" << masterKey.userId;
        }
    }
}

void ConnectionEncryptionData::handleDevicesList(
    const QHash<QString, QHash<QString, QueryKeysJob::DeviceInformation>>& newDeviceKeys)
{
    for(const auto &[user, keys] : newDeviceKeys.asKeyValueRange()) {
        const auto oldDevices = deviceKeys[user];
        auto query = database.prepareQuery("SELECT * FROM self_signing_keys WHERE userId=:userId;"_L1);
        query.bindValue(":userId"_L1, user);
        database.execute(query);
        const auto selfSigningKey = query.next() ? query.value("key"_L1).toString() : QString();
        deviceKeys[user].clear();
        selfVerifiedDevices[user].clear();
        for (const auto &device : keys) {
            if (device.userId != user) {
                qWarning(E2EE)
                    << "mxId mismatch during device key verification:"
                    << device.userId << user;
                continue;
            }
            if (!std::all_of(device.algorithms.cbegin(),
                             device.algorithms.cend(), isSupportedAlgorithm)) {
                qWarning(E2EE) << "Unsupported encryption algorithms found"
                               << device.algorithms;
                continue;
            }
            if (!verifyIdentitySignature(device, device.deviceId,
                                         device.userId)) {
                qWarning(E2EE) << "Failed to verify device keys signature. "
                                  "Skipping device" << device.userId << device.deviceId;
                continue;
            }
            if (const auto oldDeviceKeys = oldDevices.value(device.deviceId);
                !oldDeviceKeys.deviceId.isEmpty()) // We've seen this device...
            {
                if (const auto keyId = "ed25519:"_L1 + device.deviceId;
                    oldDeviceKeys.keys[keyId] != device.keys[keyId])
                    // ...but its keys that came now are not the same
                {
                    qDebug(E2EE)
                        << "Device reuse detected. Skipping device" << device.userId << device.deviceId;
                    continue;
                }
            }
            if (!selfSigningKey.isEmpty() && !device.signatures[user]["ed25519:"_L1 + selfSigningKey].isEmpty()) {
                if (ed25519VerifySignature(selfSigningKey, toJson(static_cast<const DeviceKeys&>(device)), device.signatures[user]["ed25519:"_L1 + selfSigningKey])) {
                    selfVerifiedDevices[user][device.deviceId] = true;
                    emit q->sessionVerified(user, device.deviceId);
                } else {
                    qCWarning(E2EE) << "failed self signing signature check" << user << device.deviceId;
                }
            }
            deviceKeys[user][device.deviceId] = SLICE(device, DeviceKeys);
        }
        outdatedUsers -= user;
    }
}

void ConnectionEncryptionData::handleQueryKeys(const QueryKeysJob::Response& keys)
{
    database.transaction();
    handleMasterKeys(keys.masterKeys);
    handleSelfSigningKeys(keys.selfSigningKeys);
    handleUserSigningKeys(keys.userSigningKeys);
    checkVerifiedMasterKeys(keys.masterKeys);
    handleDevicesList(keys.deviceKeys);
    database.commit();

    saveDevicesList();

    // A completely faithful code would call std::partition() with bare
    // isKnownCurveKey(), then handleEncryptedToDeviceEvent() on each event
    // with the known key, and then std::erase()... but
    // handleEncryptedToDeviceEvent() doesn't have side effects on the handled
    // events so a small corner-cutting should be fine.
    std::erase_if(pendingEncryptedEvents,
                  [this](const event_ptr_tt<EncryptedEvent>& pendingEvent) {
                      if (!isKnownCurveKey(pendingEvent->senderId(),
                                           pendingEvent->senderKey()))
                          return false;
                      handleEncryptedToDeviceEvent(*pendingEvent);
                      return true;
                  });
}

void ConnectionEncryptionData::encryptionUpdate(const QList<QString>& forUsers)
{
    for (const auto& userId : forUsers)
        if (!trackedUsers.contains(userId)) {
            trackedUsers += userId;
            outdatedUsers += userId;
            encryptionUpdateRequired = true;
        }
}

bool ConnectionEncryptionData::createOlmSession(
    const QString& targetUserId, const QString& targetDeviceId,
    const OneTimeKeys& oneTimeKeyObject)
{
    static QOlmUtility verifier;
    qDebug(E2EE) << "Creating a new session for" << targetUserId
                 << targetDeviceId;
    if (oneTimeKeyObject.isEmpty()) {
        qWarning(E2EE) << "No one time key for" << targetUserId
                       << targetDeviceId;
        return false;
    }
    auto* signedOneTimeKey =
        std::get_if<SignedOneTimeKey>(&*oneTimeKeyObject.begin());
    if (!signedOneTimeKey) {
        qWarning(E2EE) << "No signed one time key for" << targetUserId
                       << targetDeviceId;
        return false;
    }
    // Verify contents of signedOneTimeKey - for that, drop `signatures` and
    // `unsigned` and then verify the object against the respective signature
    const auto signature =
        signedOneTimeKey->signature(targetUserId, targetDeviceId);
    if (!verifier.ed25519Verify(
            q->edKeyForUserDevice(targetUserId, targetDeviceId).toLatin1(),
            signedOneTimeKey->toJsonForVerification(), signature)) {
        qWarning(E2EE) << "Failed to verify one-time-key signature for"
                       << targetUserId << targetDeviceId
                       << ". Skipping this device.";
        return false;
    }
    const auto recipientCurveKey =
        curveKeyForUserDevice(targetUserId, targetDeviceId).toLatin1();
    auto session = olmAccount.createOutboundSession(recipientCurveKey,
                                                    signedOneTimeKey->key());
    if (!session) {
        qCWarning(E2EE) << "Failed to create olm session for "
                        << recipientCurveKey << session.error();
        return false;
    }
    saveSession(*session, recipientCurveKey);
    olmSessions[recipientCurveKey].push_back(std::move(*session));
    return true;
}

std::pair<QOlmMessage::Type, QByteArray>
ConnectionEncryptionData::olmEncryptMessage(const QString& userId,
                                            const QString& device,
                                            const QByteArray& message) const
{
    const auto& curveKey = curveKeyForUserDevice(userId, device).toLatin1();
    const auto& olmSession = olmSessions.at(curveKey).front();
    const auto result = olmSession.encrypt(message);
    database.updateOlmSession(curveKey, olmSession);
    return { result.type(), result.toCiphertext() };
}

QJsonObject ConnectionEncryptionData::assembleEncryptedContent(
    QJsonObject payloadJson, const QString& targetUserId,
    const QString& targetDeviceId) const
{
    payloadJson.insert(SenderKey, q->userId());
    payloadJson.insert("keys"_L1,
                       QJsonObject{
                           { Ed25519Key, olmAccount.identityKeys().ed25519 } });
    payloadJson.insert("recipient"_L1, targetUserId);
    payloadJson.insert(
        "recipient_keys"_L1,
        QJsonObject{ { Ed25519Key,
                       q->edKeyForUserDevice(targetUserId, targetDeviceId) } });
    const auto [type, cipherText] = olmEncryptMessage(
        targetUserId, targetDeviceId,
        QJsonDocument(payloadJson).toJson(QJsonDocument::Compact));
    QJsonObject encrypted{
        { curveKeyForUserDevice(targetUserId, targetDeviceId),
          QJsonObject{ { "type"_L1, type },
                       { "body"_L1, QString::fromLatin1(cipherText) } } }
    };
    return EncryptedEvent(encrypted, olmAccount.identityKeys().curve25519)
        .contentJson();
}

std::pair<QByteArray, QByteArray> doDecryptMessage(const QOlmSession& session,
                                                   const QOlmMessage& message,
                                                   auto&& andThen)
{
    const auto expectedMessage = session.decrypt(message);
    if (expectedMessage) {
        const auto result =
            std::make_pair(*expectedMessage, session.sessionId());
        andThen();
        return result;
    }
    const auto errorLine = message.type() == QOlmMessage::PreKey
                               ? "Failed to decrypt prekey message:"
                               : "Failed to decrypt message:";
    qCDebug(E2EE) << errorLine << expectedMessage.error();
    return {};
}

std::pair<QByteArray, QByteArray> ConnectionEncryptionData::sessionDecryptMessage(
    const QJsonObject& personalCipherObject, const QByteArray& senderKey)
{
    const auto msgType = static_cast<QOlmMessage::Type>(
        personalCipherObject.value(TypeKey).toInt(-1));
    if (msgType != QOlmMessage::General && msgType != QOlmMessage::PreKey) {
        qCWarning(E2EE) << "Olm message has incorrect type" << msgType;
        return {};
    }
    const QOlmMessage message{
        personalCipherObject.value(BodyKey).toString().toLatin1(), msgType
    };
    for (const auto& session : olmSessions[senderKey])
        if (msgType == QOlmMessage::General
            || session.matchesInboundSessionFrom(senderKey, message)) {
            return doDecryptMessage(session, message, [this, &session] {
                q->database()->setOlmSessionLastReceived(
                    session.sessionId(), QDateTime::currentDateTime());
            });
        }

    if (msgType == QOlmMessage::General) {
        qCWarning(E2EE) << "Failed to decrypt message";
        return {};
    }

    qCDebug(E2EE) << "Creating new inbound session"; // Pre-key messages only
    auto newSessionResult =
        olmAccount.createInboundSessionFrom(senderKey, message);
    if (!newSessionResult) {
        qCWarning(E2EE) << "Failed to create inbound session for" << senderKey;
        return {};
    }
    auto&& newSession = std::move(*newSessionResult);
    if (olmAccount.removeOneTimeKeys(newSession) != OLM_SUCCESS) {
        qWarning(E2EE) << "Failed to remove one time key for session"
                       << newSession.sessionId();
        // Keep going though
    }
    return doDecryptMessage(newSession, message, [this, &senderKey, &newSession] {
        saveSession(newSession, senderKey);
        olmSessions[senderKey].push_back(std::move(newSession));
    });
}

std::pair<EventPtr, QByteArray> ConnectionEncryptionData::sessionDecryptMessage(
    const EncryptedEvent& encryptedEvent)
{
    if (encryptedEvent.algorithm() != OlmV1Curve25519AesSha2AlgoKey)
        return {};

    const auto identityKey = olmAccount.identityKeys().curve25519;
    const auto personalCipherObject = encryptedEvent.ciphertext(identityKey);
    if (personalCipherObject.isEmpty()) {
        qDebug(E2EE) << "Encrypted event is not for the current device";
        return {};
    }
    const auto [decrypted, olmSessionId] =
        sessionDecryptMessage(personalCipherObject,
                              encryptedEvent.senderKey().toLatin1());
    if (decrypted.isEmpty()) {
        qDebug(E2EE) << "Problem with new session from senderKey:"
                     << encryptedEvent.senderKey()
                     << olmAccount.oneTimeKeys().keys;

        auto query = database.prepareQuery(
            "SELECT deviceId FROM tracked_devices WHERE curveKey=:curveKey;"_L1);
        query.bindValue(":curveKey"_L1, encryptedEvent.senderKey());
        database.execute(query);
        if (!query.next()) {
            qCWarning(E2EE) << "Unknown device while trying to recover from broken olm session";
            return {};
        }
        auto senderId = encryptedEvent.senderId();
        auto deviceId = query.value("deviceId"_L1).toString();
        QHash<QString, QHash<QString, QString>> hash{ { encryptedEvent.senderId(),
                                                        { { deviceId, "signed_curve25519"_L1 } } } };
        q->callApi<ClaimKeysJob>(hash).then(q, [this, deviceId, senderId](const auto* job) {
            if (triedDevices.contains({ senderId, deviceId })) {
                return;
            }
            triedDevices += { senderId, deviceId };
            qDebug(E2EE) << "Sending dummy event to" << senderId << deviceId;
            createOlmSession(senderId, deviceId, job->oneTimeKeys()[senderId][deviceId]);
            q->sendToDevice(senderId, deviceId, DummyEvent(), true);
        });
        return {};
    }

    auto&& decryptedEvent =
        fromJson<EventPtr>(QJsonDocument::fromJson(decrypted));

    if (auto sender = decryptedEvent->fullJson()[SenderKey].toString();
        sender != encryptedEvent.senderId()) {
        qWarning(E2EE) << "Found user" << sender << "instead of sender" << encryptedEvent.senderId()
                       << "in Olm plaintext";
        return {};
    }

    auto query = database.prepareQuery(QStringLiteral(
        "SELECT edKey FROM tracked_devices WHERE curveKey=:curveKey;"));
    const auto senderKey = encryptedEvent.contentPart<QString>(SenderKeyKey);
    query.bindValue(":curveKey"_L1, senderKey);
    database.execute(query);
    if (!query.next()) {
        qWarning(E2EE) << "Received olm message from unknown device" << senderKey;
        return {};
    }
    if (auto edKey = decryptedEvent->fullJson()["keys"_L1][Ed25519Key].toString();
        edKey.isEmpty() || query.value("edKey"_L1).toString() != edKey) //
    {
        qDebug(E2EE) << "Received olm message with invalid ed key";
        return {};
    }

    // TODO: keys to constants
    const auto decryptedEventObject = decryptedEvent->fullJson();
    if (const auto recipient =
            decryptedEventObject.value("recipient"_L1).toString();
        recipient != q->userId()) //
    {
        qDebug(E2EE) << "Found user" << recipient << "instead of" << q->userId()
                     << "in Olm plaintext";
        return {};
    }
    if (const auto ourKey =
            decryptedEventObject["recipient_keys"_L1][Ed25519Key].toString();
        ourKey != olmAccount.identityKeys().ed25519) //
    {
        qDebug(E2EE) << "Found key" << ourKey
                     << "instead of our own ed25519 key in Olm plaintext";
        return {};
    }

    return { std::move(decryptedEvent), olmSessionId };
}

void ConnectionEncryptionData::doSendSessionKeyToDevices(
    const QString& roomId, const QByteArray& sessionId,
    const QByteArray& sessionKey, uint32_t messageIndex,
    const QMultiHash<QString, QString>& devices)
{
    qDebug(E2EE) << "Sending room key to devices:" << sessionId << messageIndex;
    QHash<QString, QHash<QString, QString>> hash;
    for (const auto& [userId, deviceId] : devices.asKeyValueRange())
        if (!hasOlmSession(userId, deviceId)) {
            hash[userId].insert(deviceId, "signed_curve25519"_L1);
            qDebug(E2EE) << "Adding" << userId << deviceId
                         << "to keys to claim";
        }

    const auto sendKey = [devices, this, sessionId, messageIndex, sessionKey,
                          roomId] {
        QHash<QString, QHash<QString, QJsonObject>> usersToDevicesToContent;
        for (const auto& [targetUserId, targetDeviceId] : devices.asKeyValueRange()) {
            if (!hasOlmSession(targetUserId, targetDeviceId))
                continue;

            // Noisy and leaks the key to logs but nice for debugging
//            qDebug(E2EE) << "Creating the payload for" << targetUserId
//                         << targetDeviceId << sessionId << sessionKey.toHex();
            const auto keyEventJson =
                RoomKeyEvent(MegolmV1AesSha2AlgoKey, roomId,
                             QString::fromLatin1(sessionId),
                             QString::fromLatin1(sessionKey))
                    .fullJson();

            usersToDevicesToContent[targetUserId][targetDeviceId] =
                assembleEncryptedContent(keyEventJson, targetUserId,
                                         targetDeviceId);
        }
        if (!usersToDevicesToContent.empty()) {
            q->sendToDevices(EncryptedEvent::TypeId, usersToDevicesToContent);
            QVector<std::tuple<QString, QString, QString>> receivedDevices;
            receivedDevices.reserve(devices.size());
            for (const auto& [user, device] : devices.asKeyValueRange())
                receivedDevices.push_back(
                    { user, device, curveKeyForUserDevice(user, device) });

            database.setDevicesReceivedKey(roomId, receivedDevices,
                                           sessionId, messageIndex);
        }
    };

    if (hash.isEmpty()) {
        sendKey();
        return;
    }

    q->callApi<ClaimKeysJob>(hash).then(q, [this, sendKey](const ClaimKeysJob* job) {
        for (const auto& [userId, userDevices] : job->oneTimeKeys().asKeyValueRange())
            for (const auto& [deviceId, keys] : userDevices.asKeyValueRange())
                createOlmSession(userId, deviceId, keys);

        sendKey();
    });
}

void ConnectionEncryptionData::sendSessionKeyToDevices(
    const QString& roomId, const QOlmOutboundGroupSession& outboundSession,
    const QMultiHash<QString, QString>& devices)
{
    const auto& sessionId = outboundSession.sessionId();
    const auto& sessionKey = outboundSession.sessionKey();
    const auto& index = outboundSession.sessionMessageIndex();

    const auto closure = [this, roomId, sessionId, sessionKey, index, devices] {
        doSendSessionKeyToDevices(roomId, sessionId, sessionKey, index, devices);
    };
    if (currentQueryKeysJob != nullptr) {
        currentQueryKeysJob = currentQueryKeysJob.onResult(q, closure);
    } else
        closure();
}

ConnectionEncryptionData::ConnectionEncryptionData(Connection* connection,
                                                   PicklingKey&& picklingKey)
    : q(connection)
    , olmAccount(q->userId(), q->deviceId())
    , database(q->userId(), q->deviceId(), std::move(picklingKey))
    , olmSessions(database.loadOlmSessions())
{
    QObject::connect(&olmAccount, &QOlmAccount::needsSave, q,
                     [this] { saveOlmAccount(); });
}

void ConnectionEncryptionData::saveOlmAccount()
{
    qCDebug(E2EE) << "Saving olm account";
    database.storeOlmAccount(olmAccount);
}

void ConnectionEncryptionData::reloadDevices()
{
    outdatedUsers = trackedUsers;
    loadOutdatedUserDevices();
}

bool ConnectionEncryptionData::hasConflictingDeviceIdsAndCrossSigningKeys(const QString& userId)
{
    auto devices = q->devicesForUser(userId);

    auto selfQuery = database.prepareQuery("SELECT key FROM self_signing_keys WHERE userId=:userId;"_L1);
    selfQuery.bindValue(":userId"_L1, userId);
    database.execute(selfQuery);
    if (selfQuery.next() && devices.contains(selfQuery.value("key"_L1).toString()))
        return true;

    if (devices.contains(q->masterKeyForUser(userId)))
        return true;

    auto userQuery = database.prepareQuery("SELECT key FROM user_signing_keys WHERE userId=:userId;"_L1);
    userQuery.bindValue(":userId"_L1, userId);
    database.execute(userQuery);
    return userQuery.next() && devices.contains(userQuery.value("key"_L1).toString());
}
