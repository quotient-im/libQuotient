// SPDX-FileCopyrightText: Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-FileCopyrightText: Tobias Fella <fella@posteo.de>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "accountregistry.h"

#include "connection.h"
#include "logging_categories_p.h"
#include "settings.h"

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
    if (!index.isValid() || index.row() >= size())
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
    return parent.isValid() ? 0 : size();
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

void AccountRegistry::invokeLogin()
{
    const auto accounts = SettingsGroup("Accounts"_L1).childGroups();
    for (const auto& accountId : accounts) {
        AccountSettings account { accountId };

        if (account.homeserver().isEmpty())
            continue;

        d->m_accountsLoading += accountId;
        emit accountsLoadingChanged();

        qCDebug(MAIN) << "Reading access token from keychain for" << accountId;
        auto accessTokenLoadingJob =
            new QKeychain::ReadPasswordJob(qAppName(), this);
        accessTokenLoadingJob->setKey(accountId);
        connect(accessTokenLoadingJob, &QKeychain::Job::finished, this,
                [accountId, this, accessTokenLoadingJob]() {
                    if (accessTokenLoadingJob->error()
                        != QKeychain::Error::NoError) {
                        emit keychainError(accessTokenLoadingJob->error());
                        d->m_accountsLoading.removeAll(accountId);
                        emit accountsLoadingChanged();
                        return;
                    }

                    AccountSettings account { accountId };
                    auto connection = new Connection(account.homeserver());
                    connect(connection, &Connection::connected, this,
                            [connection, this, accountId] {
                                connection->loadState();
                                connection->setLazyLoading(true);

                                connection->syncLoop();

                                d->m_accountsLoading.removeAll(accountId);
                                emit accountsLoadingChanged();
                            });
                    connect(connection, &Connection::loginError, this,
                            [this, connection, accountId](const QString& error,
                                               const QString& details) {
                                emit loginError(connection, error, details);

                                d->m_accountsLoading.removeAll(accountId);
                                emit accountsLoadingChanged();
                            });
                    connect(connection, &Connection::resolveError, this,
                            [this, connection, accountId](const QString& error) {
                                emit resolveError(connection, error);

                                d->m_accountsLoading.removeAll(accountId);
                                emit accountsLoadingChanged();
                            });
                    connection->assumeIdentity(
                        account.userId(),
                        account.deviceId(),
                        QString::fromUtf8(accessTokenLoadingJob->binaryData()));
                    add(connection);
                });
        accessTokenLoadingJob->start();
    }
}

QStringList AccountRegistry::accountsLoading() const
{
    return d->m_accountsLoading;
}
