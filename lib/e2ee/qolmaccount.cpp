// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "qolmaccount.h"

#include "connection.h"
#include "e2ee/qolmsession.h"
#include "e2ee/qolmutility.h"
#include "e2ee/qolmutils.h"

#include "csapi/keys.h"

#include <QtCore/QRandomGenerator>

#include <olm/olm.h>

using namespace Quotient;

// Convert olm error to enum
OlmErrorCode QOlmAccount::lastErrorCode() const {
    return olm_account_last_error_code(m_account);
}

const char* QOlmAccount::lastError() const
{
    return olm_account_last_error(m_account);
}

QOlmAccount::QOlmAccount(QStringView userId, QStringView deviceId,
                         QObject* parent)
    : QObject(parent)
    , m_userId(userId.toString())
    , m_deviceId(deviceId.toString())
{}

QOlmAccount::~QOlmAccount()
{
    olm_clear_account(m_account);
    delete[](reinterpret_cast<uint8_t *>(m_account));
}

void QOlmAccount::createNewAccount()
{
    m_account = olm_account(new uint8_t[olm_account_size()]);
    const auto randomLength = olm_create_account_random_length(m_account);
    QByteArray randomData = getRandom(randomLength);
    if (olm_create_account(m_account, randomData.data(), randomLength)
        == olm_error()) {
        throw lastError();
    }
    emit needsSave();
}

OlmErrorCode QOlmAccount::unpickle(QByteArray&& pickled, const PicklingMode &mode)
{
    m_account = olm_account(new uint8_t[olm_account_size()]);
    if (const auto key = toKey(mode);
        olm_unpickle_account(m_account, key.data(), key.length(),
                             pickled.data(), pickled.size())
        == olm_error()) {
        // Probably log the user out since we have no way of getting to the keys
        return lastErrorCode();
    }
    return OLM_SUCCESS;
}

QOlmExpected<QByteArray> QOlmAccount::pickle(const PicklingMode &mode)
{
    const QByteArray key = toKey(mode);
    const size_t pickleLength = olm_pickle_account_length(m_account);
    QByteArray pickleBuffer(pickleLength, '\0');
    if (olm_pickle_account(m_account, key.data(), key.length(),
                           pickleBuffer.data(), pickleLength)
        == olm_error())
        return lastErrorCode();
    return pickleBuffer;
}

IdentityKeys QOlmAccount::identityKeys() const
{
    const size_t keyLength = olm_account_identity_keys_length(m_account);
    QByteArray keyBuffer(keyLength, '\0');
    if (olm_account_identity_keys(m_account, keyBuffer.data(), keyLength)
        == olm_error()) {
        throw lastError();
    }
    const auto key = QJsonDocument::fromJson(keyBuffer).object();
    return IdentityKeys {
        key.value(QStringLiteral("curve25519")).toString().toUtf8(),
        key.value(QStringLiteral("ed25519")).toString().toUtf8()
    };
}

QByteArray QOlmAccount::sign(const QByteArray &message) const
{
    QByteArray signatureBuffer(olm_account_signature_length(m_account), '\0');

    if (olm_account_sign(m_account, message.data(), message.length(),
                         signatureBuffer.data(), signatureBuffer.length())
        == olm_error()) {
        throw lastError();
    }
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
    return olm_account_max_number_of_one_time_keys(m_account);
}

size_t QOlmAccount::generateOneTimeKeys(size_t numberOfKeys)
{
    const auto randomLength =
        olm_account_generate_one_time_keys_random_length(m_account,
                                                         numberOfKeys);
    QByteArray randomBuffer = getRandom(randomLength);
    const auto result =
        olm_account_generate_one_time_keys(m_account, numberOfKeys,
                                           randomBuffer.data(), randomLength);

    if (result == olm_error()) {
        throw lastError();
    }
    emit needsSave();
    return result;
}

UnsignedOneTimeKeys QOlmAccount::oneTimeKeys() const
{
    const auto oneTimeKeyLength = olm_account_one_time_keys_length(m_account);
    QByteArray oneTimeKeysBuffer(static_cast<int>(oneTimeKeyLength), '\0');

    if (olm_account_one_time_keys(m_account, oneTimeKeysBuffer.data(),
                                  oneTimeKeyLength)
        == olm_error()) {
        throw lastError();
    }
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
    if (olm_remove_one_time_keys(m_account, session.raw()) == olm_error()) {
        qWarning(E2EE).nospace()
            << "Failed to remove one-time keys for session "
            << session.sessionId() << ": " << lastError();
        return lastErrorCode();
    }
    emit needsSave();
    return OLM_SUCCESS;
}

OlmAccount* QOlmAccount::data() { return m_account; }

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

QOlmExpected<QOlmSessionPtr> QOlmAccount::createInboundSession(
    const QOlmMessage& preKeyMessage)
{
    Q_ASSERT(preKeyMessage.type() == QOlmMessage::PreKey);
    return QOlmSession::createInboundSession(this, preKeyMessage);
}

QOlmExpected<QOlmSessionPtr> QOlmAccount::createInboundSessionFrom(
    const QByteArray& theirIdentityKey, const QOlmMessage& preKeyMessage)
{
    Q_ASSERT(preKeyMessage.type() == QOlmMessage::PreKey);
    return QOlmSession::createInboundSessionFrom(this, theirIdentityKey,
                                                 preKeyMessage);
}

QOlmExpected<QOlmSessionPtr> QOlmAccount::createOutboundSession(
    const QByteArray& theirIdentityKey, const QByteArray& theirOneTimeKey)
{
    return QOlmSession::createOutboundSession(this, theirIdentityKey,
                                              theirOneTimeKey);
}

void QOlmAccount::markKeysAsPublished()
{
    olm_account_mark_keys_as_published(m_account);
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
    return utility.ed25519Verify(signingKeyBuf, canonicalJson, signatureBuf)
        .value_or(false);
}
