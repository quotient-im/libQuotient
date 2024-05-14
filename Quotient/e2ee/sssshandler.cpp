// SPDX-FileCopyrightText: 2023 Tobias Fella <tobias.fella@kde.org>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "sssshandler.h"

#include "qolmaccount.h"

#include "../csapi/key_backup.h"

#include "../events/event.h"

#include "../database.h"
#include "../logging_categories_p.h"
#include "../room.h"

using namespace Quotient;

namespace {
constexpr inline auto MegolmBackupKey = "m.megolm_backup.v1"_ls;
constexpr inline auto CrossSigningMasterKey = "m.cross_signing.master"_ls;
constexpr inline auto CrossSigningSelfSigningKey = "m.cross_signing.self_signing"_ls;
constexpr inline auto CrossSigningUserSigningKey = "m.cross_signing.user_signing"_ls;
}

QByteArray SSSSHandler::decryptKey(event_type_t keyType, const QString& defaultKey,
                                   key_view_t decryptionKey)
{
    Q_ASSERT(m_connection);
    const auto& encryptedKeyObject = m_connection->accountData(keyType);
    if (!encryptedKeyObject) {
        qWarning() << "No account data for key" << keyType;
        emit error(NoKeyError);
        return {};
    }
    const auto& encrypted =
        encryptedKeyObject->contentPart<QJsonObject>("encrypted"_ls).value(defaultKey).toObject();

    auto hkdfResult = hkdfSha256(decryptionKey, zeroes<32>(), asCBytes<>(keyType));
    if (!hkdfResult.has_value()) {
        qCWarning(E2EE) << "Failed to calculate HKDF for" << keyType;
        emit error(DecryptionError);
    }
    const auto& keys = hkdfResult.value();

    auto rawCipher = QByteArray::fromBase64(encrypted["ciphertext"_ls].toString().toLatin1());
    auto result = hmacSha256(keys.mac(), rawCipher);
    if (!result.has_value()) {
        qCWarning(E2EE) << "Failed to calculate HMAC for" << keyType;
        emit error(DecryptionError);
    }
    if (QString::fromLatin1(result.value().toBase64()) != encrypted["mac"_ls].toString()) {
        qCWarning(E2EE) << "MAC mismatch for" << keyType;
        emit error(DecryptionError);
        return {};
    }
    auto decryptResult =
        aesCtr256Decrypt(rawCipher, keys.aes(),
                         asCBytes<AesBlockSize>(QByteArray::fromBase64(
                             encrypted["iv"_ls].toString().toLatin1())));
    if (!decryptResult.has_value()) {
        qCWarning(E2EE) << "Failed to decrypt for" << keyType;
        emit error(DecryptionError);
    }
    auto key = QByteArray::fromBase64(decryptResult.value());
    m_connection->database()->storeEncrypted(keyType, key);
    return key;
}

class AesHmacSha2KeyDescription : public Event {
public:
    // No QUO_EVENT because the account data "type id" is variable
    QUO_CONTENT_GETTER(QString, algorithm)
    QUO_CONTENT_GETTER(QJsonObject, passphrase)
    QUO_CONTENT_GETTER(QString, iv)
    QUO_CONTENT_GETTER(QString, mac)
};

struct SSSSHandler::UnlockData {
    static Expected<UnlockData, Error> prepare(const Connection* c)
    {
        Q_ASSERT(c);

        const auto& defaultKeyEvent = c->accountData("m.secret_storage.default_key"_ls);
        if (!defaultKeyEvent) {
            qCWarning(E2EE) << "SSSS: No default secret storage key";
            return NoKeyError;
        }
        auto defaultKey = defaultKeyEvent->contentPart<QString>("key"_ls);
        const auto keyName = "m.secret_storage.key."_ls + defaultKey;
        auto* const keyDescription = c->accountData<AesHmacSha2KeyDescription>(keyName);
        if (!keyDescription) {
            qCWarning(E2EE) << "SSSS: No account data for key" << keyName;
            return NoKeyError;
        }

        if (keyDescription->algorithm() != "m.secret_storage.v1.aes-hmac-sha2"_ls) {
            qCWarning(E2EE) << "Unsupported SSSS key algorithm" << keyDescription->algorithm()
                            << " - aborting.";
            return UnsupportedAlgorithmError;
        }
        auto iv = QByteArray::fromBase64Encoding(keyDescription->iv().toLatin1());
        if (!iv || iv.decoded.isEmpty() || iv.decoded.size() != AesBlockSize) {
            qCWarning(E2EE) << "SSSS: Malformed or empty IV";
            return DecryptionError;
        }
        auto&& mac = QByteArray::fromBase64Encoding(keyDescription->mac().toLatin1());
        if (!mac || mac.decoded.isEmpty()) {
            qCWarning(E2EE) << "SSSS: Failed to decode expected MAC or it is empty";
            return DecryptionError;
        }
        return UnlockData{ std::move(defaultKey), keyDescription->passphrase(), std::move(*iv),
                           std::move(*mac) };
    }

    QString defaultKey;
    QJsonObject passphraseInfo;
    QByteArray decodedIV;
    QByteArray decodedMac;
};

void SSSSHandler::unlockSSSSWithPassphrase(const QString& passphrase)
{
    const auto unlockData = UnlockData::prepare(m_connection);
    if (!unlockData) {
        emit error(unlockData.error());
        return;
    }
    if (unlockData->passphraseInfo["algorithm"_ls].toString() != "m.pbkdf2"_ls) {
        qCWarning(E2EE) << "Unsupported SSSS passphrase algorithm"
                        << unlockData->passphraseInfo["algorithm"_ls].toString() << " - aborting.";
        emit error(UnsupportedAlgorithmError);
        return;
    }
    if (const auto& result =
            pbkdf2HmacSha512(viewAsByteArray(passphrase.toUtf8()),
                             unlockData->passphraseInfo["salt"_ls].toString().toLatin1(),
                             unlockData->passphraseInfo["iterations"_ls].toInt())) {
        unlockAndLoad(*unlockData, *result);
        return;
    }
    qCWarning(E2EE) << "Failed to calculate PBKDF";
    emit error(DecryptionError);
    return;
}

void SSSSHandler::unlockSSSSFromCrossSigning()
{
    Q_ASSERT(m_connection);
    m_connection->requestKeyFromDevices(MegolmBackupKey).then([this](const QByteArray& key) {
        loadMegolmBackup(key);
    });
    for (auto k : {CrossSigningUserSigningKey, CrossSigningSelfSigningKey, CrossSigningMasterKey})
        m_connection->requestKeyFromDevices(k);
}

Connection* SSSSHandler::connection() const
{
    return m_connection;
}

void SSSSHandler::setConnection(Connection* connection)
{
    if (connection == m_connection) {
        return;
    }
    m_connection = connection;
    emit connectionChanged();
}

void SSSSHandler::loadMegolmBackup(const QByteArray& megolmDecryptionKey)
{
    auto job = m_connection->callApi<GetRoomKeysVersionCurrentJob>();
    connect(job, &BaseJob::finished, this, [this, job, megolmDecryptionKey] {
        m_connection->database()->storeEncrypted("etag"_ls, job->etag().toLatin1());
        auto authData = job->authData();
        const auto& userSignaturesObject =
            authData["signatures"_ls].toObject().value(m_connection->userId()).toObject();
        for (auto it = userSignaturesObject.constBegin(); it != userSignaturesObject.constEnd();
             ++it) {
            const auto& edKey =
                m_connection->database()->edKeyForKeyId(m_connection->userId(), it.key());
            if (edKey.isEmpty()) {
                continue;
            }
            const auto& signature = it.value().toString();
            if (!ed25519VerifySignature(edKey, authData, signature)) {
                qCWarning(E2EE) << "Signature mismatch for" << edKey;
                emit error(InvalidSignatureError);
                return;
            }
        }
        qCDebug(E2EE) << "Loading key backup" << job->version();
        auto keysJob = m_connection->callApi<GetRoomKeysJob>(job->version());
        connect(keysJob, &BaseJob::finished, this, [this, keysJob, megolmDecryptionKey](){
            const auto &rooms = keysJob->rooms();
            qCDebug(E2EE) << rooms.size() << "rooms in the backup";
            for (const auto& [roomId, roomKeyBackup] : rooms.asKeyValueRange()) {
                if (!m_connection->room(roomId))
                    continue;

                for (const auto& [sessionId, backupData] :
                     roomKeyBackup.sessions.asKeyValueRange()) {
                    const auto& sessionData = backupData.sessionData;
                    const auto result =
                        curve25519AesSha2Decrypt(sessionData["ciphertext"_ls].toString().toLatin1(),
                                                 megolmDecryptionKey,
                                                 sessionData["ephemeral"_ls].toString().toLatin1(),
                                                 sessionData["mac"_ls].toString().toLatin1());
                    if (!result.has_value()) {
                        qCWarning(E2EE) << "Failed to decrypt session" << sessionId;
                        emit error(DecryptionError);
                        continue;
                    }
                    const auto data = QJsonDocument::fromJson(result.value()).object();
                    m_connection->room(roomId)->addMegolmSessionFromBackup(
                        sessionId.toLatin1(), data["session_key"_ls].toString().toLatin1(),
                        static_cast<uint32_t>(backupData.firstMessageIndex), data["sender_key"_ls].toVariant().toByteArray());
                }
            }
            emit finished();
        });
    });
}

void SSSSHandler::unlockAndLoad(const UnlockData& unlockData, key_view_t decryptingKey)
{
    const auto& testKeys = hkdfSha256(decryptingKey, zeroes<32>(), {});
    if (!testKeys.has_value()) {
        qCWarning(E2EE) << "SSSS: Failed to calculate HKDF";
        emit error(DecryptionError);
        return;
    }
    const auto& encrypted = aesCtr256Encrypt(zeroedByteArray(), testKeys.value().aes(),
                                             asCBytes<AesBlockSize>(unlockData.decodedIV));
    if (!encrypted.has_value()) {
        qCWarning(E2EE) << "SSSS: Failed to encrypt test keys";
        emit error(DecryptionError);
        return;
    }
    const auto &result = hmacSha256(testKeys.value().mac(), encrypted.value());
    if (!result.has_value()) {
        qCWarning(E2EE) << "SSSS: Failed to calculate HMAC";
        emit error(DecryptionError);
        return;
    }
    if (result.value() != unlockData.decodedMac) {
        qCWarning(E2EE) << "SSSS: MAC mismatch for secret storage test key";
        emit error(WrongKeyError);
        return;
    }

    emit keyBackupUnlocked();

    auto megolmDecryptionKey = decryptKey(MegolmBackupKey, unlockData.defaultKey, decryptingKey);
    if (megolmDecryptionKey.isEmpty()) {
        qCWarning(E2EE) << "SSSS: No megolm decryption key";
        emit error(NoKeyError);
        return;
    }
    loadMegolmBackup(megolmDecryptionKey);

    // These keys are only decrypted since this will automatically store them locally
    decryptKey(CrossSigningSelfSigningKey, unlockData.defaultKey, decryptingKey);
    decryptKey(CrossSigningUserSigningKey, unlockData.defaultKey, decryptingKey);
    decryptKey(CrossSigningMasterKey, unlockData.defaultKey, decryptingKey);
}

void SSSSHandler::unlockSSSSFromSecurityKey(const QString& encodedKey)
{
    auto securityKey = encodedKey;
    securityKey.remove(u' ');
    auto&& decoded = base58Decode(securityKey.toLatin1());
    if (decoded.size() != DefaultPbkdf2KeyLength + 3) {
        qCWarning(E2EE) << "SSSS: Incorrect decryption key length";
        emit error(WrongKeyError);
        return;
    }
    if (decoded.front() != 0x8B || decoded[1] != 0x01) {
        qCWarning(E2EE) << "SSSS: invalid prefix in the decryption key";
        emit error(WrongKeyError);
        return;
    }
    if (std::accumulate(decoded.cbegin(), decoded.cend(), uint8_t{ 0 }, std::bit_xor<>()) != 0) {
        qCWarning(E2EE) << "SSSS: invalid parity byte in the decryption key";
        emit error(WrongKeyError);
        return;
    }
    const auto unlockData = UnlockData::prepare(m_connection);
    if (!unlockData) {
        emit error(unlockData.error());
        return;
    }
    unlockAndLoad(*unlockData, byte_view_t<>(decoded).subspan<2, DefaultPbkdf2KeyLength>());
}
