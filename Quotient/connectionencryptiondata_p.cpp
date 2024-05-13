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

using namespace Quotient;
using namespace Quotient::_impl;

Expected<PicklingKey, QKeychain::Error> setupPicklingKey(const QString& id,
                                                         bool mock)
{
    if (mock) {
        qInfo(E2EE) << "Using a mock pickling key";
        return PicklingKey::generate();
    }

    // TODO: Rewrite the whole thing in an async way to get rid of nested event
    // loops
    using namespace QKeychain;
    const auto keychainId = id + "-Pickle"_ls;
    ReadPasswordJob readJob(qAppName());
    readJob.setAutoDelete(false);
    readJob.setKey(keychainId);
    QEventLoop readLoop;
    QObject::connect(&readJob, &Job::finished, &readLoop, &QEventLoop::quit);
    readJob.start();
    readLoop.exec();

    if (readJob.error() == Error::NoError) {
        auto&& data = readJob.binaryData();
        if (data.size() == PicklingKey::extent) {
            qDebug(E2EE) << "Successfully loaded pickling key from keychain";
            return PicklingKey::fromByteArray(std::move(data));
        }
        qCritical(E2EE) << "The loaded pickling key for" << id
                        << "has length" << data.size()
                        << "but the library expected" << PicklingKey::extent;
        return Error::OtherError;
    }
    if (readJob.error() == Error::EntryNotFound) {
        auto&& picklingKey = PicklingKey::generate();
        WritePasswordJob writeJob(qAppName());
        writeJob.setAutoDelete(false);
        writeJob.setKey(keychainId);
        writeJob.setBinaryData(picklingKey.viewAsByteArray());
        QEventLoop writeLoop;
        QObject::connect(&writeJob, &Job::finished, &writeLoop,
                         &QEventLoop::quit);
        writeJob.start();
        writeLoop.exec();

        if (writeJob.error() == Error::NoError)
            return std::move(picklingKey);

        qCritical(E2EE) << "Could not save pickling key to keychain: "
                        << writeJob.errorString();
        return writeJob.error();
    }
    qWarning(E2EE) << "Error loading pickling key - please fix your keychain:"
                   << readJob.errorString();
    return readJob.error();
}

std::optional<std::unique_ptr<ConnectionEncryptionData>>
ConnectionEncryptionData::setup(Connection* connection, bool mock)
{
    if (auto&& maybePicklingKey = setupPicklingKey(connection->userId(), mock)) {
        auto&& encryptionData = std::make_unique<ConnectionEncryptionData>(
            connection, std::move(*maybePicklingKey));
        if (mock) {
            encryptionData->database.clear();
            encryptionData->olmAccount.setupNewAccount();
            return std::move(encryptionData);
        }
        if (const auto outcome = encryptionData->database.setupOlmAccount(
                encryptionData->olmAccount)) {
            // account already existing or there's an error unpickling it
            if (outcome == OLM_SUCCESS)
                return std::move(encryptionData);

            qCritical(E2EE) << "Could not unpickle Olm account for"
                            << connection->objectName();
            return {};
        }
        // A new account has been created
        auto job = connection->callApi<UploadKeysJob>(
            encryptionData->olmAccount.deviceKeys());
        // eData is meant to have the same scope as connection so it's safe
        // to pass an unguarded pointer to encryption data here
        QObject::connect(job, &BaseJob::success, connection,
                         [connection, eData = encryptionData.get()] {
                             eData->trackedUsers += connection->userId();
                             eData->outdatedUsers += connection->userId();
                             eData->encryptionUpdateRequired = true;
                         });
        QObject::connect(job, &BaseJob::failure, connection, [job] {
            qCWarning(E2EE)
                << "Failed to upload device keys:" << job->errorString();
        });
        return std::move(encryptionData);
    }
    qCritical(E2EE) << "Could not load or initialise a pickling key for"
                    << connection->objectName();
    return {};
}

void ConnectionEncryptionData::saveDevicesList()
{
    database.transaction();
    auto query =
        database.prepareQuery(QStringLiteral("DELETE FROM tracked_users"));
    database.execute(query);
    query.prepare(QStringLiteral(
        "INSERT INTO tracked_users(matrixId) VALUES(:matrixId);"));
    for (const auto& user : trackedUsers) {
        query.bindValue(":matrixId"_ls, user);
        database.execute(query);
    }

    query.prepare(QStringLiteral("DELETE FROM outdated_users"));
    database.execute(query);
    query.prepare(QStringLiteral(
        "INSERT INTO outdated_users(matrixId) VALUES(:matrixId);"));
    for (const auto& user : outdatedUsers) {
        query.bindValue(":matrixId"_ls, user);
        database.execute(query);
    }

    query.prepare(QStringLiteral(
        "INSERT INTO tracked_devices"
        "(matrixId, deviceId, curveKeyId, curveKey, edKeyId, edKey, verified, selfVerified) "
        "VALUES (:matrixId, :deviceId, :curveKeyId, :curveKey, :edKeyId, :edKey, :verified, :selfVerified);"));
    for (const auto& [user, devices] : deviceKeys.asKeyValueRange()) {
        auto deleteQuery = database.prepareQuery(
            QStringLiteral("DELETE FROM tracked_devices WHERE matrixId=:matrixId;"));
        deleteQuery.bindValue(":matrixId"_ls, user);
        database.execute(deleteQuery);
        for (const auto& device : devices) {
            const auto keys = device.keys.asKeyValueRange();
            deleteQuery.prepare(
                "DELETE FROM tracked_devices WHERE matrixId=:matrixId AND deviceId=:deviceId;"_ls);
            deleteQuery.bindValue(":matrixId"_ls, user);
            deleteQuery.bindValue(":deviceId"_ls, device.deviceId);
            database.execute(deleteQuery);

            const auto curveKeyIt = std::ranges::find_if(keys, [](const auto& p) {
                return p.first.startsWith("curve"_ls);
            });
            Q_ASSERT(curveKeyIt != keys.end());
            const auto edKeyIt = std::ranges::find_if(keys, [](const auto& p) {
                return p.first.startsWith("ed"_ls);
            });
            Q_ASSERT(edKeyIt != keys.end());

            query.bindValue(":matrixId"_ls, user);
            query.bindValue(":deviceId"_ls, device.deviceId);
            query.bindValue(":curveKeyId"_ls, curveKeyIt->first);
            query.bindValue(":curveKey"_ls, curveKeyIt->second);
            query.bindValue(":edKeyId"_ls, edKeyIt->first);
            query.bindValue(":edKey"_ls, edKeyIt->second);
            // If the device gets saved here, it can't be verified
            query.bindValue(":verified"_ls, verifiedDevices[user][device.deviceId]);
            query.bindValue(":selfVerified"_ls, selfVerifiedDevices[user][device.deviceId]);

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
        deviceKeys[query.value("matrixId"_ls).toString()].insert(
            query.value("deviceId"_ls).toString(),
            {
                .userId = query.value("matrixId"_ls).toString(),
                .deviceId = query.value("deviceId"_ls).toString(),
                .algorithms = Algorithms,
                .keys{ { query.value("curveKeyId"_ls).toString(),
                         query.value("curveKey"_ls).toString() },
                       { query.value("edKeyId"_ls).toString(),
                         query.value("edKey"_ls).toString() } },
                .signatures{} // not needed after initial validation so not saved
            });
        selfVerifiedDevices[query.value("matrixId"_ls).toString()][query.value("deviceId"_ls).toString()] = query.value("selfVerified"_ls).toBool();
        verifiedDevices[query.value("matrixId"_ls).toString()][query.value("deviceId"_ls).toString()] = query.value("verified"_ls).toBool();
    }
}

QString ConnectionEncryptionData::curveKeyForUserDevice(
    const QString& userId, const QString& device) const
{
    return deviceKeys[userId][device].keys["curve25519:"_ls + device];
}

bool ConnectionEncryptionData::isKnownCurveKey(const QString& userId,
                                               const QString& curveKey) const
{
    auto query = database.prepareQuery(
        QStringLiteral("SELECT * FROM tracked_devices WHERE matrixId=:matrixId "
                       "AND curveKey=:curveKey"));
    query.bindValue(":matrixId"_ls, userId);
    query.bindValue(":curveKey"_ls, curveKey);
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
    if(currentQueryKeysJob) {
        currentQueryKeysJob->abandon();
        currentQueryKeysJob = nullptr;
    }
    auto queryKeysJob = q->callApi<QueryKeysJob>(users);
    currentQueryKeysJob = queryKeysJob;
    QObject::connect(queryKeysJob, &BaseJob::result, q, [this, queryKeysJob] {
        currentQueryKeysJob = nullptr;
        if (queryKeysJob->error() == BaseJob::Success) {
            handleQueryKeys(queryKeysJob->jsonData());
        }
        emit q->finishedQueryingKeys();
    });
}

void ConnectionEncryptionData::consumeToDeviceEvents(Events&& toDeviceEvents)
{
    if (!toDeviceEvents.empty()) {
        qCDebug(E2EE) << "Consuming" << toDeviceEvents.size()
                      << "to-device events";
        for (auto&& tdEvt : std::move(toDeviceEvents)) {
            if (processIfVerificationEvent(*tdEvt, false))
                continue;
            if (auto&& event = eventCast<EncryptedEvent>(std::move(tdEvt))) {
                if (event->algorithm() != OlmV1Curve25519AesSha2AlgoKey) {
                    qCDebug(E2EE) << "Unsupported algorithm" << event->id()
                                  << "for event" << event->algorithm();
                    continue;
                }
                if (isKnownCurveKey(event->senderId(), event->senderKey())) {
                    handleEncryptedToDeviceEvent(*event);
                    continue;
                }
                trackedUsers += event->senderId();
                outdatedUsers += event->senderId();
                encryptionUpdateRequired = true;
                pendingEncryptedEvents.push_back(std::move(event));
            }
        }
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

void ConnectionEncryptionData::handleEncryptedToDeviceEvent(
    const EncryptedEvent& event)
{
    const auto [decryptedEvent, olmSessionId] = sessionDecryptMessage(event);
    if (!decryptedEvent) {
        qCWarning(E2EE) << "Failed to decrypt to-device event from device"
                        << event.deviceId();
        return;
    }

    if (processIfVerificationEvent(*decryptedEvent, true))
        return;
    switchOnType(
        *decryptedEvent,
        [this, &event,
         olmSessionId = olmSessionId](const RoomKeyEvent& roomKeyEvent) {
            if (auto* detectedRoom = q->room(roomKeyEvent.roomId())) {
                detectedRoom->handleRoomKeyEvent(roomKeyEvent, event.senderId(),
                                                 olmSessionId);
            } else {
                qCDebug(E2EE)
                    << "Encrypted event room id" << roomKeyEvent.roomId()
                    << "is not found at the connection" << q->objectName();
            }
        },
        [this](const Event& evt) {
            //TODO create an event subclass for this
            if (evt.matrixType() == "m.secret.send"_ls) {
                emit q->secretReceived(evt.contentPart<QString>("request_id"_ls), evt.contentPart<QString>("secret"_ls));
                return;
            }
            qCWarning(E2EE) << "Skipping encrypted to_device event, type"
                            << evt.matrixType();
        });
}

void ConnectionEncryptionData::handleMasterKeys(const QHash<QString, CrossSigningKey>& masterKeys)
{
    for (const auto &[userId, key] : asKeyValueRange(masterKeys)) {
        if (key.userId != userId) {
            qCWarning(E2EE) << "Master key: userId mismatch" << key.userId << userId;
            continue;
        }
        if (!key.usage.contains("master"_ls)) {
            qCWarning(E2EE) << "Master key: invalid usage" << key.usage;
            continue;
        }
        auto checkQuery = database.prepareQuery("SELECT * FROM master_keys WHERE userId=:userId"_ls);
        checkQuery.bindValue(":userId"_ls, key.userId);
        database.execute(checkQuery);
        if (checkQuery.next()) {
            if (checkQuery.value("key"_ls).toString() == key.keys.values()[0]) {
                continue;
            }
            qCWarning(E2EE) << "New master key for" << key.userId;
            database.transaction();
            auto query = database.prepareQuery(
                "UPDATE tracked_devices SET verified=0, selfVerified=0 WHERE matrixId=:matrixId;"_ls);
            query.bindValue(":matrixId"_ls, userId);
            database.execute(query);
            query = database.prepareQuery("DELETE FROM self_signing_keys WHERE userId=:userId;"_ls);
            query.bindValue(":userId"_ls, userId);
            database.execute(query);
            database.commit();
        }

        auto query = database.prepareQuery("DELETE FROM master_keys WHERE userId=:userId;"_ls);
        query.bindValue(":userId"_ls, userId);
        database.execute(query);
        query = database.prepareQuery("INSERT INTO master_keys(userId, key, verified) VALUES(:userId, :key, false);"_ls);
        query.bindValue(":userId"_ls, userId);
        query.bindValue(":key"_ls, key.keys.values()[0]);
        database.execute(query);
    }
}

namespace {
QString getEd25519Signature(const CrossSigningKey& keyObject, const QString& userId,
                            const QString& masterKey)
{
    return keyObject.signatures[userId]["ed25519:"_ls + masterKey].toString();
}
}

void ConnectionEncryptionData::handleSelfSigningKeys(const QHash<QString, CrossSigningKey>& selfSigningKeys)
{
    for (const auto &[userId, key] : asKeyValueRange(selfSigningKeys)) {
        if (key.userId != userId) {
            qCWarning(E2EE) << "Self signing key: userId mismatch"<< key.userId << userId;
            continue;
        }
        if (!key.usage.contains("self_signing"_ls)) {
            qCWarning(E2EE) << "Self signing key: invalid usage" << key.usage;
            continue;
        }
        const auto masterKey = q->masterKeyForUser(userId);
        if (masterKey.isEmpty())
            continue;

        auto checkQuery = database.prepareQuery("SELECT key FROM self_signing_keys WHERE userId=:userId;"_ls);
        checkQuery.bindValue(":userId"_ls, userId);
        database.execute(checkQuery);
        if (checkQuery.next()) {
            auto oldKey = checkQuery.value("key"_ls).toString();
            if (oldKey != key.keys.values()[0]) {
                qCWarning(E2EE) << "New self-signing key for" << userId << ". Marking all devices as unverified.";
                database.transaction();
                auto query = database.prepareQuery(
                    "UPDATE tracked_devices SET verified=0, selfVerified=0 WHERE matrixId=:matrixId;"_ls);
                query.bindValue(":matrixId"_ls, userId);
                database.execute(query);
                database.commit();
            }
        }

        if (!ed25519VerifySignature(masterKey, toJson(key),
                                    getEd25519Signature(key, userId, masterKey))) {
            qCWarning(E2EE) << "Self signing key: failed signature verification" << userId;
            continue;
        }
        auto query = database.prepareQuery("DELETE FROM self_signing_keys WHERE userId=:userId;"_ls);
        query.bindValue(":userId"_ls, userId);
        database.execute(query);
        query = database.prepareQuery("INSERT INTO self_signing_keys(userId, key) VALUES(:userId, :key);"_ls);
        query.bindValue(":userId"_ls, userId);
        query.bindValue(":key"_ls, key.keys.values()[0]);
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
        if (!key.usage.contains("user_signing"_ls)) {
            qWarning() << "User signing key: invalid usage" << key.usage;
            continue;
        }
        const auto masterKey = q->masterKeyForUser(userId);
        if (masterKey.isEmpty())
            continue;

        auto checkQuery = database.prepareQuery("SELECT key FROM user_signing_keys WHERE userId=:userId"_ls);
        checkQuery.bindValue(":userId"_ls, userId);
        database.execute(checkQuery);
        if (checkQuery.next()) {
            auto oldKey = checkQuery.value("key"_ls).toString();
            if (oldKey != key.keys.values()[0]) {
                qCWarning(E2EE) << "New user signing key; marking all master signing keys as unverified" << userId;
                database.transaction();
                auto query = database.prepareQuery(
                    "UPDATE master_keys SET verified=0;"_ls);
                database.execute(query);
                database.commit();
            }
        }

        if (!ed25519VerifySignature(masterKey, toJson(key),
                                    getEd25519Signature(key, userId, masterKey))) {
            qWarning() << "User signing key: failed signature verification" << userId;
            continue;
        }
        auto query = database.prepareQuery("DELETE FROM user_signing_keys WHERE userId=:userId;"_ls);
        query.bindValue(":userId"_ls, userId);
        database.execute(query);
        query = database.prepareQuery("INSERT INTO user_signing_keys(userId, key) VALUES(:userId, :key);"_ls);
        query.bindValue(":userId"_ls, userId);
        query.bindValue(":key"_ls, key.keys.values()[0]);
        database.execute(query);
    }
}

void ConnectionEncryptionData::checkVerifiedMasterKeys(const QHash<QString, CrossSigningKey>& masterKeys)
{
    if (!q->isUserVerified(q->userId())) {
        return;
    }
    auto query = database.prepareQuery("SELECT key FROM user_signing_keys WHERE userId=:userId;"_ls);
    query.bindValue(":userId"_ls, q->userId());
    database.execute(query);
    if (!query.next()) {
        return;
    }
    const auto userSigningKey = query.value("key"_ls).toString();
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

void ConnectionEncryptionData::handleDevicesList(const QHash<QString, QHash<QString, QueryKeysJob::DeviceInformation>>& deviceKeys)
{
    for(const auto &[user, keys] : deviceKeys.asKeyValueRange()) {
        const auto oldDevices = this->deviceKeys[user];
        auto query = database.prepareQuery("SELECT * FROM self_signing_keys WHERE userId=:userId;"_ls);
        query.bindValue(":userId"_ls, user);
        database.execute(query);
        const auto selfSigningKey = query.next() ? query.value("key"_ls).toString() : QString();
        this->deviceKeys[user].clear();
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
                qWarning(E2EE) << "Failed to verify devicekeys signature. "
                                  "Skipping device" << device.userId << device.deviceId;
                continue;
            }
            if (const auto oldDeviceKeys = oldDevices.value(device.deviceId);
                !oldDeviceKeys.deviceId.isEmpty()) // We've seen this device...
            {
                if (const auto keyId = "ed25519:"_ls + device.deviceId;
                    oldDeviceKeys.keys[keyId] != device.keys[keyId])
                    // ...but its keys that came now are not the same
                {
                    qDebug(E2EE)
                        << "Device reuse detected. Skipping device" << device.userId << device.deviceId;
                    continue;
                }
            }
            if (!selfSigningKey.isEmpty() && !device.signatures[user]["ed25519:"_ls + selfSigningKey].isEmpty()) {
                if (ed25519VerifySignature(selfSigningKey, toJson(static_cast<const DeviceKeys&>(device)), device.signatures[user]["ed25519:"_ls + selfSigningKey])) {
                    selfVerifiedDevices[user][device.deviceId] = true;
                    emit q->sessionVerified(user, device.deviceId);
                } else {
                    qCWarning(E2EE) << "failed self signing signature check" << user << device.deviceId;
                }
            }
            this->deviceKeys[user][device.deviceId] = SLICE(device, DeviceKeys);
        }
        outdatedUsers -= user;
    }
}

void ConnectionEncryptionData::handleQueryKeys(const QJsonObject& keysJson)
{
    database.transaction();
    const auto masterKeys = fromJson<QHash<QString, CrossSigningKey>>(keysJson["master_keys"_ls]);
    handleMasterKeys(masterKeys);
    handleSelfSigningKeys(
        fromJson<QHash<QString, CrossSigningKey>>(keysJson["self_signing_keys"_ls]));
    handleUserSigningKeys(
        fromJson<QHash<QString, CrossSigningKey>>(keysJson["user_signing_keys"_ls]));
    checkVerifiedMasterKeys(masterKeys);
    handleDevicesList(fromJson<QHash<QString, QHash<QString, QueryKeysJob::DeviceInformation>>>(
        keysJson["device_keys"_ls]));
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
    payloadJson.insert("keys"_ls,
                       QJsonObject{
                           { Ed25519Key, olmAccount.identityKeys().ed25519 } });
    payloadJson.insert("recipient"_ls, targetUserId);
    payloadJson.insert(
        "recipient_keys"_ls,
        QJsonObject{ { Ed25519Key,
                       q->edKeyForUserDevice(targetUserId, targetDeviceId) } });
    const auto [type, cipherText] = olmEncryptMessage(
        targetUserId, targetDeviceId,
        QJsonDocument(payloadJson).toJson(QJsonDocument::Compact));
    QJsonObject encrypted{
        { curveKeyForUserDevice(targetUserId, targetDeviceId),
          QJsonObject{ { "type"_ls, type },
                       { "body"_ls, QString::fromLatin1(cipherText) } } }
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
            "SELECT deviceId FROM tracked_devices WHERE curveKey=:curveKey;"_ls);
        query.bindValue(":curveKey"_ls, encryptedEvent.senderKey());
        database.execute(query);
        if (!query.next()) {
            qCWarning(E2EE) << "Unknown device while trying to recover from "
                               "broken olm session";
            return {};
        }
        auto senderId = encryptedEvent.senderId();
        auto deviceId = query.value("deviceId"_ls).toString();
        QHash<QString, QHash<QString, QString>> hash{
            { encryptedEvent.senderId(),
              { { deviceId, "signed_curve25519"_ls } } }
        };
        auto job = q->callApi<ClaimKeysJob>(hash);
        QObject::connect(
            job, &BaseJob::finished, q, [this, deviceId, job, senderId] {
                if (triedDevices.contains({ senderId, deviceId })) {
                    return;
                }
                triedDevices += { senderId, deviceId };
                qDebug(E2EE)
                    << "Sending dummy event to" << senderId << deviceId;
                createOlmSession(senderId, deviceId,
                                 job->oneTimeKeys()[senderId][deviceId]);
                q->sendToDevice(senderId, deviceId, DummyEvent(), true);
            });
        return {};
    }

    auto&& decryptedEvent =
        fromJson<EventPtr>(QJsonDocument::fromJson(decrypted));

    if (auto sender = decryptedEvent->fullJson()[SenderKey].toString();
        sender != encryptedEvent.senderId()) {
        qWarning(E2EE) << "Found user" << sender << "instead of sender"
                       << encryptedEvent.senderId() << "in Olm plaintext";
        return {};
    }

    auto query = database.prepareQuery(QStringLiteral(
        "SELECT edKey FROM tracked_devices WHERE curveKey=:curveKey;"));
    const auto senderKey = encryptedEvent.contentPart<QString>(SenderKeyKey);
    query.bindValue(":curveKey"_ls, senderKey);
    database.execute(query);
    if (!query.next()) {
        qWarning(E2EE) << "Received olm message from unknown device"
                       << senderKey;
        return {};
    }
    if (auto edKey =
            decryptedEvent->fullJson()["keys"_ls][Ed25519Key].toString();
        edKey.isEmpty() || query.value("edKey"_ls).toString() != edKey) //
    {
        qDebug(E2EE) << "Received olm message with invalid ed key";
        return {};
    }

    // TODO: keys to constants
    const auto decryptedEventObject = decryptedEvent->fullJson();
    if (const auto recipient =
            decryptedEventObject.value("recipient"_ls).toString();
        recipient != q->userId()) //
    {
        qDebug(E2EE) << "Found user" << recipient << "instead of" << q->userId()
                     << "in Olm plaintext";
        return {};
    }
    if (const auto ourKey =
            decryptedEventObject["recipient_keys"_ls][Ed25519Key].toString();
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
            hash[userId].insert(deviceId, "signed_curve25519"_ls);
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

    auto job = q->callApi<ClaimKeysJob>(hash);
    QObject::connect(job, &BaseJob::success, q, [job, this, sendKey] {
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
        connectSingleShot(q, &Connection::finishedQueryingKeys, q, closure);
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

    auto selfQuery = database.prepareQuery("SELECT key FROM self_signing_keys WHERE userId=:userId;"_ls);
    selfQuery.bindValue(":userId"_ls, userId);
    database.execute(selfQuery);
    if (selfQuery.next()) {
        if (devices.contains(selfQuery.value("key"_ls).toString())) {
            return true;
        }
    }

    auto masterQuery = database.prepareQuery("SELECT key FROM master_keys WHERE userId=:userId;"_ls);
    masterQuery.bindValue(":userId"_ls, userId);
    database.execute(masterQuery);
    if (masterQuery.next()) {
        if (devices.contains(masterQuery.value("key"_ls).toString())) {
            return true;
        }
    }

    auto userQuery = database.prepareQuery("SELECT key FROM user_signing_keys WHERE userId=:userId;"_ls);
    userQuery.bindValue(":userId"_ls, userId);
    database.execute(userQuery);
    if (userQuery.next()) {
        if (devices.contains(userQuery.value("key"_ls).toString())) {
            return true;
        }
    }
    return false;
}
