// SPDX-FileCopyrightText: 2024 Tobias Fella <tobias.fella@kde.org>
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include "accountregistry.h"

#include <QObject>

namespace Quotient
{
namespace _impl {
class ConnectionEncryptionData;
}

class PendingConnection : public QObject
{
    Q_OBJECT

public:
    static PendingConnection *loginWithPassword(const QString& userId, const QString& password, const Quotient::ConnectionSettings& settings, Quotient::AccountRegistry* accountRegistry);
    static PendingConnection *restoreConnection(const QString& userId, const Quotient::ConnectionSettings& settings, Quotient::AccountRegistry* accountRegistry);

    ~PendingConnection();
    Quotient::Connection* connection() const;

Q_SIGNALS:
    void loginFlowsChanged(); //TODO make private somehow?
    void homeserverChanged(QUrl baseUrl);

    //! \brief Initial server resolution has failed
    //!
    //! This signal is emitted when resolveServer() did not manage to resolve
    //! the homeserver using its .well-known/client record or otherwise.
    //! \sa resolveServer
    void resolveError(QString error);
    void loginError(QString message, QString details);
    void ready();

private:
    friend class Quotient::_impl::ConnectionEncryptionData;
    PendingConnection(const QString& userId, const QString& password, const Quotient::ConnectionSettings& settings, Quotient::AccountRegistry* accountRegistry);
    PendingConnection(const QString& userId, const Quotient::ConnectionSettings& settings, Quotient::AccountRegistry* accountRegistry);

    QString userId() const;

    class Private;
    std::unique_ptr<Private> d;
};

}
