// SPDX-FileCopyrightText: 2019 Alexey Andreyev <aa13q@ya.ru>
// SPDX-FileCopyrightText: 2019 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifdef Quotient_E2EE_ENABLED
#include "encryptionmanager.h"

#include "connection.h"
#include "crypto/e2ee.h"

#include "csapi/keys.h"

#include <QtCore/QHash>
#include <QtCore/QStringBuilder>

#include "crypto/qolmaccount.h"
#include "crypto/qolmsession.h"
#include "crypto/qolmmessage.h"
#include "crypto/qolmerrors.h"
#include "crypto/qolmutils.h"
#include <functional>
#include <memory>

using namespace Quotient;
using std::move;

class EncryptionManager::Private {
public:
    explicit Private(const QByteArray& encryptionAccountPickle,
                     float signedKeysProportion, float oneTimeKeyThreshold)
        : q(nullptr)
        , signedKeysProportion(move(signedKeysProportion))
        , oneTimeKeyThreshold(move(oneTimeKeyThreshold))
    {
        Q_ASSERT((0 <= signedKeysProportion) && (signedKeysProportion <= 1));
        Q_ASSERT((0 <= oneTimeKeyThreshold) && (oneTimeKeyThreshold <= 1));
        if (encryptionAccountPickle.isEmpty()) {
            // new e2ee TODO: olmAccount.reset(new QOlmAccount());
        } else {
            // new e2ee TODO: olmAccount.reset(new QOlmAccount(encryptionAccountPickle)); // TODO: passphrase even with qtkeychain?
        }
        /*
         * Note about targetKeysNumber:
         *
         * From: https://github.com/Zil0/matrix-python-sdk/
         * File: matrix_client/crypto/olm_device.py
         *
         * Try to maintain half the number of one-time keys libolm can hold
         * uploaded on the HS. This is because some keys will be claimed by
         * peers but not used instantly, and we want them to stay in libolm,
         * until the limit is reached and it starts discarding keys, starting by
         * the oldest.
         */
        targetKeysNumber = olmAccount->maxNumberOfOneTimeKeys() / 2;
        targetOneTimeKeyCounts = {
            { SignedCurve25519Key,
              qRound(signedKeysProportion * targetKeysNumber) },
            { Curve25519Key,
              qRound((1 - signedKeysProportion) * targetKeysNumber) }
        };
        updateKeysToUpload();
    }
    ~Private() = default;

    EncryptionManager* q;

    UploadKeysJob* uploadIdentityKeysJob = nullptr;
    UploadKeysJob* uploadOneTimeKeysInitJob = nullptr;
    UploadKeysJob* uploadOneTimeKeysJob = nullptr;
    QueryKeysJob* queryKeysJob = nullptr;

    QScopedPointer<QOlmAccount> olmAccount;

    float signedKeysProportion;
    float oneTimeKeyThreshold;
    int targetKeysNumber;

    void updateKeysToUpload();
    bool oneTimeKeyShouldUpload();

    QHash<QString, int> oneTimeKeyCounts;
    void setOneTimeKeyCounts(const QHash<QString, int> oneTimeKeyCountsNewValue)
    {
        oneTimeKeyCounts = oneTimeKeyCountsNewValue;
        updateKeysToUpload();
    }
    QHash<QString, int> oneTimeKeysToUploadCounts;
    QHash<QString, int> targetOneTimeKeyCounts;

    // A map from senderKey to InboundSession
    QMap<QString, std::unique_ptr<QOlmSession>> sessions; // TODO: cache
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
    QString sessionDecrypt(const QOlmMessage& message, const QString& senderKey)
    {
        // Try to decrypt message body using one of the known sessions for that
        // device
        /*bool sessionsPassed = false;
        // new e2ee TODO:
        for (auto &senderSession : sessions) {
            if (senderSession == sessions.last()) {
                sessionsPassed = true;
            }

            const auto decryptedResult = senderSession->decrypt(message);
            if (std::holds_alternative<QString>(decryptedResult)) {
                qCDebug(E2EE)
                    << "Success decrypting Olm event using existing session"
                    << senderSession->sessionId();
                return std::get<QString>(decryptedResult);
            } else {
                const auto error = std::get<QOlmError>(decryptedResult);
                if (message.type() == QOlmMessage::PreKey) {
                    const auto matches = senderSession->matchesInboundSessionFrom(senderKey, message);
                    if (auto hasMatch = std::get_if<bool>(&matches)) {
                        if (hasMatch) {
                            // We had a matching session for a pre-key message, but
                            // it didn't work. This means something is wrong, so we
                            // fail now.
                            qCDebug(E2EE)
                                << "Error decrypting pre-key message with existing "
                                   "Olm session"
                                << senderSession->sessionId() << "reason:" << error;
                            return QString();
                        }
                    }
                }
                // Simply keep trying otherwise
            }
        }
        if (sessionsPassed || sessions.empty()) {
            if (message.type() != QOlmMessage::PreKey) {
                // Not a pre-key message, we should have had a matching session
                if (!sessions.empty()) {
                    qCDebug(E2EE) << "Error decrypting with existing sessions";
                    return QString();
                }
                qCDebug(E2EE) << "No existing sessions";
                return QString();
            }
            // We have a pre-key message without any matching session, in this
            // case we should try to create one.
            qCDebug(E2EE) << "try to establish new InboundSession with" << senderKey;
            QOlmMessage preKeyMessage = QOlmMessage(message.toCiphertext(), QOlmMessage::PreKey);
            // new e2ee TODO:
            //const auto sessionResult = olmAccount->createInboundSessionFrom(senderKey.toUtf8(), preKeyMessage);

            if (const auto error = std::get_if<QOlmError>(&sessionResult)) {
                qCDebug(E2EE) << "Error decrypting pre-key message when trying "
                                 "to establish a new session:"
                              << error;
                return QString();
            }

            const auto newSession = std::get<std::unique_ptr<QOlmSession>>(olmAccount->createInboundSessionFrom(senderKey.toUtf8(), preKeyMessage));

            qCDebug(E2EE) << "Created new Olm session" << newSession->sessionId();

            const auto decryptedResult = newSession->decrypt(message);
            if (const auto error = std::get_if<QOlmError>(&decryptedResult)) {
                qCDebug(E2EE)
                    << "Error decrypting pre-key message with new session"
                    << error;
                return QString();
            }

            if (auto error = olmAccount->removeOneTimeKeys(newSession)) {
                qCDebug(E2EE)
                    << "Error removing one time keys"
                    << error.value();
            }
            //sessions.insert(senderKey, std::move(newSession)); TODO
            //return std::get<QString>(decryptedResult);
        }*/
        return QString();
    }
};

EncryptionManager::EncryptionManager(const QByteArray& encryptionAccountPickle,
                                     float signedKeysProportion,
                                     float oneTimeKeyThreshold, QObject* parent)
    : QObject(parent)
    , d(std::make_unique<Private>(std::move(encryptionAccountPickle),
                                  std::move(signedKeysProportion),
                                  std::move(oneTimeKeyThreshold)))
{
    d->q = this;
}

EncryptionManager::~EncryptionManager() = default;

void EncryptionManager::uploadIdentityKeys(Connection* connection)
{
    // https://matrix.org/docs/spec/client_server/latest#post-matrix-client-r0-keys-upload
    DeviceKeys deviceKeys {
        /*
         * The ID of the user the device belongs to. Must match the user ID used
         * when logging in. The ID of the device these keys belong to. Must
         * match the device ID used when logging in. The encryption algorithms
         * supported by this device.
         */
        connection->userId(),
        connection->deviceId(),
        SupportedAlgorithms,
        /*
         * Public identity keys. The names of the properties should be in the
         * format <algorithm>:<device_id>. The keys themselves should be encoded
         * as specified by the key algorithm.
         */
        { { Curve25519Key + QStringLiteral(":") + connection->deviceId(),
            d->olmAccount->identityKeys().curve25519 },
          { Ed25519Key + QStringLiteral(":") + connection->deviceId(),
            d->olmAccount->identityKeys().curve25519 } },
        /* signatures should be provided after the unsigned deviceKeys
           generation */
        {}
    };

    QJsonObject deviceKeysJsonObject = toJson(deviceKeys);
    /* additionally removing signatures key,
     * since we could not initialize deviceKeys
     * without an empty signatures value:
     */
    deviceKeysJsonObject.remove(QStringLiteral("signatures"));
    /*
     * Signatures for the device key object.
     * A map from user ID, to a map from <algorithm>:<device_id> to the
     * signature. The signature is calculated using the process called Signing
     * JSON.
     */
    deviceKeys.signatures = {
        { connection->userId(),
          { { Ed25519Key + QStringLiteral(":") + connection->deviceId(),
              d->olmAccount->sign(deviceKeysJsonObject) } } }
    };

    d->uploadIdentityKeysJob = connection->callApi<UploadKeysJob>(deviceKeys);
    connect(d->uploadIdentityKeysJob, &BaseJob::success, this, [this] {
        d->setOneTimeKeyCounts(d->uploadIdentityKeysJob->oneTimeKeyCounts());
    });
}

void EncryptionManager::uploadOneTimeKeys(Connection* connection,
                                          bool forceUpdate)
{
    if (forceUpdate || d->oneTimeKeyCounts.isEmpty()) {
        d->uploadOneTimeKeysInitJob = connection->callApi<UploadKeysJob>();
        connect(d->uploadOneTimeKeysInitJob, &BaseJob::success, this, [this] {
            d->setOneTimeKeyCounts(d->uploadOneTimeKeysInitJob->oneTimeKeyCounts());
        });
    }

    int signedKeysToUploadCount =
        d->oneTimeKeysToUploadCounts.value(SignedCurve25519Key, 0);
    int unsignedKeysToUploadCount =
        d->oneTimeKeysToUploadCounts.value(Curve25519Key, 0);

    d->olmAccount->generateOneTimeKeys(signedKeysToUploadCount
                                       + unsignedKeysToUploadCount);

    QHash<QString, QVariant> oneTimeKeys = {};
    const auto& olmAccountCurve25519OneTimeKeys = d->olmAccount->oneTimeKeys().curve25519();

    int oneTimeKeysCounter = 0;
    for (auto it = olmAccountCurve25519OneTimeKeys.cbegin();
         it != olmAccountCurve25519OneTimeKeys.cend(); ++it) {
        QString keyId = it.key();
        QString keyType;
        QVariant key;
        if (oneTimeKeysCounter < signedKeysToUploadCount) {
            QJsonObject message { { QStringLiteral("key"),
                                    it.value() } };

            QByteArray signedMessage = d->olmAccount->sign(message);
            QJsonObject signatures {
                { connection->userId(),
                  QJsonObject { { Ed25519Key + QStringLiteral(":")
                                      + connection->deviceId(),
                                  QString::fromUtf8(signedMessage) } } }
            };
            message.insert(QStringLiteral("signatures"), signatures);
            key = message;
            keyType = SignedCurve25519Key;
        } else {
            key = it.value();
            keyType = Curve25519Key;
        }
        ++oneTimeKeysCounter;
        oneTimeKeys.insert(QString("%1:%2").arg(keyType).arg(keyId), key);
    }
    d->uploadOneTimeKeysJob =
        connection->callApi<UploadKeysJob>(none, oneTimeKeys);
    connect(d->uploadOneTimeKeysJob, &BaseJob::success, this, [this] {
        d->setOneTimeKeyCounts(d->uploadOneTimeKeysJob->oneTimeKeyCounts());
    });
    // new e2ee TODO: d->olmAccount->markKeysAsPublished();
    qCDebug(E2EE) << QString("Uploaded new one-time keys: %1 signed, %2 unsigned.")
                    .arg(signedKeysToUploadCount)
                .arg(unsignedKeysToUploadCount);
}

void EncryptionManager::updateOneTimeKeyCounts(
    Connection* connection, const QHash<QString, int>& deviceOneTimeKeysCount)
{
    d->oneTimeKeyCounts = deviceOneTimeKeysCount;
    if (d->oneTimeKeyShouldUpload()) {
        qCDebug(E2EE) << "Uploading new one-time keys.";
        uploadOneTimeKeys(connection);
    }
}

void Quotient::EncryptionManager::updateDeviceKeys(
    Connection* connection, const QHash<QString, QStringList>& deviceKeys)
{
    d->queryKeysJob = connection->callApi<QueryKeysJob>(deviceKeys);
    connect(d->queryKeysJob, &BaseJob::success, this,
            [this] { d->updateDeviceKeys(d->queryKeysJob->deviceKeys()); });
}

QString EncryptionManager::sessionDecryptMessage(
    const QJsonObject& personalCipherObject, const QByteArray& senderKey)
{
    QString decrypted;
    int type = personalCipherObject.value(TypeKeyL).toInt(-1);
    QByteArray body = personalCipherObject.value(BodyKeyL).toString().toLatin1();
    if (type == 0) {
        QOlmMessage preKeyMessage = QOlmMessage(body, QOlmMessage::PreKey);
        decrypted = d->sessionDecrypt(preKeyMessage, senderKey);
    } else if (type == 1) {
        QOlmMessage message = QOlmMessage(body, QOlmMessage::PreKey);
        decrypted = d->sessionDecrypt(message, senderKey);
    }
    return decrypted;
}

QByteArray EncryptionManager::olmAccountPickle()
{
    // new e2ee TODO: return d->olmAccount->pickle(); // TODO: passphrase even with qtkeychain?
    return {};
}

QOlmAccount *EncryptionManager::account() const
{
    return d->olmAccount.data();
}

void EncryptionManager::Private::updateKeysToUpload()
{
    for (auto it = targetOneTimeKeyCounts.cbegin();
         it != targetOneTimeKeyCounts.cend(); ++it) {
        int numKeys = oneTimeKeyCounts.value(it.key(), 0);
        int numToCreate = qMax(it.value() - numKeys, 0);
        oneTimeKeysToUploadCounts.insert(it.key(), numToCreate);
    }
}

bool EncryptionManager::Private::oneTimeKeyShouldUpload()
{
    if (oneTimeKeyCounts.empty())
        return true;
    for (auto it = targetOneTimeKeyCounts.cbegin();
         it != targetOneTimeKeyCounts.cend(); ++it) {
        if (oneTimeKeyCounts.value(it.key(), 0)
            < it.value() * oneTimeKeyThreshold)
            return true;
    }
    return false;
}
#endif // Quotient_E2EE_ENABLED
