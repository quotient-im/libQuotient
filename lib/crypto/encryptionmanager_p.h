// SPDX-FileCopyrightText: 2019 Alexey Andreyev <aa13q@ya.ru>
// SPDX-FileCopyrightText: 2019 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
// SPDX-license-identifier: lgpl-2.1-or-later

/// This class is private.

#include <QObject>
#include <memory>
#include "crypto/qolmaccount.h"
#include "settings.h"
#include "csapi/keys.h"
#include "events/encryptedevent.h"

namespace Quotient {

class Connection;
class DevicesList;

/// Object that will be stored in a json file for later uses.
struct StoredOlmSession
{
    uint64_t lastMessageTimestamp;
    QByteArray pickedSession;
};

template<>
struct JsonObjectConverter<StoredOlmSession> {
    static void fillFrom(const QJsonObject &jo,
                         StoredOlmSession &result)
    {
        fromJson(jo.value("lastMessageTimestamp"_ls), result.lastMessageTimestamp);
        fromJson(jo.value("pickedSession"_ls), result.pickedSession);
    }

    static void dumpTo(QJsonObject &jo, const StoredOlmSession &result)
    {
        addParam<>(jo, QStringLiteral("lastMessageTimestamp"), result.lastMessageTimestamp);
        addParam<>(jo, QStringLiteral("pickedSession"), result.pickedSession);
    }
};

/*
template<>
struct JsonObjectConverter<QueryKeysJob::DeviceInformation> {
    static void fillFrom(const QJsonObject &jo,
                         QueryKeysJob::DeviceInformation &result)
    {
        fromJson(jo.value("user_id"_ls), pod.userId);
        fromJson(jo.value("device_id"_ls), pod.deviceId);
        fromJson(jo.value("algorithms"_ls), pod.algorithms);
        fromJson(jo.value("keys"_ls), pod.keys);
        fromJson(jo.value("signatures"_ls), pod.signatures);
        fromJson(jo.value("deviceDisplayName"_ls), result.deviceDisplayName);
    }

    static void dumpTo(QJsonObject& jo, const DeviceInformation& pod)
    {
        addParam<>(jo, QStringLiteral("user_id"), pod.userId);
        addParam<>(jo, QStringLiteral("device_id"), pod.deviceId);
        addParam<>(jo, QStringLiteral("algorithms"), pod.algorithms);
        addParam<>(jo, QStringLiteral("keys"), pod.keys);
        addParam<>(jo, QStringLiteral("signatures"), pod.signatures);
        addParam<>(jo. QStringLiteral("deviceDisplayName"), pod.deviceDisplayName);
    }
};*/

/// Manage automatically the crypto for us (device key tracking, session caching, encrypting, decrypting).
class EncryptionManager : public QObject
{
public:
    explicit EncryptionManager(const QString &userId, const QString &deviceId,
            Connection *connction);
    ~EncryptionManager();

    QOlmAccount *olmAccount() const;

    void uploadIdentityKeys() const;
    void uploadOneTimeKeys();

    void ensureOneTimeKeyCount(const QHash<QString, int> &counts) const;
    /// Track the user devices according to the spec.
    /// https://matrix.org/docs/spec/client_server/r0.6.1#tracking-the-device-list-for-a-user
    void trackUserDevices(const QString &userId);
    void trackUsersDevices(const QStringList &users);
    void updateDevices(const QStringList &changed, const QString &nextBatch);
    /// Is called during the first sync after the client wake up. Call
    /// GetKeysChangesJob and update the tracked devices.
    void getKeysChangesSince(const QString &nextBatch);

    /// Handles olm message
    EventPtr handleOlmMessage(const EncryptedEvent& encryptedEvent);

    void save();
    void load();
private:
    void saveAccount();

    /// Try to decrypt olm message
    QJsonDocument tryToHandle(const QString &senderKey, const QOlmMessage &msg);

    QJsonDocument handlePreKeyOlmMessage(const QString &sender, const QByteArray &senderKey,
                                         const QOlmMessage &message);

    /// Save an olm session to a json file.
    void saveOlmSession(const QString &curve25519, std::unique_ptr<QOlmSession> olmSession,
            uint64_t timestamp);

    /// Get an olmSession from a sessionId and a curve25519 key.
    /// Can be empty.
    std::unique_ptr<QOlmSession> getOlmSession(const QString &curve25519,
            const QString &sessionId);

    /// Used to store the accoun information
    AccountSettings m_accountSettings;

    /// Get all sessionId for a given curve25519 key.
    QStringList getOlmSessions(const QString &curve25519);

    std::unique_ptr<QOlmAccount> m_olmAccount;
    Connection *m_connection;

    /// List of users for which we tracks the devices.
    /// This is persisted across sessions.
    QStringList m_trackedUserDevices;

    /// List of users for which the tracked devices list is outdated.
    /// This is persisted across sessions.
    QStringList m_outdatedUserDevices;

    /// Store device information for each users we track.
    QHash<QString, QHash<QString, QueryKeysJob::DeviceInformation>> m_devices;

    /// Map from user to their queryKeysJob if it exists.
    QHash<QString, QueryKeysJob *>m_queryKeysJob;

    QString m_nextBatch;
};
} // namespace Quotient
