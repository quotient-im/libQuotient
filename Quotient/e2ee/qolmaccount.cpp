// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "qolmaccount.h"

#include "../logging_categories_p.h"
#include "qolmsession.h"
#include "qolmutility.h"

#include "../csapi/keys.h"

#include <QtCore/QRandomGenerator>
#include <QtCore/QTimer>

#include <span>

#include <vodozemac/vodozemac.h>

using namespace Quotient;

QOlmAccount::QOlmAccount(QObject* parent)
    : QObject(parent)
    , olmData(olm::new_account())
{}

QOlmAccount::QOlmAccount(rust::Box<olm::Account> account, QObject* parent)
    : QObject(parent)
    , olmData(std::move(account))
{}

QOlmAccount *QOlmAccount::newAccount(QObject *parent, const QString& userId, const QString& deviceId)
{
    auto account = new QOlmAccount(parent);
    account->m_userId = userId;
    account->m_deviceId = deviceId;
    QTimer::singleShot(0, account, &QOlmAccount::needsSave);
    return account;
}

//TODO set user and device id
QOlmAccount *QOlmAccount::unpickle(QByteArray&& pickled, const PicklingKey& key, QObject *parent)
{
    //TODO: This is terrible :(
    std::array<std::uint8_t, 32> _key;
    std::copy(key.data(), key.data() + 32, _key.begin());
    auto account = olm::account_from_pickle(rust::String(pickled.data(), pickled.size()), _key);
    return new QOlmAccount(std::move(account));
}

QByteArray QOlmAccount::pickle(const PicklingKey& key) const
{
    //TODO: This is terrible :(
    std::array<std::uint8_t, 32> _key;
    std::copy(key.data(), key.data() + 32, _key.begin());
    auto pickle = olmData->pickle(_key);
    return {pickle.data(), (qsizetype) pickle.size()};
}

IdentityKeys QOlmAccount::identityKeys() const
{
    const auto &edKeyRS = olmData->ed25519_key()->to_base64();
    const auto &curveKeyRS = olmData->curve25519_key()->to_base64();

    auto edKey = QString::fromLatin1(QByteArray(edKeyRS.data(), edKeyRS.size()));
    auto curveKey = QString::fromLatin1(QByteArray(curveKeyRS.data(), curveKeyRS.size()));

    return { curveKey, edKey };
}

QByteArray QOlmAccount::sign(const QByteArray &message) const
{
    auto signature = olmData->sign(::rust::Str(message.data(), message.size()))->to_base64();
    return {signature.data(), (qsizetype) signature.size()};
}

QByteArray QOlmAccount::sign(const QJsonObject &message) const
{
    return sign(QJsonDocument(message).toJson(QJsonDocument::Compact));
}

QByteArray QOlmAccount::signIdentityKeys() const
{
    const auto keys = identityKeys();
    static const auto& Algorithms = toJson(SupportedAlgorithms);
    return sign(
        QJsonObject{ { u"algorithms"_s, Algorithms },
                     { u"user_id"_s, m_userId },
                     { u"device_id"_s, m_deviceId },
                     { u"keys"_s, QJsonObject{ { "curve25519:"_L1 + m_deviceId, keys.curve25519 },
                                               { "ed25519:"_L1 + m_deviceId, keys.ed25519 } } } });
}

size_t QOlmAccount::maxNumberOfOneTimeKeys() const
{
    return olmData->max_number_of_one_time_keys();
}

void QOlmAccount::generateOneTimeKeys(size_t numberOfKeys)
{
    olmData->generate_one_time_keys(numberOfKeys);
}

UnsignedOneTimeKeys QOlmAccount::oneTimeKeys() const
{
    const auto keys = olmData->one_time_keys();

    UnsignedOneTimeKeys oneTimeKeys;
    for (size_t i = 0; i < keys.size(); i++) {
        auto id = keys[i].key_id;
        auto key = keys[i].key->to_base64();
        oneTimeKeys.keys[QString::fromLatin1(QByteArray(id.data(), id.size()))] = QString::fromLatin1(QByteArray(key.data(), key.size()));
    }
    return oneTimeKeys;
}

OneTimeKeys QOlmAccount::signOneTimeKeys(const UnsignedOneTimeKeys &keys) const
{
    OneTimeKeys signedOneTimeKeys;
    for (const auto& [keyId, key] : asKeyValueRange(keys.keys))
        signedOneTimeKeys.insert("signed_curve25519:"_ls % keyId,
                                 SignedOneTimeKey {
                                     key, m_userId, m_deviceId,
                                     sign(QJsonObject { { "key"_ls, key } }) });

    return signedOneTimeKeys;
}

DeviceKeys QOlmAccount::deviceKeys() const
{
    static const QStringList Algorithms(SupportedAlgorithms.cbegin(),
                                        SupportedAlgorithms.cend());

    const auto idKeys = identityKeys();
    return DeviceKeys{
        .userId = m_userId,
        .deviceId = m_deviceId,
        .algorithms = Algorithms,
        .keys{ { "curve25519:"_L1 + m_deviceId, idKeys.curve25519 },
               { "ed25519:"_L1 + m_deviceId, idKeys.ed25519 } },
        .signatures{ { m_userId,
                       { { "ed25519:"_L1 + m_deviceId,
                           QString::fromLatin1(signIdentityKeys()) } } } }
    };
}

UploadKeysJob* QOlmAccount::createUploadKeyRequest(
    const UnsignedOneTimeKeys& oneTimeKeys) const
{
    return new UploadKeysJob(deviceKeys(), signOneTimeKeys(oneTimeKeys));
}

QOlmSession QOlmAccount::createInboundSession(
    const QByteArray& theirIdentityKey, const QOlmMessage& preKeyMessage)
{
    rust::Str theirIdentityKeyRS(theirIdentityKey.data(), theirIdentityKey.length());
    olm::OlmMessageParts parts(preKeyMessage.type(), ::rust::String(preKeyMessage.data(), preKeyMessage.length()));
    auto theirIdentityKeyResult = types::curve25519_public_key_from_base64(theirIdentityKeyRS);
    //TODO theirIdentityKeyResult error handling
    qWarning() << "Creating inbound session";
    auto result = olmData->create_inbound_session(*types::curve25519_public_key_from_base64_result_value(std::move(theirIdentityKeyResult)), *olm_message_from_parts(parts));

    return QOlmSession(std::move(result.session));
}


QOlmSession QOlmAccount::createOutboundSession(
    const QByteArray& theirIdentityKey, const QByteArray& theirOneTimeKey)
{
    ::rust::Str theirIdentityKeyRS(theirIdentityKey.data(), theirIdentityKey.length());
    auto theirIdentityKeyResult = types::curve25519_public_key_from_base64(theirIdentityKeyRS);
    //TODO theirIdentityKeyResult error handling

    ::rust::Str theirOneTimeKeyRS(theirOneTimeKey.data(), theirOneTimeKey.length());
    auto theirOneTimeKeyResult = types::curve25519_public_key_from_base64(theirOneTimeKeyRS);
    //TODO theirOneTimeKeyResult error handling
    auto session = olmData->create_outbound_session(*types::curve25519_public_key_from_base64_result_value(std::move(theirIdentityKeyResult)), *types::curve25519_public_key_from_base64_result_value(std::move(theirOneTimeKeyResult)));

    return QOlmSession(std::move(session));
}

void QOlmAccount::markKeysAsPublished()
{
    olmData->mark_keys_as_published();
    emit needsSave();
}

bool Quotient::verifyIdentitySignature(const DeviceKeys& deviceKeys,
                                       const QString& deviceId,
                                       const QString& userId)
{
    const auto signKeyId = "ed25519:"_L1 + deviceId;
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

    obj1.remove("unsigned"_L1);
    obj1.remove("signatures"_L1);

    auto canonicalJson = QJsonDocument(obj1).toJson(QJsonDocument::Compact);

    QByteArray signingKeyBuf = signingKey.toUtf8();
    QOlmUtility utility;
    auto signatureBuf = signature.toUtf8();
    return utility.ed25519Verify(signingKeyBuf, canonicalJson, signatureBuf);
}

QString QOlmAccount::accountId() const { return m_userId % u'/' % m_deviceId; }
