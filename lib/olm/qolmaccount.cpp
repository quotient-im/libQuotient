// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifdef Quotient_E2EE_ENABLED
#include "qolmaccount.h"
#include <QJsonObject>
#include <QJsonDocument>
#include <QDebug>
#include <iostream>

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

// Convert PicklingMode to key
QByteArray toKey(PicklingMode mode)
{
    if (std::holds_alternative<Unencrypted>(mode)) {
        return "";
    }
    return std::get<Encrypted>(mode).key;
}

bool operator==(const IdentityKeys& lhs, const IdentityKeys& rhs)
{
    return lhs.curve25519 == rhs.curve25519 &&& lhs.ed25519 == rhs.ed25519;
}

// Conver olm error to enum
QOlmAccount::OlmAccountError lastError(OlmAccount *account) {
    const std::string error_raw = olm_account_last_error(account);

    if (error_raw.compare("BAD_ACCOUNT_KEY")) {
        return QOlmAccount::OlmAccountError::BadAccountKey;
    } else if (error_raw.compare("BAD_MESSAGE_KEY_ID")) {
        return QOlmAccount::OlmAccountError::BadMessageKeyId;
    } else if (error_raw.compare("INVALID_BASE64")) {
        return QOlmAccount::OlmAccountError::InvalidBase64;
    } else if (error_raw.compare("NOT_ENOUGH_RANDOM")) {
        return QOlmAccount::OlmAccountError::NotEnoughRandom;
    } else if (error_raw.compare("OUTPUT_BUFFER_TOO_SMALL")) {
        return QOlmAccount::OlmAccountError::OutputBufferTooSmall;
    } else {
        return QOlmAccount::OlmAccountError::Unknown;
    }
}

QByteArray getRandom(size_t bufferSize)
{
    QByteArray buffer(bufferSize, '0');
    std::generate(buffer.begin(), buffer.end(), std::rand);
    return buffer;
}

QOlmAccount::QOlmAccount(OlmAccount *account)
    : m_account(account)
{}

std::optional<QOlmAccount> QOlmAccount::create()
{
    auto account = olm_account(new uint8_t[olm_account_size()]);
    size_t randomSize = olm_create_account_random_length(account);
    QByteArray randomData = getRandom(randomSize);
    const auto error = olm_create_account(account, randomData.data(), randomSize);
    if (error == olm_error()) {
        return std::nullopt;
    }
    return std::make_optional<QOlmAccount>(account);
}

std::variant<QOlmAccount, QOlmAccount::OlmAccountError> QOlmAccount::unpickle(QByteArray pickled, PicklingMode mode)
{
    auto account = olm_account(new uint8_t[olm_account_size()]);
    const QByteArray key = toKey(mode);
    const auto error = olm_unpickle_account(account, key.data(), key.length(), pickled.data(), pickled.size());
    if (error == olm_error()) {
        return lastError(account);
    }
    return QOlmAccount(account);
}

std::variant<QByteArray, QOlmAccount::OlmAccountError> QOlmAccount::pickle(PicklingMode mode)
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

std::variant<IdentityKeys, QOlmAccount::OlmAccountError> QOlmAccount::identityKeys()
{
    const size_t keyLength = olm_account_identity_keys_length(m_account);
    QByteArray keyBuffer(keyLength, '0');
    const auto error = olm_account_identity_keys(m_account, keyBuffer.data(), keyLength);
    if (error == olm_error()) {
        return lastError(m_account);
    }
    const QJsonObject key = QJsonDocument::fromJson(keyBuffer).object();
    return IdentityKeys {
        key.value(QStringLiteral("curve25519")).toString().toUtf8(),
        key.value(QStringLiteral("ed25519")).toString().toUtf8()
    };
}

std::variant<QString, QOlmAccount::OlmAccountError> QOlmAccount::sign(QString message) const
{
    const size_t signatureLength = olm_account_signature_length(m_account);
    QByteArray signatureBuffer(signatureLength, '0');
    const auto error = olm_account_sign(m_account, message.data(), message.length(),
            signatureBuffer.data(), signatureLength);

    if (error == olm_error()) {
        return lastError(m_account);
    }
    return QString::fromUtf8(signatureBuffer);
}

size_t QOlmAccount::maxNumberOfOneTimeKeys() const
{
    return olm_account_max_number_of_one_time_keys(m_account);
}

std::optional<QOlmAccount::OlmAccountError> QOlmAccount::generateOneTimeKeys(size_t numberOfKeys) const
{
    const size_t randomLen = olm_account_generate_one_time_keys_random_length(m_account, numberOfKeys);
    QByteArray randomBuffer = getRandom(randomLen);
    const auto error = olm_account_generate_one_time_keys(m_account, numberOfKeys, randomBuffer.data(), randomLen);

    if (error == olm_error()) {
        return lastError(m_account);
    }
    return std::nullopt;
}

std::variant<OneTimeKeys, QOlmAccount::OlmAccountError> QOlmAccount::oneTimeKeys() const
{
    const size_t oneTimeKeyLength = olm_account_one_time_keys_length(m_account);
    QByteArray oneTimeKeysBuffer(oneTimeKeyLength, '0');

    const auto error = olm_account_one_time_keys(m_account, oneTimeKeysBuffer.data(), oneTimeKeyLength);
    if (error == olm_error()) {
        return lastError(m_account);
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

#endif
