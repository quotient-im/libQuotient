// SPDX-FileCopyrightText: Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-FileCopyrightText: Tobias Fella <fella@posteo.de>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "accountregistry.h"

#include "connection.h"
#include <QtCore/QCoreApplication>

using namespace Quotient;

void AccountRegistry::add(Connection* a)
{
    if (contains(a))
        return;
    beginInsertRows(QModelIndex(), size(), size());
    push_back(a);
    endInsertRows();
    emit accountCountChanged();
}

void AccountRegistry::drop(Connection* a)
{
    if (const auto idx = indexOf(a); idx != -1) {
        beginRemoveRows(QModelIndex(), idx, idx);
        remove(idx);
        endRemoveRows();
    }
    Q_ASSERT(!contains(a));
}

bool AccountRegistry::isLoggedIn(const QString &userId) const
{
    return std::any_of(cbegin(), cend(), [&userId](const Connection* a) {
        return a->userId() == userId;
    });
}

QVariant AccountRegistry::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= count())
        return {};

    if (role == AccountRole)
        return QVariant::fromValue(at(index.row()));

    return {};
}

int AccountRegistry::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : count();
}

QHash<int, QByteArray> AccountRegistry::roleNames() const
{
    return { { AccountRole, "connection" } };
}

Connection* AccountRegistry::get(const QString& userId)
{
    for (const auto &connection : *this) {
        if (connection->userId() == userId)
            return connection;
    }
    return nullptr;
}

QKeychain::ReadPasswordJob* AccountRegistry::loadAccessTokenFromKeychain(const QString& userId)
{
    qCDebug(MAIN) << "Reading access token from keychain for" << userId;
    auto job = new QKeychain::ReadPasswordJob(qAppName(), this);
    job->setKey(userId);
    job->start();

    return job;
}

void AccountRegistry::invokeLogin()
{
    const auto accounts = SettingsGroup("Accounts").childGroups();
    for (const auto& accountId : accounts) {
        AccountSettings account { accountId };
        m_accountsLoading += accountId;
        emit accountsLoadingChanged();

        if (account.homeserver().isEmpty())
            continue;

        auto accessTokenLoadingJob =
            loadAccessTokenFromKeychain(account.userId());
        connect(accessTokenLoadingJob, &QKeychain::Job::finished, this,
                [accountId, this, accessTokenLoadingJob]() {
                    if (accessTokenLoadingJob->error()
                        != QKeychain::Error::NoError) {
                        emit keychainError(accessTokenLoadingJob->error());
                        return;
                    }

                    AccountSettings account { accountId };
                    auto connection = new Connection(account.homeserver());
                    connect(connection, &Connection::connected, this,
                            [connection, this, accountId] {
                                connection->loadState();
                                connection->setLazyLoading(true);

                                connection->syncLoop();

                                m_accountsLoading.removeAll(accountId);
                                emit accountsLoadingChanged();
                            });
                    connect(connection, &Connection::loginError, this,
                            [this, connection, accountId](const QString& error,
                                               const QString& details) {
                                emit loginError(connection, error, details);

                                m_accountsLoading.removeAll(accountId);
                                emit accountsLoadingChanged();
                            });
                    connect(connection, &Connection::resolveError, this,
                            [this, connection, accountId](const QString& error) {
                                emit resolveError(connection, error);

                                m_accountsLoading.removeAll(accountId);
                                emit accountsLoadingChanged();
                            });
                    connection->assumeIdentity(
                        account.userId(), accessTokenLoadingJob->binaryData(),
                        account.deviceId());
                });
    }
}

QStringList AccountRegistry::accountsLoading() const
{
    return m_accountsLoading;
}
