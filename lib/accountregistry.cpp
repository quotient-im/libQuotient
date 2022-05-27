// SPDX-FileCopyrightText: Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-FileCopyrightText: Tobias Fella <fella@posteo.de>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "accountregistry.h"

#include "connection.h"
#include <QtCore/QCoreApplication>

#if QT_VERSION_MAJOR >= 6
#    include <qt6keychain/keychain.h>
#else
#    include <qt5keychain/keychain.h>
#endif
using namespace Quotient;

void AccountRegistry::add(Connection* a)
{
    if (contains(a))
        return;
    beginInsertRows(QModelIndex(), size(), size());
    push_back(a);
    endInsertRows();
    Q_EMIT accountCountChanged();
}

void AccountRegistry::drop(Connection* a)
{
    const auto idx = indexOf(a);
    beginRemoveRows(QModelIndex(), idx, idx);
    remove(idx);
    endRemoveRows();
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

    connect(job, &QKeychain::Job::finished, this, [job]() {
        if (job->error() == QKeychain::Error::NoError) {
            return;
        }
        //TODO error handling
    });
    job->start();

    return job;
}

void AccountRegistry::invokeLogin()
{
    const auto accounts = SettingsGroup("Accounts").childGroups();
    for (const auto& accountId : accounts) {
        AccountSettings account{accountId};
        m_accountsLoading += accountId;
        Q_EMIT accountsLoadingChanged();

        if (!account.homeserver().isEmpty()) {
            auto accessTokenLoadingJob = loadAccessTokenFromKeychain(account.userId());
            connect(accessTokenLoadingJob, &QKeychain::Job::finished, this, [accountId, this, accessTokenLoadingJob]() {
                AccountSettings account{accountId};
                if (accessTokenLoadingJob->error() != QKeychain::Error::NoError) {
                    //TODO error handling
                    return;
                }

                auto connection = new Connection(account.homeserver());
                connect(connection, &Connection::connected, this, [connection] {
                    connection->loadState();
                    connection->setLazyLoading(true);

                    connection->syncLoop();
                });
                connect(connection, &Connection::loginError, this, [](const QString& error, const QString&) {
                    //TODO error handling
                });
                connect(connection, &Connection::networkError, this, [](const QString& error, const QString&, int, int) {
                    //TODO error handling
                });
                connection->assumeIdentity(account.userId(), accessTokenLoadingJob->binaryData(), account.deviceId());
            });
        }
    }
}

QStringList AccountRegistry::accountsLoading() const
{
    return m_accountsLoading;
}
