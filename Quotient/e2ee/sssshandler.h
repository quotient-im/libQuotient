// SPDX-FileCopyrightText: 2023 Tobias Fella <tobias.fella@kde.org>
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include <QtCore/QObject>
#include <QtCore/QPointer>

#include "../connection.h"

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

    //! \brief Unlock the secret backup from the given password
    Q_INVOKABLE void unlockSSSSFromPassword(const QString& password);

    //! \brief Unlock the secret backup by requesting the password from other devices
    Q_INVOKABLE void unlockSSSSFromCrossSigning();

    //! \brief Unlock the secret backup from the given security key
    Q_INVOKABLE void unlockSSSSFromSecurityKey(const QString& key);

    Connection* connection() const;
    void setConnection(Connection* connection);

Q_SIGNALS:
    void keyBackupUnlocked();
    void error(Error error);
    void connectionChanged();

private:
    QPointer<Connection> m_connection;

    //! \brief Decrypt the key with this name from the account data
    QByteArray decryptKey(event_type_t keyType, const QByteArray& decryptionKey);

    void loadMegolmBackup(const QByteArray& megolmDecryptionKey);
    void calculateDefaultKey(const QByteArray& secret, bool requirePassphrase);
};
} // namespace Quotient
