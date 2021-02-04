// SPDX-FileCopyrightText: 2019 Alexey Andreyev <aa13q@ya.ru>
// SPDX-FileCopyrightText: 2019 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
// SPDX-license-identifier: lgpl-2.1-or-later

/// This class is private.

#include <QObject>
#include <memory>
#include "crypto/qolmaccount.h"
#include "csapi/keys.h"

namespace Quotient {

class Connection;
class DevicesList;

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

    void save();
    void load();
private:
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
