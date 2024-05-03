// SPDX-FileCopyrightText: 2023 Tobias Fella <tobias.fella@kde.org>
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include "cryptoutils.h"

#include "../connection.h"

#include <QtCore/QObject>
#include <QtCore/QPointer>

namespace Quotient {
class QUOTIENT_API SSSSHandler : public QObject
{
    Q_OBJECT
    Q_PROPERTY(Quotient::Connection* connection READ connection WRITE setConnection NOTIFY connectionChanged)

public:
    enum Error
    {
        WrongKeyError,
        NoKeyError,
        DecryptionError,
        InvalidSignatureError,
        UnsupportedAlgorithmError,
    };
    Q_ENUM(Error)

    using QObject::QObject;

    //! \brief Unlock the secret backup from the given passprhase
    Q_INVOKABLE void unlockSSSSWithPassphrase(const QString& passphrase);

    //! \brief Unlock the secret backup by requesting the decryption keys from other devices
    Q_INVOKABLE void unlockSSSSFromCrossSigning();

    //! \brief Unlock the secret backup from the given security key
    Q_INVOKABLE void unlockSSSSFromSecurityKey(const QString& encodedKey);

    Connection* connection() const;
    void setConnection(Connection* connection);

Q_SIGNALS:
    void keyBackupUnlocked();
    void error(Error error);
    void connectionChanged();

    //! \brief Emitted after keys are loaded
    void finished();

private:
    QPointer<Connection> m_connection;

    //! \brief Decrypt the key with this name from the account data
    QByteArray decryptKey(event_type_t keyType, const QString& defaultKey, key_view_t decryptionKey);

    void loadMegolmBackup(const QByteArray& megolmDecryptionKey);
    struct UnlockData;
    void unlockAndLoad(const UnlockData& unlockData, key_view_t decryptingKey);
};
} // namespace Quotient
