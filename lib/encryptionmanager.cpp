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
    explicit Private()
        : q(nullptr)
    {
    }
    ~Private() = default;

    EncryptionManager* q;

    // A map from senderKey to InboundSession
    std::map<QString, std::unique_ptr<QOlmSession>> sessions; // TODO: cache
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
    QString sessionDecryptPrekey(const QOlmMessage& message, const QString &senderKey, std::unique_ptr<QOlmAccount>& olmAccount)
    {
        Q_ASSERT(message.type() == QOlmMessage::PreKey);
        for(auto& session : sessions) {
            const auto matches = session.second->matchesInboundSessionFrom(senderKey, message);
            if(std::holds_alternative<bool>(matches) && std::get<bool>(matches)) {
                qCDebug(E2EE) << "Found inbound session";
                const auto result = session.second->decrypt(message);
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
            qCWarning(E2EE) << "Failed to create inbound session for" << senderKey;
            return {};
        }
        std::unique_ptr<QOlmSession> newSession = std::move(std::get<std::unique_ptr<QOlmSession>>(newSessionResult));
        // TODO Error handling?
        olmAccount->removeOneTimeKeys(newSession);
        const auto result = newSession->decrypt(message);
        sessions[senderKey] = std::move(newSession);
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
#endif // Quotient_E2EE_ENABLED
