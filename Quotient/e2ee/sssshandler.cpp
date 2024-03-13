// SPDX-FileCopyrightText: 2023 Tobias Fella <tobias.fella@kde.org>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "sssshandler.h"

#include "cryptoutils.h"
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
                                   const QByteArray& decryptionKey)
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

    auto hkdfResult = hkdfSha256(decryptionKey, QByteArray(32, u'\0'), keyType.data());
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

void SSSSHandler::unlockSSSSFromPassword(const QString& password)
{
    Q_ASSERT(m_connection);
    unlockAndLoad(password.toUtf8(), true);
}

void SSSSHandler::unlockSSSSFromCrossSigning()
{
    Q_ASSERT(m_connection);
    m_connection->requestKeyFromDevices(MegolmBackupKey, [this](const QByteArray &key){
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
        auto authData = job->jsonData()["auth_data"_ls].toObject();
        for (const auto& key : authData["signatures"_ls].toObject()[m_connection->userId()].toObject().keys()) {
            const auto& edKey = m_connection->database()->edKeyForKeyId(m_connection->userId(), key);
            if (edKey.isEmpty()) {
                continue;
            }
            const auto& signature = authData["signatures"_ls].toObject()[m_connection->userId()].toObject()[key].toString();
            if (!ed25519VerifySignature(edKey, authData, signature)) {
                qCWarning(E2EE) << "Signature mismatch for" << edKey;
                emit error(InvalidSignatureError);
                return;
            }
        }
        qCDebug(E2EE) << "Loading key backup" << job->jsonData()["version"_ls].toString();
        auto keysJob = m_connection->callApi<GetRoomKeysJob>(job->jsonData()["version"_ls].toString());
        connect(keysJob, &BaseJob::finished, this, [this, keysJob, megolmDecryptionKey](){
            const auto &rooms = keysJob->jsonData()["rooms"_ls].toObject();
            qCDebug(E2EE) << rooms.size() << "rooms in the backup";
            for (const auto& roomId : rooms.keys()) {
                if (!m_connection->room(roomId)) {
                    continue;
                }
                const auto &sessions = rooms[roomId]["sessions"_ls].toObject();
                for (const auto& sessionId : sessions.keys()) {
                    const auto &session = sessions[sessionId].toObject();
                    const auto &sessionData = session["session_data"_ls].toObject();
                    auto result = curve25519AesSha2Decrypt(sessionData["ciphertext"_ls].toString().toLatin1(), megolmDecryptionKey, sessionData["ephemeral"_ls].toString().toLatin1(), sessionData["mac"_ls].toString().toLatin1());
                    if (!result.has_value()) {
                        qCWarning(E2EE) << "Failed to decrypt session" << sessionId;
                        emit error(DecryptionError);
                    }
                    auto data = QJsonDocument::fromJson(result.value()).object();
                    m_connection->room(roomId)->addMegolmSessionFromBackup(sessionId.toLatin1(), data["session_key"_ls].toString().toLatin1(), session["first_message_index"_ls].toInt());
                }
            }
        });
    });
}

void SSSSHandler::unlockAndLoad(QByteArray&& secret, bool requirePassphrase)
{
    const auto& defaultKeyEvent = m_connection->accountData("m.secret_storage.default_key"_ls);
    if (!defaultKeyEvent) {
        qCWarning(E2EE) << "No default secret storage key";
        emit error(NoKeyError);
        return;
    }
    const auto defaultKey = defaultKeyEvent->contentPart<QString>("key"_ls);
    if (!m_connection->hasAccountData("m.secret_storage.key."_ls + defaultKey)) {
        qCWarning(E2EE) << "No account data for key" << ("m.secret_storage.key."_ls + defaultKey);
        emit error(NoKeyError);
        return;
    }
    const auto &keyEvent = m_connection->accountData("m.secret_storage.key."_ls + defaultKey);
    if (keyEvent->contentPart<QString>("algorithm"_ls) != "m.secret_storage.v1.aes-hmac-sha2"_ls) {
        qCWarning(E2EE) << "Unsupported SSSS key algorithm" << keyEvent->contentPart<QString>("algorithm"_ls) << " - aborting.";
        emit error(UnsupportedAlgorithmError);
        return;
    }

    if (requirePassphrase) {
        const auto &passphraseJson = keyEvent->contentPart<QJsonObject>("passphrase"_ls);
        if (passphraseJson["algorithm"_ls].toString() != "m.pbkdf2"_ls) {
            qCWarning(E2EE) << "Unsupported SSSS passphrase algorithm" << passphraseJson["algorithm"_ls].toString() << " - aborting.";
            emit error(UnsupportedAlgorithmError);
            return;
        }

        auto&& result = pbkdf2HmacSha512(secret, passphraseJson["salt"_ls].toString().toLatin1(), passphraseJson["iterations"_ls].toInt(), 32);
        if (!result.has_value()) {
            qCWarning(E2EE) << "Failed to calculate pbkdf";
            emit error(DecryptionError);
            return;
        }
        secret = std::move(result.value());
    }

    const auto& testKeys = hkdfSha256(secret, QByteArray(32, u'\0'), {});
    if (!testKeys.has_value()) {
        qCWarning(E2EE) << "Failed to calculate hkdf";
        emit error(DecryptionError);
        return;
    }
    const auto& encrypted = aesCtr256Encrypt(
        QByteArray(32, u'\0'), testKeys.value().aes(),
        asCBytes<AesBlockSize>(QByteArray::fromBase64(
            keyEvent->contentPart<QString>("iv"_ls).toLatin1())));
    if (!encrypted.has_value()) {
        qCWarning(E2EE) << "Failed to encrypt test keys";
        emit error(DecryptionError);
        return;
    }
    const auto &result = hmacSha256(testKeys.value().mac(), encrypted.value());
    if (!result.has_value()) {
        qCWarning(E2EE) << "Failed to calculate hmac";
        emit error(DecryptionError);
        return;
    }
    if (result.value() != QByteArray::fromBase64(keyEvent->contentPart<QString>("mac"_ls).toLatin1())) {
        qCWarning(E2EE) << "MAC mismatch for secret storage key";
        emit error(WrongKeyError);
        return;
    }

    emit keyBackupUnlocked();

    auto megolmDecryptionKey = decryptKey(MegolmBackupKey, defaultKey, secret);

    // These keys are only decrypted since this will automatically store them locally
    decryptKey(CrossSigningSelfSigningKey, defaultKey, secret);
    decryptKey(CrossSigningUserSigningKey, defaultKey, secret);
    decryptKey(CrossSigningMasterKey, defaultKey, secret);
    if (megolmDecryptionKey.isEmpty()) {
        qCWarning(E2EE) << "No megolm decryption key";
        emit error(NoKeyError);
        return;
    }
    loadMegolmBackup(megolmDecryptionKey);
}

void SSSSHandler::unlockSSSSFromSecurityKey(const QString& key)
{
    auto securityKey = key;
    securityKey.remove(u' ');
    auto&& decoded = base58Decode(securityKey.toLatin1());
    unsigned char parity = 0;
    for (const auto b : decoded) {
        parity ^= b;
    }
    if (parity != 0) {
        qCWarning(E2EE) << "Invalid parity byte";
        emit error(WrongKeyError);
        return;
    }
    if (static_cast<uint8_t>(decoded[0]) != 0x8B || static_cast<uint8_t>(decoded[1]) != 0x01) {
        qCWarning(E2EE) << "Invalid prefix";
        emit error(WrongKeyError);
        return;
    }
    if (decoded.size() != 32 + 3) {
        qCWarning(E2EE) << "Incorrect length";
        emit error(WrongKeyError);
        return;
    }
    decoded = decoded.mid(2);
    decoded.chop(1);

    unlockAndLoad(std::move(decoded), false);
}
