// SPDX-FileCopyrightText: 2023 Tobias Fella <tobias.fella@kde.org>
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include <QtCore/QObject>
#include <QtCore/QPointer>

#include "../connection.h"

class QUOTIENT_API SSSSHandler : public QObject
{
    Q_OBJECT
    Q_PROPERTY(Quotient::Connection* connection READ connection WRITE setConnection NOTIFY connectionChanged)

public:
    using QObject::QObject;

    //! \brief Unlock the secret backup from the given password
    Q_INVOKABLE void unlockSSSSFromPassword(const QString& password);

    //! \brief Unlock the secret backup by requesting the password from other devices
    Q_INVOKABLE void unlockSSSSFromCrossSigning();

    //! \brief Unlock the secret backup from the given security key
    Q_INVOKABLE void unlockSSSSFromSecurityKey(const QString& key);

    Quotient::Connection* connection() const;
    void setConnection(Quotient::Connection* connection);

Q_SIGNALS:
    void keyBackupUnlocked();
    void keyBackupKeyWrong();
    void connectionChanged();

private:
    QPointer<Quotient::Connection> m_connection;

    //! \brief Decrypt the key with this name from the account data
    QByteArray decryptKey(const QString& name, const QByteArray& decryptionKey) const;

    void loadMegolmBackup(const QByteArray& megolmDecryptionKey);
    void calculateDefaultKey(const QByteArray& secret, bool requirePassphrase);
    void requestKeyFromDevices(const QString& name, const std::function<void(const QByteArray&)>& then);
};
