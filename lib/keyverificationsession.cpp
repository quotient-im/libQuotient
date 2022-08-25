// SPDX-FileCopyrightText: 2022 Tobias Fella <fella@posteo.de>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "keyverificationsession.h"

#include "connection.h"
#include "database.h"
#include "e2ee/qolmaccount.h"
#include "e2ee/qolmutils.h"
#include "olm/sas.h"

#include "events/event.h"

#include <QtCore/QCryptographicHash>
#include <QtCore/QTimer>
#include <QtCore/QUuid>

#include <chrono>

using namespace Quotient;
using namespace std::chrono;

KeyVerificationSession::KeyVerificationSession(
    QString remoteUserId, const KeyVerificationRequestEvent& event,
    Connection* connection, bool encrypted)
    : QObject(connection)
    , m_remoteUserId(std::move(remoteUserId))
    , m_remoteDeviceId(event.fromDevice())
    , m_transactionId(event.transactionId())
    , m_connection(connection)
    , m_encrypted(encrypted)
    , m_remoteSupportedMethods(event.methods())
{
    const auto& currentTime = QDateTime::currentDateTime();
    const auto timeoutTime =
        std::min(event.timestamp().addSecs(600), currentTime.addSecs(120));
    const milliseconds timeout{ currentTime.msecsTo(timeoutTime) };
    if (timeout > 5s)
        init(timeout);
    // Otherwise don't even bother starting up
}

KeyVerificationSession::KeyVerificationSession(QString userId, QString deviceId,
                                               Connection* connection)
    : QObject(connection)
    , m_remoteUserId(std::move(userId))
    , m_remoteDeviceId(std::move(deviceId))
    , m_transactionId(QUuid::createUuid().toString())
    , m_connection(connection)
    , m_encrypted(false)
{
    init(600s);
    QMetaObject::invokeMethod(this, &KeyVerificationSession::sendRequest);
}

void KeyVerificationSession::init(milliseconds timeout)
{
    connect(m_connection, &Connection::incomingKeyVerificationReady, this, [this](const KeyVerificationReadyEvent& event) {
        if (event.transactionId() == m_transactionId && event.fromDevice() == m_remoteDeviceId) {
            handleReady(event);
        }
    });
    connect(m_connection, &Connection::incomingKeyVerificationStart, this, [this](const KeyVerificationStartEvent& event) {
        if (event.transactionId() == m_transactionId && event.fromDevice() == m_remoteDeviceId) {
            handleStart(event);
        }
    });
    connect(m_connection, &Connection::incomingKeyVerificationAccept, this, [this](const KeyVerificationAcceptEvent& event) {
        if (event.transactionId() == m_transactionId) {
            handleAccept(event);
        }
    });
    connect(m_connection, &Connection::incomingKeyVerificationKey, this, [this](const KeyVerificationKeyEvent& event) {
        if (event.transactionId() == m_transactionId) {
            handleKey(event);
        }
    });
    connect(m_connection, &Connection::incomingKeyVerificationMac, this, [this](const KeyVerificationMacEvent& event) {
        if (event.transactionId() == m_transactionId) {
            handleMac(event);
        }
    });
    connect(m_connection, &Connection::incomingKeyVerificationDone, this, [this](const KeyVerificationDoneEvent& event) {
        if (event.transactionId() == m_transactionId) {
            handleDone(event);
        }
    });
    connect(m_connection, &Connection::incomingKeyVerificationCancel, this, [this](const KeyVerificationCancelEvent& event) {
        if (event.transactionId() == m_transactionId) {
            handleCancel(event);
        }
    });

    QTimer::singleShot(timeout, this, [this] { cancelVerification(TIMEOUT); });

    m_sas = olm_sas(new uint8_t[olm_sas_size()]);
    auto randomSize = olm_create_sas_random_length(m_sas);
    auto random = getRandom(randomSize);
    olm_create_sas(m_sas, random.data(), randomSize);

    m_language = QLocale::system().uiLanguages()[0];
    m_language = m_language.left(m_language.indexOf('-'));
}

KeyVerificationSession::~KeyVerificationSession()
{
    delete[] reinterpret_cast<uint8_t*>(m_sas);
}

void KeyVerificationSession::handleKey(const KeyVerificationKeyEvent& event)
{
    if (state() != WAITINGFORKEY && state() != WAITINGFORVERIFICATION) {
        cancelVerification(UNEXPECTED_MESSAGE);
        return;
    }
    olm_sas_set_their_key(m_sas, event.key().toLatin1().data(), event.key().toLatin1().size());

    if (startSentByUs) {
        auto commitment = QString(QCryptographicHash::hash((event.key() % m_startEvent).toLatin1(), QCryptographicHash::Sha256).toBase64());
        commitment = commitment.left(commitment.indexOf('='));
        if (commitment != m_commitment) {
            qCWarning(E2EE) << "Commitment mismatch; aborting verification";
            cancelVerification(MISMATCHED_COMMITMENT);
            return;
        }
    } else {
        sendKey();
    }
    setState(WAITINGFORVERIFICATION);

    QByteArray keyBytes(olm_sas_pubkey_length(m_sas), '\0');
    olm_sas_get_pubkey(m_sas, keyBytes.data(), keyBytes.size());
    QString key = QString(keyBytes);

    QByteArray output(6, '\0');
    QString infoTemplate = startSentByUs ? "MATRIX_KEY_VERIFICATION_SAS|%1|%2|%3|%4|%5|%6|%7"_ls : "MATRIX_KEY_VERIFICATION_SAS|%4|%5|%6|%1|%2|%3|%7"_ls;

    auto info = infoTemplate.arg(m_connection->userId()).arg(m_connection->deviceId()).arg(key).arg(m_remoteUserId).arg(m_remoteDeviceId).arg(event.key()).arg(m_transactionId);
    olm_sas_generate_bytes(m_sas, info.toLatin1().data(), info.toLatin1().size(), output.data(), output.size());

    QVector<uint8_t> code(7, 0);
    const auto& data = (uint8_t *) output.data();

    code[0] = data[0] >> 2;
    code[1] = (data[0] << 4 & 0x3f) | data[1] >> 4;
    code[2] = (data[1] << 2 & 0x3f) | data[2] >> 6;
    code[3] = data[2] & 0x3f;
    code[4] = data[3] >> 2;
    code[5] = (data[3] << 4 & 0x3f) | data[4] >> 4;
    code[6] = (data[4] << 2 & 0x3f) | data[5] >> 6;

    for (const auto& c : code) {
        auto [emoji, description] = emojiForCode(c);
        QVariantMap map;
        map["emoji"] = emoji;
        map["description"] = description;
        m_sasEmojis += map;
    }
    emit sasEmojisChanged();
    emit keyReceived();
}

QByteArray KeyVerificationSession::macInfo(bool verifying, const QString& key)
{
    return (verifying ? "MATRIX_KEY_VERIFICATION_MAC%3%4%1%2%5%6"_ls : "MATRIX_KEY_VERIFICATION_MAC%1%2%3%4%5%6"_ls).arg(m_connection->userId()).arg(m_connection->deviceId()).arg(m_remoteUserId).arg(m_remoteDeviceId).arg(m_transactionId).arg(key).toLatin1();
}

QString KeyVerificationSession::calculateMac(const QString& input, bool verifying, const QString& keyId)
{
    QByteArray inputBytes = input.toLatin1();
    QByteArray outputBytes(olm_sas_mac_length(m_sas), '\0');
    olm_sas_calculate_mac(m_sas, inputBytes.data(), inputBytes.size(), macInfo(verifying, keyId).data(), macInfo(verifying, keyId).size(), outputBytes.data(), outputBytes.size());
    auto output = QString(outputBytes);
    return output.left(output.indexOf('='));
}

void KeyVerificationSession::sendMac()
{
    QString edKeyId = "ed25519:" % m_connection->deviceId();

    auto keys = calculateMac(edKeyId, false);

    QJsonObject mac;
    auto key = m_connection->olmAccount()->deviceKeys().keys[edKeyId];
    mac[edKeyId] = calculateMac(key, false, edKeyId);

    auto event = makeEvent<KeyVerificationMacEvent>(QJsonObject {
        {"type", "m.key.verification.mac"},
        {"content", QJsonObject{
            {"transaction_id", m_transactionId},
            {"keys", keys},
            {"mac", mac},
        }}
    });
    m_connection->sendToDevice(m_remoteUserId, m_remoteDeviceId, std::move(event), m_encrypted);
    setState (macReceived ? DONE : WAITINGFORMAC);
}

void KeyVerificationSession::sendDone()
{
    auto event = makeEvent<KeyVerificationDoneEvent>(QJsonObject {
        {"type", "m.key.verification.done"},
        {"content", QJsonObject{
            {"transaction_id", m_transactionId},
        }}
    });
    m_connection->sendToDevice(m_remoteUserId, m_remoteDeviceId, std::move(event), m_encrypted);
}

void KeyVerificationSession::sendKey()
{
    QByteArray keyBytes(olm_sas_pubkey_length(m_sas), '\0');
    olm_sas_get_pubkey(m_sas, keyBytes.data(), keyBytes.size());
    QString key = QString(keyBytes);
    auto event = makeEvent<KeyVerificationKeyEvent>(QJsonObject {
        {"type", "m.key.verification.key"},
        {"content", QJsonObject{
            {"transaction_id", m_transactionId},
            {"key", key},
        }}
    });
    m_connection->sendToDevice(m_remoteUserId, m_remoteDeviceId, std::move(event), m_encrypted);
}


void KeyVerificationSession::cancelVerification(Error error)
{
    auto event = makeEvent<KeyVerificationCancelEvent>(QJsonObject {
        {"type", "m.key.verification.cancel"},
        {"content", QJsonObject{
            {"code", errorToString(error)},
            {"reason", errorToString(error)},
            {"transaction_id", m_transactionId}
        }}
    });
    m_connection->sendToDevice(m_remoteUserId, m_remoteDeviceId, std::move(event), m_encrypted);
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

    auto event = makeEvent<KeyVerificationReadyEvent>(QJsonObject {
        {"type", "m.key.verification.ready"},
        {"content", QJsonObject {
            {"from_device", m_connection->deviceId()},
            {"methods", toJson(methods)},
            {"transaction_id", m_transactionId},
        }}
    });
    m_connection->sendToDevice(m_remoteUserId, m_remoteDeviceId, std::move(event), m_encrypted);
    setState(READY);

    if (methods.size() == 1) {
        sendStartSas();
    }
}

void KeyVerificationSession::sendStartSas()
{
    startSentByUs = true;
    auto event = makeEvent<KeyVerificationStartEvent>(QJsonObject {
        {"type", "m.key.verification.start"},
        {"content", QJsonObject {
            {"from_device", m_connection->deviceId()},
            {"hashes", QJsonArray {"sha256"}},
            {"key_agreement_protocols", QJsonArray { "curve25519-hkdf-sha256" }},
            {"message_authentication_codes", QJsonArray { "hkdf-hmac-sha256" }},
            {"method", "m.sas.v1"},
            {"short_authentication_string", QJsonArray { "decimal", "emoji" }},
            {"transaction_id", m_transactionId},
        }}
    });
    m_startEvent = QJsonDocument(event->contentJson()).toJson(QJsonDocument::Compact);
    m_connection->sendToDevice(m_remoteUserId, m_remoteDeviceId, std::move(event), m_encrypted);
    setState(WAITINGFORACCEPT);
}

void KeyVerificationSession::handleReady(const KeyVerificationReadyEvent& event)
{
    if (state() != WAITINGFORREADY) {
        cancelVerification(UNEXPECTED_MESSAGE);
        return;
    }
    setState(READY);
    m_remoteSupportedMethods = event.methods();
    auto methods = commonSupportedMethods(m_remoteSupportedMethods);

    if (methods.isEmpty()) {
        cancelVerification(UNKNOWN_METHOD);
        return;
    }

    if (methods.size() == 1) {
        sendStartSas();
    }
}

void KeyVerificationSession::handleStart(const KeyVerificationStartEvent& event)
{
    if (state() != READY) {
        cancelVerification(UNEXPECTED_MESSAGE);
        return;
    }
    if (startSentByUs) {
        if (m_remoteUserId > m_connection->userId() || (m_remoteUserId == m_connection->userId() && m_remoteDeviceId > m_connection->deviceId())) {
            return;
        } else {
            startSentByUs = false;
        }
    }
    QByteArray publicKey(olm_sas_pubkey_length(m_sas), '\0');
    olm_sas_get_pubkey(m_sas, publicKey.data(), publicKey.size());
    const auto canonicalEvent = QString(QJsonDocument(event.contentJson()).toJson(QJsonDocument::Compact));
    auto commitment = QString(QCryptographicHash::hash((QString(publicKey) % canonicalEvent).toLatin1(), QCryptographicHash::Sha256).toBase64());
    commitment = commitment.left(commitment.indexOf('='));

    auto acceptEvent = makeEvent<KeyVerificationAcceptEvent>(QJsonObject {
        {"type", "m.key.verification.accept"},
        {"content", QJsonObject {
            {"commitment", commitment},
            {"hash", "sha256"},
            {"key_agreement_protocol", "curve25519-hkdf-sha256"},
            {"message_authentication_code", "hkdf-hmac-sha256"},
            {"method", "m.sas.v1"},
            {"short_authentication_string", QJsonArray {
                "decimal",
                "emoji",
            }},
            {"transaction_id", m_transactionId},
        }}
    });
    m_connection->sendToDevice(m_remoteUserId, m_remoteDeviceId, std::move(acceptEvent), m_encrypted);
    setState(ACCEPTED);
}

void KeyVerificationSession::handleAccept(const KeyVerificationAcceptEvent& event)
{
    if(state() != WAITINGFORACCEPT) {
        cancelVerification(UNEXPECTED_MESSAGE);
        return;
    }
    m_commitment = event.commitment();
    sendKey();
    setState(WAITINGFORKEY);
}

void KeyVerificationSession::handleMac(const KeyVerificationMacEvent& event)
{
    QStringList keys = event.mac().keys();
    keys.sort();
    const auto& key = keys.join(",");
    const QString edKeyId = "ed25519:"_ls % m_remoteDeviceId;

    if (calculateMac(m_connection->edKeyForUserDevice(m_remoteUserId, m_remoteDeviceId), true, edKeyId) != event.mac()[edKeyId]) {
        cancelVerification(KEY_MISMATCH);
        return;
    }

    if (calculateMac(key, true) != event.keys()) {
        cancelVerification(KEY_MISMATCH);
        return;
    }

    m_connection->database()->setSessionVerified(edKeyId);
    emit m_connection->sessionVerified(m_remoteUserId, m_remoteDeviceId);
    macReceived = true;

    if (state() == WAITINGFORMAC) {
        setState(DONE);
        sendDone();
        emit finished();
        deleteLater();
    }
}

void KeyVerificationSession::handleDone(const KeyVerificationDoneEvent&)
{
    if (state() != DONE) {
        cancelVerification(UNEXPECTED_MESSAGE);
    }
}

void KeyVerificationSession::handleCancel(const KeyVerificationCancelEvent& event)
{
    setError(stringToError(event.code()));
    setState(CANCELED);
}

std::pair<QString, QString> KeyVerificationSession::emojiForCode(int code)
{
    static QJsonArray data;
    if (data.isEmpty()) {
        QFile dataFile(":/sas-emoji.json");
        dataFile.open(QFile::ReadOnly);
        data = QJsonDocument::fromJson(dataFile.readAll()).array();
    }
    if (data[code].toObject()["translated_descriptions"].toObject().contains(m_language)) {
        return {data[code].toObject()["emoji"].toString(), data[code].toObject()["translated_descriptions"].toObject()[m_language].toString()};
    }
    return {data[code].toObject()["emoji"].toString(), data[code].toObject()["description"].toString()};
}

QList<QVariantMap> KeyVerificationSession::sasEmojis() const
{
    return m_sasEmojis;
}

void KeyVerificationSession::sendRequest()
{
    QJsonArray methods = toJson(m_supportedMethods);
    auto event = makeEvent<KeyVerificationRequestEvent>(QJsonObject {
        {"type", "m.key.verification.request"},
        {"content", QJsonObject {
            {"from_device", m_connection->deviceId()},
            {"transaction_id", m_transactionId},
            {"methods", methods},
            {"timestamp", QDateTime::currentMSecsSinceEpoch()},
        }},
    });
    m_connection->sendToDevice(m_remoteUserId, m_remoteDeviceId, std::move(event), m_encrypted);
    setState(WAITINGFORREADY);
}

KeyVerificationSession::State KeyVerificationSession::state() const
{
    return m_state;
}

void KeyVerificationSession::setState(KeyVerificationSession::State state)
{
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

QString KeyVerificationSession::errorToString(Error error) const
{
    switch(error) {
        case NONE:
            return "none"_ls;
        case TIMEOUT:
            return "m.timeout"_ls;
        case USER:
            return "m.user"_ls;
        case UNEXPECTED_MESSAGE:
            return "m.unexpected_message"_ls;
        case UNKNOWN_TRANSACTION:
            return "m.unknown_transaction"_ls;
        case UNKNOWN_METHOD:
            return "m.unknown_method"_ls;
        case KEY_MISMATCH:
            return "m.key_mismatch"_ls;
        case USER_MISMATCH:
            return "m.user_mismatch"_ls;
        case INVALID_MESSAGE:
            return "m.invalid_message"_ls;
        case SESSION_ACCEPTED:
            return "m.accepted"_ls;
        case MISMATCHED_COMMITMENT:
            return "m.mismatched_commitment"_ls;
        case MISMATCHED_SAS:
            return "m.mismatched_sas"_ls;
        default:
            return "m.user"_ls;
    }
}

KeyVerificationSession::Error KeyVerificationSession::stringToError(const QString& error) const
{
    if (error == "m.timeout"_ls) {
        return REMOTE_TIMEOUT;
    } else if (error == "m.user"_ls) {
        return REMOTE_USER;
    } else if (error == "m.unexpected_message"_ls) {
        return REMOTE_UNEXPECTED_MESSAGE;
    } else if (error == "m.unknown_message"_ls) {
        return REMOTE_UNEXPECTED_MESSAGE;
    } else if (error == "m.unknown_transaction"_ls) {
        return REMOTE_UNKNOWN_TRANSACTION;
    } else if (error == "m.unknown_method"_ls) {
        return REMOTE_UNKNOWN_METHOD;
    } else if (error == "m.key_mismatch"_ls) {
        return REMOTE_KEY_MISMATCH;
    } else if (error == "m.user_mismatch"_ls) {
        return REMOTE_USER_MISMATCH;
    } else if (error == "m.invalid_message"_ls) {
        return REMOTE_INVALID_MESSAGE;
    } else if (error == "m.accepted"_ls) {
        return REMOTE_SESSION_ACCEPTED;
    } else if (error == "m.mismatched_commitment"_ls) {
        return REMOTE_MISMATCHED_COMMITMENT;
    } else if (error == "m.mismatched_sas"_ls) {
        return REMOTE_MISMATCHED_SAS;
    }
    return NONE;
}

QStringList KeyVerificationSession::commonSupportedMethods(const QStringList& remoteMethods) const
{
    QStringList result;
    for (const auto& method : remoteMethods) {
        if (m_supportedMethods.contains(method)) {
            result += method;
        }
    }
    return result;
}
