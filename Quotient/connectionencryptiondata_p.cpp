#include "connectionencryptiondata_p.h"

#include "room.h"
#include "syncdata.h"
#include "user.h"
#include "qt_connection_util.h"

#include "e2ee/qolmutility.h"

#include "events/encryptedevent.h"
#include "events/roomkeyevent.h"

#if QT_VERSION_MAJOR >= 6
#    include <qt6keychain/keychain.h>
#else
#    include <qt5keychain/keychain.h>
#endif

#include <QtCore/QCoreApplication>

using namespace Quotient;
using namespace Quotient::_impl;

Expected<PicklingKey, QKeychain::Error> setupPicklingKey(const QString& id,
                                                         bool mock)
{
    if (mock) {
        qInfo(E2EE) << "Using a mock pickling key";
        return PicklingKey::mock();
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

Omittable<std::unique_ptr<ConnectionEncryptionData>>
ConnectionEncryptionData::setup(Connection* connection, bool mock)
{
    if (auto&& maybePicklingKey = setupPicklingKey(connection->userId(), mock)) {
        auto&& encryptionData = std::make_unique<ConnectionEncryptionData>(
            connection, std::move(*maybePicklingKey));
        if (const auto outcome = encryptionData->database.setupOlmAccount(
                encryptionData->olmAccount)) {
            // account already existing or there's an error unpickling it
            if (outcome == OLM_SUCCESS)
                return std::move(encryptionData);

            qCritical(E2EE) << "Could not unpickle Olm account for"
                            << connection->objectName();
            return none;
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
    return none;
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
        "(matrixId, deviceId, curveKeyId, curveKey, edKeyId, edKey, verified) "
        "SELECT :matrixId, :deviceId, :curveKeyId, :curveKey, :edKeyId, "
        ":edKey, :verified WHERE NOT EXISTS(SELECT 1 FROM tracked_devices "
        "WHERE matrixId=:matrixId AND deviceId=:deviceId);"));
    for (const auto& [user, devices] : asKeyValueRange(deviceKeys)) {
        for (const auto& device : devices) {
            auto keys = device.keys.keys();
            auto curveKeyId = keys[0].startsWith("curve"_ls) ? keys[0]
                                                             : keys[1];
            auto edKeyId = keys[0].startsWith("ed"_ls) ? keys[0] : keys[1];

            query.bindValue(":matrixId"_ls, user);
            query.bindValue(":deviceId"_ls, device.deviceId);
            query.bindValue(":curveKeyId"_ls, curveKeyId);
            query.bindValue(":curveKey"_ls, device.keys[curveKeyId]);
            query.bindValue(":edKeyId"_ls, edKeyId);
            query.bindValue(":edKey"_ls, device.keys[edKeyId]);
            // If the device gets saved here, it can't be verified
            query.bindValue(":verified"_ls, false);

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
            handleQueryKeys(queryKeysJob);
        }
        emit q->finishedQueryingKeys();
    });
}

void ConnectionEncryptionData::consumeToDeviceEvents(Events&& toDeviceEvents)
{
    if (!toDeviceEvents.empty()) {
        qCDebug(E2EE) << "Consuming" << toDeviceEvents.size()
                      << "to-device events";
        for (auto&& tdEvt : toDeviceEvents) {
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
        [](const KeyVerificationDoneEvent&) { return true; },
        [this](const KeyVerificationEvent& kvEvt) {
            if (auto* const session =
                    verificationSessions.value(kvEvt.transactionId())) {
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
        qCWarning(E2EE) << "Failed to decrypt event" << event.id();
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
        [](const Event& evt) {
            qCWarning(E2EE) << "Skipping encrypted to_device event, type"
                            << evt.matrixType();
        });
}

void ConnectionEncryptionData::handleQueryKeys(const QueryKeysJob* job)
{
    for (const auto& [user, keys] : asKeyValueRange(job->deviceKeys())) {
        const QHash<QString, Quotient::DeviceKeys> oldDevices = deviceKeys[user];
        deviceKeys[user].clear();
        for (const auto& device : keys) {
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
                                  "Skipping this device";
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
                        << "Device reuse detected. Skipping this device";
                    continue;
                }
            }
            deviceKeys[user][device.deviceId] = SLICE(device, DeviceKeys);
        }
        outdatedUsers -= user;
    }
    saveDevicesList();

    // A completely faithful code would call std::partition() with bare
    // isKnownCurveKey(), then handleEncryptedToDeviceEvent() on each event
    // with the known key, and then std::erase()... but
    // handleEncryptedToDeviceEvent() doesn't have side effects on the handled
    // events so a small corner-cutting should be fine.
    std::erase_if(pendingEncryptedEvents,
                  [this](const event_ptr_tt<EncryptedEvent>& pendingEvent) {
                      if (!isKnownCurveKey(
                              pendingEvent->fullJson()[SenderKey].toString(),
                              pendingEvent->contentPart<QString>(SenderKeyKey)))
                          return false;
                      handleEncryptedToDeviceEvent(*pendingEvent);
                      return true;
                  });
}

void ConnectionEncryptionData::encryptionUpdate(const QList<User*>& forUsers)
{
    for (const auto& user : forUsers)
        if (!trackedUsers.contains(user->id())) {
            trackedUsers += user->id();
            outdatedUsers += user->id();
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
        qCWarning(E2EE) << "Failed to create inbound session for" << senderKey
                        << "with error" << newSessionResult.error();
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
    for (const auto& [userId, deviceId] : asKeyValueRange(devices))
        if (!hasOlmSession(userId, deviceId)) {
            hash[userId].insert(deviceId, "signed_curve25519"_ls);
            qDebug(E2EE) << "Adding" << userId << deviceId
                         << "to keys to claim";
        }

    const auto sendKey = [devices, this, sessionId, messageIndex, sessionKey,
                          roomId] {
        QHash<QString, QHash<QString, QJsonObject>> usersToDevicesToContent;
        for (const auto& [targetUserId, targetDeviceId] :
             asKeyValueRange(devices)) {
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
            for (const auto& [user, device] : asKeyValueRange(devices))
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
        for (const auto oneTimeKeys = job->oneTimeKeys();
             const auto& [userId, userDevices] : asKeyValueRange(oneTimeKeys)) {
            for (const auto& [deviceId, keys] : asKeyValueRange(userDevices)) {
                createOlmSession(userId, deviceId, keys);
            }
        }
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
