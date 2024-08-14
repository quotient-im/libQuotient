// SPDX-FileCopyrightText: Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-FileCopyrightText: Tobias Fella <fella@posteo.de>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "accountregistry.h"

#include "pendingconnection.h"
#include "connection.h"
#include "logging_categories_p.h"
#include "settings.h"
#include "config.h"

#include <QtCore/QCoreApplication>

using namespace Quotient;

struct Q_DECL_HIDDEN AccountRegistry::Private {
    QStringList m_accountsLoading;
};

AccountRegistry::AccountRegistry(QObject* parent)
    : QAbstractListModel(parent), d(makeImpl<Private>())
{}

void AccountRegistry::add(Connection* a)
{
    Q_ASSERT(a != nullptr);
    if (get(a->userId()) != nullptr) {
        qWarning(MAIN) << "Attempt to add another connection for the same user "
                          "id; skipping";
        return;
    }
    beginInsertRows(QModelIndex(), size(), size());
    push_back(a);
    connect(a, &Connection::loggedOut, this, [this, a] { drop(a); });
    qDebug(MAIN) << "Added" << a->objectName() << "to the account registry";
    endInsertRows();
    emit accountCountChanged();
}

void AccountRegistry::drop(Connection* a)
{
    if (const auto idx = indexOf(a); idx != -1) {
        beginRemoveRows(QModelIndex(), idx, idx);
        remove(idx);
        qDebug(MAIN) << "Removed" << a->objectName()
                     << "from the account registry";
        endRemoveRows();
    }
    Q_ASSERT(!contains(a));
}

bool AccountRegistry::isLoggedIn(const QString &userId) const
{
    const auto conn = get(userId);
    return conn != nullptr && conn->isLoggedIn();
}

QVariant AccountRegistry::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= count())
        return {};

    switch (role) {
        case AccountRole:
            return QVariant::fromValue(at(index.row()));
        case UserIdRole:
            return QVariant::fromValue(at(index.row())->userId());
        default:
            return {};
    }
}

int AccountRegistry::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : count();
}

QHash<int, QByteArray> AccountRegistry::roleNames() const
{
    return { { AccountRole, QByteArrayLiteral("connection") },
             { UserIdRole, QByteArrayLiteral("userId") } };
}

Connection* AccountRegistry::get(const QString& userId) const
{
    for (const auto& connection : accounts()) {
        if (connection->userId() == userId)
            return connection;
    }
    return nullptr;
}

QStringList AccountRegistry::accountsLoading() const
{
    return d->m_accountsLoading;
}

PendingConnection* AccountRegistry::loginWithPassword(const QString& matrixId, const QString& password, const ConnectionSettings& settings)
{
    return PendingConnection::loginWithPassword(matrixId, password, settings, this);
}

PendingConnection* AccountRegistry::restoreConnection(const QString& matrixId, const ConnectionSettings& settings)
{
    return PendingConnection::restoreConnection(matrixId, settings, this);
}

QStringList AccountRegistry::availableConnections() const
{
    //TODO change accounts -> QuotientAccounts?
    return Config::instance()->childGroups("Accounts"_ls, Config::Data);
    //TODO check whether we have plausible data?
}
