// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifdef Quotient_E2EE_ENABLED
#include "qolmaccount.h"
#include "crypto/qolmutils.h"
#include <QJsonObject>
#include <QJsonDocument>
#include <QDebug>
#include <iostream>

using namespace Quotient;

QMap<QString, QString> OneTimeKeys::curve25519() const
{
    return keys[QStringLiteral("curve25519")];
}

std::optional<QMap<QString, QString>> OneTimeKeys::get(QString keyType) const
{
    if (!keys.contains(keyType)) {
        return std::nullopt;
    }
    return keys[keyType];
}

bool operator==(const IdentityKeys& lhs, const IdentityKeys& rhs)
{
    return lhs.curve25519 == rhs.curve25519 &&& lhs.ed25519 == rhs.ed25519;
}

// Convert olm error to enum
QOlmError lastError(OlmAccount *account) {
    const std::string error_raw = olm_account_last_error(account);

    return fromString(error_raw);
}

QByteArray getRandom(size_t bufferSize)
{
    QByteArray buffer(bufferSize, '0');
    std::generate(buffer.begin(), buffer.end(), std::rand);
    return buffer;
}

QOlmAccount::QOlmAccount(const QString &userId, const QString &deviceId)
    : m_userId(userId)
    , m_deviceId(deviceId)
{
}

QOlmAccount::~QOlmAccount()
{
    olm_clear_account(m_account);
    delete[](reinterpret_cast<uint8_t *>(m_account));
}

void QOlmAccount::createNewAccount()
{
    m_account = olm_account(new uint8_t[olm_account_size()]);
    size_t randomSize = olm_create_account_random_length(m_account);
    QByteArray randomData = getRandom(randomSize);
    const auto error = olm_create_account(m_account, randomData.data(), randomSize);
    if (error == olm_error()) {
        throw lastError(m_account);
    }
}

void QOlmAccount::unpickle(QByteArray &pickled, const PicklingMode &mode)
{
    m_account = olm_account(new uint8_t[olm_account_size()]);
    const QByteArray key = toKey(mode);
    const auto error = olm_unpickle_account(m_account, key.data(), key.length(), pickled.data(), pickled.size());
    if (error == olm_error()) {
        throw lastError(m_account);
    }
}

std::variant<QByteArray, QOlmError> QOlmAccount::pickle(const PicklingMode &mode)
{
    const QByteArray key = toKey(mode);
    const size_t pickleLength = olm_pickle_account_length(m_account);
    QByteArray pickleBuffer(pickleLength, '0');
    const auto error = olm_pickle_account(m_account, key.data(),
                key.length(), pickleBuffer.data(), pickleLength);
    if (error == olm_error()) {
        return lastError(m_account);
    }
    return pickleBuffer;
}

IdentityKeys QOlmAccount::identityKeys() const
{
    const size_t keyLength = olm_account_identity_keys_length(m_account);
    QByteArray keyBuffer(keyLength, '0');
    const auto error = olm_account_identity_keys(m_account, keyBuffer.data(), keyLength);
    if (error == olm_error()) {
        throw lastError(m_account);
    }
    const QJsonObject key = QJsonDocument::fromJson(keyBuffer).object();
    return IdentityKeys {
        key.value(QStringLiteral("curve25519")).toString().toUtf8(),
        key.value(QStringLiteral("ed25519")).toString().toUtf8()
    };
}

QByteArray QOlmAccount::sign(const QByteArray &message) const
{
    const size_t signatureLength = olm_account_signature_length(m_account);
    QByteArray signatureBuffer(signatureLength, '0');
    const auto error = olm_account_sign(m_account, message.data(), message.length(),
            signatureBuffer.data(), signatureLength);

    if (error == olm_error()) {
        throw lastError(m_account);
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
    const QJsonObject j{ {Curve25519Key, QString(keys.curve25519)}, {Ed25519Key, QString(keys.ed25519)} };
    QJsonDocument doc;
    doc.setObject(j);
    return sign(doc.toJson());

}

size_t QOlmAccount::maxNumberOfOneTimeKeys() const
{
    return olm_account_max_number_of_one_time_keys(m_account);
}

void QOlmAccount::generateOneTimeKeys(size_t numberOfKeys) const
{
    const size_t randomLen = olm_account_generate_one_time_keys_random_length(m_account, numberOfKeys);
    QByteArray randomBuffer = getRandom(randomLen);
    const auto error = olm_account_generate_one_time_keys(m_account, numberOfKeys, randomBuffer.data(), randomLen);

    if (error == olm_error()) {
        throw lastError(m_account);
    }
}

OneTimeKeys QOlmAccount::oneTimeKeys() const
{
    const size_t oneTimeKeyLength = olm_account_one_time_keys_length(m_account);
    QByteArray oneTimeKeysBuffer(oneTimeKeyLength, '0');

    const auto error = olm_account_one_time_keys(m_account, oneTimeKeysBuffer.data(), oneTimeKeyLength);
    if (error == olm_error()) {
        throw lastError(m_account);
    }
    const auto json = QJsonDocument::fromJson(oneTimeKeysBuffer).object();
    OneTimeKeys oneTimeKeys;

    for (const QJsonValue &key1 : json.keys()) {
        auto oneTimeKeyObject = json[key1.toString()].toObject();
        auto keyMap = QMap<QString, QString>();
        for (const QString &key2 : oneTimeKeyObject.keys()) {
            keyMap[key2] = oneTimeKeyObject[key2].toString();
        }
        oneTimeKeys.keys[key1.toString()] = keyMap;
    }
    return oneTimeKeys;
}

QMap<QString, SignedOneTimeKey> QOlmAccount::signOneTimeKeys(const OneTimeKeys &keys) const
{
    QMap<QString, SignedOneTimeKey> signedOneTimeKeys;
    for (const auto &keyid : keys.curve25519().keys()) {
        const auto oneTimeKey = keys.curve25519()[keyid];
        QByteArray sign = signOneTimeKey(oneTimeKey);
        signedOneTimeKeys["signed_curve25519:" + keyid] = signedOneTimeKey(oneTimeKey.toUtf8(), sign);
    }
    return signedOneTimeKeys;
}

SignedOneTimeKey QOlmAccount::signedOneTimeKey(const QByteArray &key, const QString &signature) const
{
    SignedOneTimeKey sign{};
    sign.key = key;
    sign.signatures = {{m_userId, {{"ed25519:" + m_deviceId, signature}}}};
    return sign;
}

QByteArray QOlmAccount::signOneTimeKey(const QString &key) const
{
    QJsonDocument j(QJsonObject{{"key", key}});
    return sign(j.toJson());
}

std::optional<QOlmError> QOlmAccount::removeOneTimeKeys(const std::unique_ptr<QOlmSession> &session) const
{
    const auto error = olm_remove_one_time_keys(m_account, session->raw());

    if (error == olm_error()) {
        return lastError(m_account);
    }
    return std::nullopt;
}

OlmAccount *Quotient::QOlmAccount::data()
{
    return m_account;
}

std::variant<std::unique_ptr<QOlmSession>, QOlmError> QOlmAccount::createInboundSession(const QOlmMessage &preKeyMessage)
{
    Q_ASSERT(preKeyMessage.type() == QOlmMessage::PreKey);
    return QOlmSession::createInboundSession(this, preKeyMessage);
}

std::variant<std::unique_ptr<QOlmSession>, QOlmError> QOlmAccount::createInboundSessionFrom(const QByteArray &theirIdentityKey, const QOlmMessage &preKeyMessage)
{
    Q_ASSERT(preKeyMessage.type() == QOlmMessage::PreKey);
    return QOlmSession::createInboundSessionFrom(this, theirIdentityKey, preKeyMessage);
}

std::variant<std::unique_ptr<QOlmSession>, QOlmError> QOlmAccount::createOutboundSession(const QByteArray &theirIdentityKey, const QByteArray &theirOneTimeKey)
{
    return QOlmSession::createOutboundSession(this, theirIdentityKey, theirOneTimeKey);
}

#endif
