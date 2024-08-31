// SPDX-FileCopyrightText: 2022 Tobias Fella <fella@posteo.de>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "keyverificationsession.h"

#include "connection.h"
#include "database.h"
#include "logging_categories_p.h"
#include "csapi/cross_signing.h"
#include "room.h"

#include "e2ee/cryptoutils.h"
#include "e2ee/sssshandler.h"
#include "e2ee/qolmaccount.h"

#include "events/event.h"

#include <QtCore/QCryptographicHash>
#include <QtCore/QTimer>
#include <QtCore/QUuid>

#include <chrono>

using namespace Quotient;
using namespace std::chrono;

const QStringList supportedMethods = { SasV1Method };

QStringList commonSupportedMethods(const QStringList& remoteMethods)
{
    QStringList result;
    for (const auto& method : remoteMethods) {
        if (supportedMethods.contains(method)) {
            result += method;
        }
    }
    return result;
}

KeyVerificationSession::KeyVerificationSession(QString remoteUserId,
                                               const KeyVerificationRequestEvent& event,
                                               Connection* connection, bool encrypted)
    : KeyVerificationSession(std::move(remoteUserId), connection, event.fromDevice(), encrypted,
                             event.methods(), event.timestamp(), event.transactionId())
{}

KeyVerificationSession::KeyVerificationSession(const RoomMessageEvent* event, Room* room)
    : KeyVerificationSession(event->senderId(), room->connection(),
                             event->contentPart<QString>("from_device"_L1), room->usesEncryption(),
                             event->contentPart<QStringList>("methods"_L1),
                             event->originTimestamp(), {}, room, event->id())
{}

KeyVerificationSession::KeyVerificationSession(QString remoteUserId, Connection* connection,
                                               QString remoteDeviceId, bool encrypted,
                                               QStringList methods, QDateTime startTimestamp,
                                               QString transactionId, Room* room,
                                               QString requestEventId)
    : QObject(connection)
    , m_connection(connection)
    , m_room(room)
    , m_remoteUserId(std::move(remoteUserId))
    , m_remoteDeviceId(std::move(remoteDeviceId))
    , m_transactionId(std::move(transactionId))
    , m_encrypted(encrypted)
    , m_remoteSupportedMethods(std::move(methods))
    , m_sas(sas::new_sas())
    , m_requestEventId(std::move(requestEventId)) // TODO: Consider merging with transactionId
{
    if (m_connection->hasConflictingDeviceIdsAndCrossSigningKeys(m_remoteUserId)) {
        qCWarning(E2EE) << "Remote user has conflicting device ids and cross signing keys; refusing to verify.";
        return;
    }
    const auto& currentTime = QDateTime::currentDateTime();
    const auto timeoutTime =
        std::min(startTimestamp.addSecs(600), currentTime.addSecs(120));
    const milliseconds timeout{ currentTime.msecsTo(timeoutTime) };
    if (timeout > 5s) {
        setupTimeout(timeout);
    } else {
        // Otherwise don't even bother starting up
        deleteLater();
    }
}

KeyVerificationSession::KeyVerificationSession(QString userId, QString deviceId,
                                               Connection* connection)
    : KeyVerificationSession(std::move(userId), connection, nullptr, std::move(deviceId),
                             QUuid::createUuid().toString())
{}

KeyVerificationSession::KeyVerificationSession(Room* room)
    : KeyVerificationSession(room->members()[room->members()[0].isLocalMember() ? 1 : 0].id(),
                             room->connection(),
                             room)
{}

KeyVerificationSession::KeyVerificationSession(QString remoteUserId, Connection* connection,
                                               Room* room, QString remoteDeviceId,
                                               QString transactionId)
    : QObject(connection)
    , m_connection(connection)
    , m_room(room)
    , m_remoteUserId(std::move(remoteUserId))
    , m_remoteDeviceId(std::move(remoteDeviceId))
    , m_transactionId(std::move(transactionId))
    , m_sas(sas::new_sas())
{
    if (m_connection->hasConflictingDeviceIdsAndCrossSigningKeys(m_remoteUserId)) {
        qCWarning(E2EE) << "Remote user has conflicting device ids and cross signing keys; refusing to verify.";
        return;
    }
    setupTimeout(600s);
    QMetaObject::invokeMethod(this, &KeyVerificationSession::sendRequest);
}

void KeyVerificationSession::setupTimeout(milliseconds timeout)
{
    QTimer::singleShot(timeout, this, [this] { cancelVerification(TIMEOUT); });
}

void KeyVerificationSession::handleEvent(const KeyVerificationEvent& baseEvent)
{
    if (!switchOnType(
            baseEvent,
            [this](const KeyVerificationCancelEvent& event) {
                setError(stringToError(event.code()));
                setState(CANCELED);
                return true;
            },
            [this](const KeyVerificationStartEvent& event) {
                if (state() != WAITINGFORREADY && state() != READY && state() != WAITINGFORACCEPT)
                    return false;
                handleStart(event);
                return true;
            },
            [this](const KeyVerificationReadyEvent& event) {
                if (state() == WAITINGFORREADY)
                    handleReady(event);
                // ACCEPTED is also fine here because it's possible to receive
                // ready and start in the same sync, in which case start might
                // be handled before ready.
                return state() == READY || state() == WAITINGFORACCEPT || state() == ACCEPTED;
            },
            [this](const KeyVerificationAcceptEvent& event) {
                if (state() != WAITINGFORACCEPT)
                    return false;
                const auto& theirMac = event.messageAuthenticationCode();
                for (const auto& mac : SupportedMacs) {
                    if (mac == theirMac) {
                        m_commonMacCodes.push_back(theirMac);
                    }
                }
                if (m_commonMacCodes.isEmpty()) {
                    cancelVerification(UNKNOWN_METHOD);
                    return false;
                }
                m_commitment = event.commitment();
                sendKey();
                setState(WAITINGFORKEY);
                return true;
            },
            [this](const KeyVerificationKeyEvent& event) {
                if (state() != ACCEPTED && state() != WAITINGFORKEY)
                    return false;
                handleKey(event);
                return true;
            },
            [this](const KeyVerificationMacEvent& event) {
                if (state() != WAITINGFORMAC && state() != WAITINGFORVERIFICATION)
                    return false;
                handleMac(event);
                return true;
            },
            [this](const KeyVerificationDoneEvent&) { return state() == DONE; }))
        cancelVerification(UNEXPECTED_MESSAGE);
}

struct EmojiStoreEntry : EmojiEntry {
    QHash<QString, QString> translatedDescriptions;

    explicit EmojiStoreEntry(const QJsonObject& json)
        : EmojiEntry{ fromJson<QString>(json["emoji"_L1]),
                      fromJson<QString>(json["description"_L1]) }
        , translatedDescriptions{ fromJson<QHash<QString, QString>>(
              json["translated_descriptions"_L1]) }
    {}
};

using EmojiStore = QVector<EmojiStoreEntry>;

EmojiStore loadEmojiStore()
{
    Q_INIT_RESOURCE(libquotientemojis);
    QFile dataFile(":/sas-emoji.json"_L1);
    dataFile.open(QFile::ReadOnly);
    auto data = dataFile.readAll();
    Q_CLEANUP_RESOURCE(libquotientemojis);
    return fromJson<EmojiStore>(
        QJsonDocument::fromJson(data).array());
}

EmojiEntry emojiForCode(int code, const QString& language)
{
    static const EmojiStore emojiStore = loadEmojiStore();
    const auto& entry = emojiStore[code];
    if (!language.isEmpty())
        if (const auto translatedDescription =
            emojiStore[code].translatedDescriptions.value(language);
            !translatedDescription.isNull())
            return { entry.emoji, translatedDescription };

    return SLICE(entry, EmojiEntry);
}

void KeyVerificationSession::handleKey(const KeyVerificationKeyEvent& event)
{
    auto eventKey = event.key().toLatin1();
    auto publicKeyResult = types::curve25519_public_key_from_base64(rust::String(eventKey.data(), eventKey.size()));
    //TODO error handling

    m_establishedSas = m_sas->diffie_hellman(*curve25519_public_key_from_base64_result_value(std::move(publicKeyResult)));

    if (startSentByUs) {
        const auto paddedCommitment =
            QCryptographicHash::hash((event.key() % m_startEvent).toLatin1(),
                                     QCryptographicHash::Sha256)
                .toBase64();
        const QLatin1String unpaddedCommitment(paddedCommitment.constData(),
                                               QString::fromLatin1(paddedCommitment).indexOf(u'='));
        if (unpaddedCommitment != m_commitment) {
            qCWarning(E2EE) << "Commitment mismatch; aborting verification";
            cancelVerification(MISMATCHED_COMMITMENT);
            return;
        }
    } else {
        sendKey();
    }

    auto ourPublicKey = m_sas->public_key()->to_base64();

    const auto infoTemplate =
        startSentByUs ? "MATRIX_KEY_VERIFICATION_SAS|%1|%2|%3|%4|%5|%6|%7"_L1
                      : "MATRIX_KEY_VERIFICATION_SAS|%4|%5|%6|%1|%2|%3|%7"_L1;

    const auto info = infoTemplate
                          .arg(m_connection->userId(), m_connection->deviceId(),
                               QString::fromLatin1({ourPublicKey.data(), (int) ourPublicKey.length()}), m_remoteUserId, m_remoteDeviceId,
                               event.key(), m_room ? m_requestEventId : m_transactionId)
                          .toLatin1();

    auto emojiIndices = (*m_establishedSas)->bytes(rust::String(info.data(), info.size()))->emoji_indices();


    const auto uiLanguages = QLocale().uiLanguages();
    const auto preferredLanguage = uiLanguages.isEmpty()
                                       ? QString()
                                       : uiLanguages.front().section(u'-', 0, 0);
    for (const auto& c : emojiIndices)
        m_sasEmojis += emojiForCode(c, preferredLanguage);

    emit sasEmojisChanged();
    emit keyReceived();
    setState(WAITINGFORVERIFICATION);
}

//TODO use verify_mac
QString KeyVerificationSession::calculateMac(const QString& input,
                                             bool verifying,
                                             const QString& keyId)
{
    const auto inputBytes = input.toLatin1();
    const auto macInfo =
        (verifying ? "MATRIX_KEY_VERIFICATION_MAC%3%4%1%2%5%6"_ls
                   : "MATRIX_KEY_VERIFICATION_MAC%1%2%3%4%5%6"_ls)
            .arg(m_connection->userId(), m_connection->deviceId(),
                 m_remoteUserId, m_remoteDeviceId, m_room ? m_requestEventId : m_transactionId, input.contains(u',') ? QStringLiteral("KEY_IDS") : keyId)
            .toLatin1();
    if (m_commonMacCodes.contains(HmacSha256V2Code)) {
        auto mac = (*m_establishedSas)->calculate_mac(rust::String(input.toLatin1().data(), input.size()), rust::String(macInfo.data(), macInfo.size()))->to_base64();
        return QString::fromLatin1(mac.data(), mac.size());
    } else {
        auto mac = (*m_establishedSas)->calculate_mac_invalid_base64(rust::String(input.toLatin1().data(), input.size()), rust::String(macInfo.data(), macInfo.size()));
        return QString::fromLatin1(mac.data(), mac.size());
    }

}

void KeyVerificationSession::sendMac()
{
    QString keyId{ "ed25519:"_L1 % m_connection->deviceId() };

    const auto &masterKey = m_connection->isUserVerified(m_connection->userId()) ? m_connection->masterKeyForUser(m_connection->userId()) : QString();

    QStringList keyList{keyId};
    if (!masterKey.isEmpty()) {
        keyList += "ed25519:"_L1 + masterKey;
    }
    keyList.sort();

    auto keys = calculateMac(keyList.join(u','), false);

    QJsonObject mac;
    auto key = m_connection->olmAccount()->deviceKeys().keys.value(keyId);
    mac[keyId] = calculateMac(key, false, keyId);
    if (!masterKey.isEmpty()) {
        mac["ed25519:"_L1 + masterKey] = calculateMac(masterKey, false, "ed25519:"_L1 + masterKey);
    }

    sendEvent(m_remoteUserId, m_remoteDeviceId,
                               KeyVerificationMacEvent(m_transactionId, keys,
                                                       mac),
                               m_encrypted);
    setState (macReceived ? DONE : WAITINGFORMAC);
    m_verified = true;
    if (!m_pendingEdKeyId.isEmpty()) {
        trustKeys();
    }
}

void KeyVerificationSession::sendDone()
{
    sendEvent(m_remoteUserId, m_remoteDeviceId,
                               KeyVerificationDoneEvent(m_transactionId),
                               m_encrypted);
}

void KeyVerificationSession::sendKey()
{
    auto publicKey = m_sas->public_key()->to_base64();
    sendEvent(m_remoteUserId, m_remoteDeviceId,
                               KeyVerificationKeyEvent(m_transactionId,
                                                       QString::fromLatin1({publicKey.data(), (int) publicKey.size()})),
                               m_encrypted);
}


void KeyVerificationSession::cancelVerification(Error error)
{
    sendEvent(m_remoteUserId, m_remoteDeviceId, KeyVerificationCancelEvent(m_transactionId,
                                                          errorToString(error)), m_encrypted);
    setState(CANCELED);
    setError(error);
    emit finished();
    deleteLater();
}

void KeyVerificationSession::sendReady()
{
    auto methods = commonSupportedMethods(m_remoteSupportedMethods);

    if (methods.isEmpty()) {
        cancelVerification(UNKNOWN_METHOD);
        return;
    }

    sendEvent(
        m_remoteUserId, m_remoteDeviceId,
        KeyVerificationReadyEvent(m_transactionId, m_connection->deviceId(),
                                  methods),
        m_encrypted);
    setState(READY);

    if (methods.size() == 1) {
        sendStartSas();
    }
}

void KeyVerificationSession::sendStartSas()
{
    startSentByUs = true;
    KeyVerificationStartEvent event(m_transactionId, m_connection->deviceId());
    auto fixedJson = event.contentJson();
    if (m_room) {
        fixedJson.remove("transaction_id"_L1);
        fixedJson["m.relates_to"_L1] = QJsonObject {
            {"event_id"_L1, m_requestEventId},
            {"rel_type"_L1, "m.reference"_L1},
        };
    }
    m_startEvent = QString::fromUtf8(
        QJsonDocument(fixedJson).toJson(QJsonDocument::Compact));
    sendEvent(m_remoteUserId, m_remoteDeviceId, event,
                               m_encrypted);
    setState(WAITINGFORACCEPT);
}

void KeyVerificationSession::handleReady(const KeyVerificationReadyEvent& event)
{
    setState(READY);
    m_remoteSupportedMethods = event.methods();
    auto methods = commonSupportedMethods(m_remoteSupportedMethods);

    // This happens for outgoing user verification
    if (m_remoteDeviceId.isEmpty()) {
        m_remoteDeviceId = event.fromDevice();
    }

    if (methods.isEmpty())
        cancelVerification(UNKNOWN_METHOD);
    else if (methods.size() == 1)
        sendStartSas(); // -> WAITINGFORACCEPT
}

void KeyVerificationSession::handleStart(const KeyVerificationStartEvent& event)
{
    if (startSentByUs) {
        if (m_remoteUserId > m_connection->userId()
            || (m_remoteUserId == m_connection->userId()
                && m_remoteDeviceId > m_connection->deviceId())) {
            return;
        }
        startSentByUs = false;
    }
    const auto& theirMacs = event.messageAuthenticationCodes();
    for (const auto& macCode : SupportedMacs)
        if (theirMacs.contains(macCode))
            m_commonMacCodes.push_back(macCode);
    if (m_commonMacCodes.isEmpty()) {
        cancelVerification(UNKNOWN_METHOD);
        return;
    }

    auto publicKeyRust = m_sas->public_key()->to_base64();
    auto publicKey = QByteArray {publicKeyRust.data(), (int) publicKeyRust.size()};

    const auto canonicalEvent = QJsonDocument(event.contentJson()).toJson(QJsonDocument::Compact);
    const auto commitment =
        QString::fromLatin1(QCryptographicHash::hash(publicKey + canonicalEvent,
                                                     QCryptographicHash::Sha256)
                                .toBase64(QByteArray::OmitTrailingEquals));

    sendEvent(m_remoteUserId, m_remoteDeviceId,
                               KeyVerificationAcceptEvent(m_transactionId,
                                                          commitment),
                               m_encrypted);
    setState(ACCEPTED);
}

void KeyVerificationSession::handleMac(const KeyVerificationMacEvent& event)
{
    QStringList keys = event.mac().keys();
    keys.sort();
    const auto& key = keys.join(","_L1);
    const QString edKeyId = "ed25519:"_L1 % m_remoteDeviceId;

    if (calculateMac(m_connection->edKeyForUserDevice(m_remoteUserId,
                                                      m_remoteDeviceId),
                     true, edKeyId)
        != event.mac().value(edKeyId)) {
        cancelVerification(KEY_MISMATCH);
        return;
    }

    auto masterKey = m_connection->masterKeyForUser(m_remoteUserId);
    if (event.mac().contains("ed25519:"_L1 % masterKey)
        && calculateMac(masterKey, true, "ed25519:"_L1 % masterKey)
               != event.mac().value("ed25519:"_L1 % masterKey)) {
        cancelVerification(KEY_MISMATCH);
        return;
    }

    if (calculateMac(key, true) != event.keys()) {
        cancelVerification(KEY_MISMATCH);
        return;
    }

    m_pendingEdKeyId = edKeyId;
    m_pendingMasterKey = masterKey;

    if (m_verified) {
        trustKeys();
    }
}

void KeyVerificationSession::trustKeys()
{
    m_connection->database()->setSessionVerified(m_pendingEdKeyId);
    m_connection->database()->setMasterKeyVerified(m_pendingMasterKey);
    if (m_remoteUserId == m_connection->userId()) {
        m_connection->reloadDevices();
    }

    if (!m_pendingMasterKey.isEmpty()) {
        if (m_remoteUserId == m_connection->userId()) {
            const auto selfSigningKey = m_connection->database()->loadEncrypted("m.cross_signing.self_signing"_L1);
            if (!selfSigningKey.isEmpty()) {
                QHash<QString, QHash<QString, QJsonObject>> signatures;
                auto json = QJsonObject {
                    {"keys"_L1, QJsonObject {
                        {"ed25519:"_L1 + m_remoteDeviceId, m_connection->edKeyForUserDevice(m_remoteUserId, m_remoteDeviceId)},
                        {"curve25519:"_L1 + m_remoteDeviceId, m_connection->curveKeyForUserDevice(m_remoteUserId, m_remoteDeviceId)},
                    }},
                    {"algorithms"_L1, QJsonArray {"m.olm.v1.curve25519-aes-sha2"_L1, "m.megolm.v1.aes-sha2"_L1}},
                    {"device_id"_L1, m_remoteDeviceId},
                    {"user_id"_L1, m_remoteUserId},
                };
                auto signature = sign(selfSigningKey, QJsonDocument(json).toJson(QJsonDocument::Compact));
                json["signatures"_L1] = QJsonObject {
                    {m_connection->userId(), QJsonObject {
                        {"ed25519:"_L1 + m_connection->database()->selfSigningPublicKey(), QString::fromLatin1(signature)},
                    }},
                };
                signatures[m_remoteUserId][m_remoteDeviceId] = json;
                auto uploadSignatureJob = m_connection->callApi<UploadCrossSigningSignaturesJob>(signatures);
                connect(uploadSignatureJob, &BaseJob::finished, m_connection, [uploadSignatureJob]() {
                    if (uploadSignatureJob->error() != BaseJob::Success) {
                        qCWarning(E2EE) << "Failed to upload self-signing signature" << uploadSignatureJob << uploadSignatureJob->error() << uploadSignatureJob->errorString();
                    }
                });
            } else {
                // Not parenting to this since the session is going to be destroyed soon
                auto handler = new SSSSHandler(m_connection);
                handler->setConnection(m_connection);
                handler->unlockSSSSFromCrossSigning();
                connect(handler, &SSSSHandler::finished, handler, &QObject::deleteLater);
            }
        } else {
            const auto userSigningKey = m_connection->database()->loadEncrypted("m.cross_signing.user_signing"_L1);
            if (!userSigningKey.isEmpty()) {
                QHash<QString, QHash<QString, QJsonObject>> signatures;
                auto json = QJsonObject {
                    {"keys"_L1, QJsonObject {
                        {"ed25519:"_L1 + m_pendingMasterKey, m_pendingMasterKey},
                    }},
                    {"usage"_L1, QJsonArray {"master"_L1}},
                    {"user_id"_L1, m_remoteUserId},
                };
                auto signature = sign(userSigningKey, QJsonDocument(json).toJson(QJsonDocument::Compact));
                json["signatures"_L1] = QJsonObject {
                    {m_connection->userId(), QJsonObject {
                        {"ed25519:"_L1 + m_connection->database()->userSigningPublicKey(), QString::fromLatin1(signature)},
                    }},
                };
                signatures[m_remoteUserId][m_pendingMasterKey] = json;
                auto uploadSignatureJob = m_connection->callApi<UploadCrossSigningSignaturesJob>(signatures);
                connect(uploadSignatureJob, &BaseJob::finished, m_connection, [uploadSignatureJob, userId = m_remoteUserId](){
                    if (uploadSignatureJob->error() != BaseJob::Success) {
                        qCWarning(E2EE) << "Failed to upload user-signing signature for" << userId << uploadSignatureJob << uploadSignatureJob->error() << uploadSignatureJob->errorString();
                    }
                });
            }
        }
        emit m_connection->userVerified(m_remoteUserId);
    }

    emit m_connection->sessionVerified(m_remoteUserId, m_remoteDeviceId);
    macReceived = true;

    if (state() == WAITINGFORMAC) {
        setState(DONE);
        sendDone();
        emit finished();
        deleteLater();
    }
}

QVector<EmojiEntry> KeyVerificationSession::sasEmojis() const
{
    return m_sasEmojis;
}

void KeyVerificationSession::sendRequest()
{
    sendEvent(
        m_remoteUserId, m_remoteDeviceId,
        KeyVerificationRequestEvent(m_transactionId, m_connection->deviceId(),
                                    supportedMethods,
                                    QDateTime::currentDateTime()),
        m_encrypted);
    setState(WAITINGFORREADY);
}

KeyVerificationSession::State KeyVerificationSession::state() const
{
    return m_state;
}

void KeyVerificationSession::setState(KeyVerificationSession::State state)
{
    qCDebug(E2EE) << "KeyVerificationSession state" << m_state << "->" << state;
    m_state = state;
    emit stateChanged();
}

KeyVerificationSession::Error KeyVerificationSession::error() const
{
    return m_error;
}

void KeyVerificationSession::setError(Error error)
{
    m_error = error;
    emit errorChanged();
}

QString KeyVerificationSession::errorToString(Error error)
{
    switch(error) {
        case NONE:
            return "none"_L1;
        case TIMEOUT:
            return "m.timeout"_L1;
        case USER:
            return "m.user"_L1;
        case UNEXPECTED_MESSAGE:
            return "m.unexpected_message"_L1;
        case UNKNOWN_TRANSACTION:
            return "m.unknown_transaction"_L1;
        case UNKNOWN_METHOD:
            return "m.unknown_method"_L1;
        case KEY_MISMATCH:
            return "m.key_mismatch"_L1;
        case USER_MISMATCH:
            return "m.user_mismatch"_L1;
        case INVALID_MESSAGE:
            return "m.invalid_message"_L1;
        case SESSION_ACCEPTED:
            return "m.accepted"_L1;
        case MISMATCHED_COMMITMENT:
            return "m.mismatched_commitment"_L1;
        case MISMATCHED_SAS:
            return "m.mismatched_sas"_L1;
        default:
            return "m.user"_L1;
    }
}

KeyVerificationSession::Error KeyVerificationSession::stringToError(const QString& error)
{
    if (error == "m.timeout"_L1)
        return REMOTE_TIMEOUT;
    if (error == "m.user"_L1)
        return REMOTE_USER;
    if (error == "m.unexpected_message"_L1)
        return REMOTE_UNEXPECTED_MESSAGE;
    if (error == "m.unknown_message"_L1)
        return REMOTE_UNEXPECTED_MESSAGE;
    if (error == "m.unknown_transaction"_L1)
        return REMOTE_UNKNOWN_TRANSACTION;
    if (error == "m.unknown_method"_L1)
        return REMOTE_UNKNOWN_METHOD;
    if (error == "m.key_mismatch"_L1)
        return REMOTE_KEY_MISMATCH;
    if (error == "m.user_mismatch"_L1)
        return REMOTE_USER_MISMATCH;
    if (error == "m.invalid_message"_L1)
        return REMOTE_INVALID_MESSAGE;
    if (error == "m.accepted"_L1)
        return REMOTE_SESSION_ACCEPTED;
    if (error == "m.mismatched_commitment"_L1)
        return REMOTE_MISMATCHED_COMMITMENT;
    if (error == "m.mismatched_sas"_L1)
        return REMOTE_MISMATCHED_SAS;
    return NONE;
}

QString KeyVerificationSession::remoteDeviceId() const
{
    return m_remoteDeviceId;
}

QString KeyVerificationSession::transactionId() const
{
    return m_transactionId;
}

void KeyVerificationSession::sendEvent(const QString &userId, const QString &deviceId, const KeyVerificationEvent &event, bool encrypted)
{
    if (m_room) {
        auto json = event.contentJson();
        json.remove("transaction_id"_L1);
        if (event.metaType().matrixId == KeyVerificationRequestEvent::TypeId) {
            json["msgtype"_L1] = event.matrixType();
            json["body"_L1] = m_connection->userId() + " sent a verification request"_L1;
            json["to"_L1] = m_remoteUserId;
            m_room->postJson("m.room.message"_L1, json);
        } else {
            json["m.relates_to"_L1] = QJsonObject {
                {"event_id"_L1, m_requestEventId},
                {"rel_type"_L1, "m.reference"_L1}
            };
            m_room->postJson(event.matrixType(), json);
        }
    } else {
        m_connection->sendToDevice(userId, deviceId, event, encrypted);
    }
}

bool KeyVerificationSession::userVerification() const
{
    return m_room;
}

void KeyVerificationSession::setRequestEventId(const QString& eventId)
{
    m_requestEventId = eventId;
}
