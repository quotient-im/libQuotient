// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "qolmaccount.h"

#include "connection.h"
#include "e2ee/qolmsession.h"
#include "e2ee/qolmutility.h"

#include "csapi/keys.h"

#include <QtCore/QRandomGenerator>

#include <olm/olm.h>
#include <span>

using namespace Quotient;

// Convert olm error to enum
OlmErrorCode QOlmAccount::lastErrorCode() const {
    return olm_account_last_error_code(olmData);
}

const char* QOlmAccount::lastError() const
{
    return olm_account_last_error(olmData);
}

QOlmExpected<QOlmSession> QOlmAccount::createInbound(
    QOlmMessage preKeyMessage, const QByteArray& theirIdentityKey) const
{
    if (preKeyMessage.type() != QOlmMessage::PreKey) {
        qCCritical(E2EE) << "The message is not a pre-key; will try to create "
                            "the inbound session anyway";
    }

    QOlmSession session{};

    // std::span has size_type that fits standard library and Olm, avoiding
    // the warning noise about integer signedness/precision
    const std::span oneTimeKeyMessageBuf{ preKeyMessage.begin(),
                                          preKeyMessage.end() };
    const std::span theirIdentityKeyBuf{ theirIdentityKey.cbegin(),
                                         theirIdentityKey.cend() };
    const auto error =
        theirIdentityKey.isEmpty()
            ? olm_create_inbound_session(session.olmData, olmData,
                                         oneTimeKeyMessageBuf.data(),
                                         oneTimeKeyMessageBuf.size())
            : olm_create_inbound_session_from(session.olmData, olmData,
                                              theirIdentityKeyBuf.data(),
                                              theirIdentityKeyBuf.size(),
                                              oneTimeKeyMessageBuf.data(),
                                              oneTimeKeyMessageBuf.size());

    if (error == olm_error()) {
        qCWarning(E2EE) << "Error when creating inbound session"
                        << session.lastError();
        return session.lastErrorCode();
    }

    return session;
}

QOlmAccount::QOlmAccount(QStringView userId, QStringView deviceId,
                         QObject* parent)
    : QObject(parent)
    , olmDataHolder(
          makeCStruct(olm_account, olm_account_size, olm_clear_account))
    , m_userId(userId.toString())
    , m_deviceId(deviceId.toString())
{}

void QOlmAccount::setupNewAccount()
{
    if (const auto randomLength = olm_create_account_random_length(olmData);
        olm_create_account(olmData, getRandom(randomLength).data(), randomLength)
        == olm_error())
        QOLM_INTERNAL_ERROR("Failed to setup a new account");

    emit needsSave();
}

OlmErrorCode QOlmAccount::unpickle(QByteArray&& pickled, const PicklingKey& key)
{
    if (olm_unpickle_account(olmData, key.data(), key.size(), pickled.data(),
                             pickled.size())
        == olm_error()) {
        // Probably log the user out since we have no way of getting to the keys
        return lastErrorCode();
    }
    return OLM_SUCCESS;
}

QByteArray QOlmAccount::pickle(const PicklingKey& key) const
{
    const size_t pickleLength = olm_pickle_account_length(olmData);
    QByteArray pickleBuffer(pickleLength, '\0');
    if (olm_pickle_account(olmData, key.data(), key.size(),
                           pickleBuffer.data(), pickleLength)
        == olm_error())
        QOLM_INTERNAL_ERROR(qPrintable("Failed to pickle Olm account "
                                       + accountId()));

    return pickleBuffer;
}

IdentityKeys QOlmAccount::identityKeys() const
{
    const size_t keyLength = olm_account_identity_keys_length(olmData);
    QByteArray keyBuffer(keyLength, '\0');
    if (olm_account_identity_keys(olmData, keyBuffer.data(), keyLength)
        == olm_error()) {
        QOLM_INTERNAL_ERROR(
            qPrintable("Failed to get " % accountId() % " identity keys"));
    }
    const auto key = QJsonDocument::fromJson(keyBuffer).object();
    return IdentityKeys {
        key.value(QStringLiteral("curve25519")).toString().toUtf8(),
        key.value(QStringLiteral("ed25519")).toString().toUtf8()
    };
}

QByteArray QOlmAccount::sign(const QByteArray &message) const
{
    const auto signatureLength = olm_account_signature_length(olmData);
    auto signatureBuffer = bufferForOlm(signatureLength);

    if (olm_account_sign(olmData, message.data(), message.length(),
                         signatureBuffer.data(), signatureLength)
        == olm_error())
        QOLM_INTERNAL_ERROR("Failed to sign a message");

    return signatureBuffer;
}

QByteArray QOlmAccount::sign(const QJsonObject &message) const
{
    return sign(QJsonDocument(message).toJson(QJsonDocument::Compact));
}

QByteArray QOlmAccount::signIdentityKeys() const
{
    const auto keys = identityKeys();
    return sign(QJsonObject{
        { "algorithms", QJsonArray{ "m.olm.v1.curve25519-aes-sha2",
                                    "m.megolm.v1.aes-sha2" } },
        { "user_id", m_userId },
        { "device_id", m_deviceId },
        { "keys", QJsonObject{ { QStringLiteral("curve25519:") + m_deviceId,
                                 QString::fromUtf8(keys.curve25519) },
                               { QStringLiteral("ed25519:") + m_deviceId,
                                 QString::fromUtf8(keys.ed25519) } } } });
}

size_t QOlmAccount::maxNumberOfOneTimeKeys() const
{
    return olm_account_max_number_of_one_time_keys(olmData);
}

size_t QOlmAccount::generateOneTimeKeys(size_t numberOfKeys)
{
    const auto randomLength =
        olm_account_generate_one_time_keys_random_length(olmData, numberOfKeys);
    const auto result = olm_account_generate_one_time_keys(
        olmData, numberOfKeys, getRandom(randomLength).data(), randomLength);

    if (result == olm_error())
        QOLM_INTERNAL_ERROR(qPrintable(
            "Failed to generate one-time keys for account " + accountId()));

    emit needsSave();
    return result;
}

UnsignedOneTimeKeys QOlmAccount::oneTimeKeys() const
{
    const auto oneTimeKeyLength = olm_account_one_time_keys_length(olmData);
    QByteArray oneTimeKeysBuffer(static_cast<int>(oneTimeKeyLength), '\0');

    if (olm_account_one_time_keys(olmData, oneTimeKeysBuffer.data(),
                                  oneTimeKeyLength)
        == olm_error())
        QOLM_INTERNAL_ERROR(qPrintable(
            "Failed to obtain one-time keys for account" % accountId()));

    const auto json = QJsonDocument::fromJson(oneTimeKeysBuffer).object();
    UnsignedOneTimeKeys oneTimeKeys;
    fromJson(json, oneTimeKeys.keys);
    return oneTimeKeys;
}

OneTimeKeys QOlmAccount::signOneTimeKeys(const UnsignedOneTimeKeys &keys) const
{
    OneTimeKeys signedOneTimeKeys;
    for (const auto& curveKeys = keys.curve25519();
         const auto& [keyId, key] : asKeyValueRange(curveKeys))
        signedOneTimeKeys.insert("signed_curve25519:" % keyId,
                                 SignedOneTimeKey {
                                     key, m_userId, m_deviceId,
                                     sign(QJsonObject { { "key", key } }) });
    return signedOneTimeKeys;
}

OlmErrorCode QOlmAccount::removeOneTimeKeys(const QOlmSession& session)
{
    if (olm_remove_one_time_keys(olmData, session.olmData) == olm_error()) {
        qWarning(E2EE).nospace()
            << "Failed to remove one-time keys for session "
            << session.sessionId() << ": " << lastError();
        return lastErrorCode();
    }
    emit needsSave();
    return OLM_SUCCESS;
}

DeviceKeys QOlmAccount::deviceKeys() const
{
    static QStringList Algorithms(SupportedAlgorithms.cbegin(),
                                  SupportedAlgorithms.cend());

    const auto idKeys = identityKeys();
    return DeviceKeys{
        .userId = m_userId,
        .deviceId = m_deviceId,
        .algorithms = Algorithms,
        .keys{ { "curve25519:" + m_deviceId, idKeys.curve25519 },
               { "ed25519:" + m_deviceId, idKeys.ed25519 } },
        .signatures{
            { m_userId, { { "ed25519:" + m_deviceId, signIdentityKeys() } } } }
    };
}

UploadKeysJob* QOlmAccount::createUploadKeyRequest(
    const UnsignedOneTimeKeys& oneTimeKeys) const
{
    return new UploadKeysJob(deviceKeys(), signOneTimeKeys(oneTimeKeys));
}

QOlmExpected<QOlmSession> QOlmAccount::createInboundSession(
    const QOlmMessage& preKeyMessage) const
{
    Q_ASSERT(preKeyMessage.type() == QOlmMessage::PreKey);
    return createInbound(preKeyMessage);
}

QOlmExpected<QOlmSession> QOlmAccount::createInboundSessionFrom(
    const QByteArray& theirIdentityKey, const QOlmMessage& preKeyMessage) const
{
    Q_ASSERT(preKeyMessage.type() == QOlmMessage::PreKey);
    return createInbound(preKeyMessage, theirIdentityKey);
}

QOlmExpected<QOlmSession> QOlmAccount::createOutboundSession(
    const QByteArray& theirIdentityKey, const QByteArray& theirOneTimeKey) const
{
    QOlmSession olmOutboundSession{};
    if (const auto randomLength = olm_create_outbound_session_random_length(
            olmOutboundSession.olmData);
        olm_create_outbound_session(
            olmOutboundSession.olmData, olmData, theirIdentityKey.data(),
            theirIdentityKey.length(), theirOneTimeKey.data(),
            theirOneTimeKey.length(), getRandom(randomLength).data(),
            randomLength)
        == olm_error()) {
        const auto errorCode = olmOutboundSession.lastErrorCode();
        QOLM_FAIL_OR_LOG_X(errorCode == OLM_NOT_ENOUGH_RANDOM,
                         "Failed to create an outbound Olm session",
                           olmOutboundSession.lastError());
        return errorCode;
    }
    return olmOutboundSession;
}

void QOlmAccount::markKeysAsPublished()
{
    olm_account_mark_keys_as_published(olmData);
    emit needsSave();
}

bool Quotient::verifyIdentitySignature(const DeviceKeys& deviceKeys,
                                       const QString& deviceId,
                                       const QString& userId)
{
    const auto signKeyId = "ed25519:" + deviceId;
    const auto signingKey = deviceKeys.keys[signKeyId];
    const auto signature = deviceKeys.signatures[userId][signKeyId];

    return ed25519VerifySignature(signingKey, toJson(deviceKeys), signature);
}

bool Quotient::ed25519VerifySignature(const QString& signingKey,
                                      const QJsonObject& obj,
                                      const QString& signature)
{
    if (signature.isEmpty())
        return false;

    QJsonObject obj1 = obj;

    obj1.remove("unsigned");
    obj1.remove("signatures");

    auto canonicalJson = QJsonDocument(obj1).toJson(QJsonDocument::Compact);

    QByteArray signingKeyBuf = signingKey.toUtf8();
    QOlmUtility utility;
    auto signatureBuf = signature.toUtf8();
    return utility.ed25519Verify(signingKeyBuf, canonicalJson, signatureBuf);
}

QString QOlmAccount::accountId() const { return m_userId % '/' % m_deviceId; }
