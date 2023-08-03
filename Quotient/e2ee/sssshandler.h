// SPDX-FileCopyrightText: 2023 Tobias Fella <tobias.fella@kde.org>
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include <QtCore/QObject>
#include <QtCore/QPointer>

#include "connection.h"

class SSSSHandler : public QObject
{
    Q_OBJECT
    Q_PROPERTY(Quotient::Connection* connection READ connection WRITE setConnection NOTIFY connectionChanged)

public:

    SSSSHandler(QObject* parent = nullptr);

    //! \brief Unlock the secret backup from the given password
    void unlockSSSSFromPassword(const QString& password);

    //! \brief Decrypt the key with this name from the account data
    QByteArray decryptKey(const QString& name, const QByteArray& decryptionKey) const;

    Quotient::Connection* connection() const;
    void setConnection(Quotient::Connection* connection);

Q_SIGNALS:
    void keyBackupPasswordCorrect();
    void keyBackupPasswordWrong();
    void connectionChanged();
private:
    QPointer<Quotient::Connection> m_connection;
};
