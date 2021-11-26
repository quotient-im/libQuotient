// SPDX-FileCopyrightText: 2019 Alexey Andreyev <aa13q@ya.ru>
// SPDX-FileCopyrightText: 2019 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifdef Quotient_E2EE_ENABLED
#include "encryptionmanager.h"

#include "connection.h"
#include "crypto/e2ee.h"
#include "events/encryptedfile.h"

#include "csapi/keys.h"

#include <QtCore/QHash>
#include <QtCore/QStringBuilder>
#include <QtCore/QCryptographicHash>

#include "crypto/qolmaccount.h"
#include "crypto/qolmsession.h"
#include "crypto/qolmmessage.h"
#include "crypto/qolmerrors.h"
#include "crypto/qolmutils.h"
#include <functional>
#include <memory>

#include <openssl/evp.h>

using namespace Quotient;
using std::move;

class EncryptionManager::Private {
public:
    explicit Private()
    {
    }
    ~Private() = default;

    EncryptionManager* q;

    // A map from senderKey to InboundSession
    UnorderedMap<QString, std::unique_ptr<QOlmSession>> sessions; // TODO: cache
    void updateDeviceKeys(
        const QHash<QString,
                    QHash<QString, QueryKeysJob::DeviceInformation>>& deviceKeys)
    {
        for (auto userId : deviceKeys.keys()) {
            for (auto deviceId : deviceKeys.value(userId).keys()) {
                auto info = deviceKeys.value(userId).value(deviceId);
                // TODO: ed25519Verify, etc
            }
        }
    }
    void loadSessions() {
        QFile file { static_cast<Connection *>(q->parent())->e2eeDataDir() % "/olmsessions.json" };
        if(!file.exists() || !file.open(QIODevice::ReadOnly)) {
            qCDebug(E2EE) << "No sessions cache exists.";
            return;
        }
        auto data = file.readAll();
        const auto json = data.startsWith('{')
            ? QJsonDocument::fromJson(data).object()
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
            : QCborValue::fromCbor(data).toJsonValue().toObject()
#else
            : QJsonDocument::fromBinaryData(data).object()
#endif
            ;
        if (json.isEmpty()) {
            qCWarning(MAIN) << "Sessions cache is empty";
            return;
        }
        for(const auto &senderKey : json["sessions"].toObject().keys()) {
            auto pickle = json["sessions"].toObject()[senderKey].toString();
            auto sessionResult = QOlmSession::unpickle(pickle.toLatin1(), static_cast<Connection *>(q->parent())->picklingMode());
            if(std::holds_alternative<QOlmError>(sessionResult)) {
                qCWarning(E2EE) << "Failed to unpickle olm session";
                continue;
            }
            sessions[senderKey] = std::move(std::get<std::unique_ptr<QOlmSession>>(sessionResult));
        }
    }
    void saveSessions() {
        QFile outFile { static_cast<Connection *>(q->parent())->e2eeDataDir() + QStringLiteral("/olmsessions.json") };
        if (!outFile.open(QFile::WriteOnly)) {
            qCWarning(E2EE) << "Error opening" << outFile.fileName() << ":"
                            << outFile.errorString();
            qCWarning(E2EE) << "Failed to write olm sessions";
            return;
        }

        QJsonObject rootObj {
            { QStringLiteral("cache_version"),
            QJsonObject {
                { QStringLiteral("major"), 1 },
                { QStringLiteral("minor"), 0 } } }
        };
        {
            QJsonObject sessionsJson;
            for (const auto &session : sessions) {
                auto pickleResult = session.second->pickle(static_cast<Connection *>(q->parent())->picklingMode());
                if(std::holds_alternative<QOlmError>(pickleResult)) {
                    qCWarning(E2EE) << "Failed to pickle session";
                    continue;
                }
                sessionsJson[session.first] = QString(std::get<QByteArray>(pickleResult));
            }
            rootObj.insert(QStringLiteral("sessions"), sessionsJson);
        }

    #if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
        const auto data = QJsonDocument(rootObj).toJson(QJsonDocument::Compact);
    #else
        QJsonDocument json { rootObj };
        const auto data = json.toJson(QJsonDocument::Compact);
    #endif

        outFile.write(data.data(), data.size());
        qCDebug(E2EE) << "Sessions saved to" << outFile.fileName();
    }
    QString sessionDecryptPrekey(const QOlmMessage& message, const QString &senderKey, std::unique_ptr<QOlmAccount>& olmAccount)
    {
        Q_ASSERT(message.type() == QOlmMessage::PreKey);
        for(auto& session : sessions) {
            const auto matches = session.second->matchesInboundSessionFrom(senderKey, message);
            if(std::holds_alternative<bool>(matches) && std::get<bool>(matches)) {
                qCDebug(E2EE) << "Found inbound session";
                const auto result = session.second->decrypt(message);
                saveSessions();
                if(std::holds_alternative<QString>(result)) {
                    return std::get<QString>(result);
                } else {
                    qCDebug(E2EE) << "Failed to decrypt prekey message";
                    return {};
                }
            }
        }
        qCDebug(E2EE) << "Creating new inbound session";
        auto newSessionResult = olmAccount->createInboundSessionFrom(senderKey.toUtf8(), message);
        if(std::holds_alternative<QOlmError>(newSessionResult)) {
            qCWarning(E2EE) << "Failed to create inbound session for" << senderKey << std::get<QOlmError>(newSessionResult);
            return {};
        }
        std::unique_ptr<QOlmSession> newSession = std::move(std::get<std::unique_ptr<QOlmSession>>(newSessionResult));
        // TODO Error handling?
        olmAccount->removeOneTimeKeys(newSession);
        const auto result = newSession->decrypt(message);
        sessions[senderKey] = std::move(newSession);
        saveSessions();
        if(std::holds_alternative<QString>(result)) {
            return std::get<QString>(result);
        } else {
            qCDebug(E2EE) << "Failed to decrypt prekey message with new session";
            return {};
        }
    }
    QString sessionDecryptGeneral(const QOlmMessage& message, const QString &senderKey)
    {
        Q_ASSERT(message.type() == QOlmMessage::General);
        for(auto& session : sessions) {
            const auto result = session.second->decrypt(message);
            if(std::holds_alternative<QString>(result)) {
                saveSessions();
                return std::get<QString>(result);
            }
        }
        qCWarning(E2EE) << "Failed to decrypt message";
        return {};
    }
};

EncryptionManager::EncryptionManager(QObject* parent)
    : QObject(parent)
    , d(std::make_unique<Private>())
{
    d->q = this;
    d->loadSessions();
}

EncryptionManager::~EncryptionManager() = default;

QString EncryptionManager::sessionDecryptMessage(
    const QJsonObject& personalCipherObject, const QByteArray& senderKey, std::unique_ptr<QOlmAccount>& account)
{
    QString decrypted;
    int type = personalCipherObject.value(TypeKeyL).toInt(-1);
    QByteArray body = personalCipherObject.value(BodyKeyL).toString().toLatin1();
    if (type == 0) {
        QOlmMessage preKeyMessage(body, QOlmMessage::PreKey);
        decrypted = d->sessionDecryptPrekey(preKeyMessage, senderKey, account);
    } else if (type == 1) {
        QOlmMessage message(body, QOlmMessage::General);
        decrypted = d->sessionDecryptGeneral(message, senderKey);
    }
    return decrypted;
}

QByteArray EncryptionManager::decryptFile(const QByteArray &ciphertext, EncryptedFile* file)
{
    const auto key = QByteArray::fromBase64(file->key.k.replace(QLatin1Char('_'), QLatin1Char('/')).replace(QLatin1Char('-'), QLatin1Char('+')).toLatin1());
    const auto iv =  QByteArray::fromBase64(file->iv.toLatin1());
    const auto sha256 = QByteArray::fromBase64(file->hashes["sha256"].toLatin1());
    if(sha256 != QCryptographicHash::hash(ciphertext, QCryptographicHash::Sha256)) {
        qCWarning(E2EE) << "Hash verification failed for file";
        return QByteArray();
    }
    QByteArray plaintext(ciphertext.size(), 0);
    EVP_CIPHER_CTX *ctx;
    int length;
    ctx = EVP_CIPHER_CTX_new();
    EVP_DecryptInit_ex(ctx, EVP_aes_256_ctr(), NULL, (const unsigned char *)key.data(), (const unsigned char *)iv.data());
    EVP_DecryptUpdate(ctx, (unsigned char *)plaintext.data(), &length, (const unsigned char *)ciphertext.data(), ciphertext.size());
    EVP_DecryptFinal_ex(ctx, (unsigned char *)plaintext.data() + length, &length);
    EVP_CIPHER_CTX_free(ctx);
    return plaintext;
}
#endif // Quotient_E2EE_ENABLED
