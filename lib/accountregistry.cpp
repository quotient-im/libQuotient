// SPDX-FileCopyrightText: Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-FileCopyrightText: Tobias Fella <fella@posteo.de>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "accountregistry.h"

#include "connection.h"
#include "settings.h"

#include <QtCore/QCoreApplication>

using namespace Quotient;

AccountRegistry::AccountRegistry() { qInfo(MAIN) << "Account registry created"; }

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

Connection* AccountRegistry::get(const QString& userId)
{
    return const_cast<const AccountRegistry*>(this)->get(userId);
}

Connection* AccountRegistry::get(const QString& userId) const
{
    for (const auto& connection : *this) {
        if (connection->userId() == userId)
            return connection;
    }
    return nullptr;
}

QKeychain::ReadPasswordJob* AccountRegistry::loadAccessTokenFromKeychain(
    const QString& userId)
{
    qCDebug(MAIN) << "Reading access token from keychain for" << userId;
    auto job = new QKeychain::ReadPasswordJob(qAppName(), this);
    job->setKey(userId);
    job->start();

    return job;
}

void AccountRegistry::invokeLogin()
{
    const auto accounts = SettingsGroup("Accounts"_ls).childGroups();
    for (const auto& accountId : accounts) {
        AccountSettings account { accountId };

        if (account.homeserver().isEmpty())
            continue;

        m_accountsLoading += accountId;
        emit accountsLoadingChanged();

        auto accessTokenLoadingJob =
            loadAccessTokenFromKeychain(account.userId());
        connect(accessTokenLoadingJob, &QKeychain::Job::finished, this,
                [accountId, this, accessTokenLoadingJob]() {
                    if (accessTokenLoadingJob->error()
                        != QKeychain::Error::NoError) {
                        emit keychainError(accessTokenLoadingJob->error());
                        m_accountsLoading.removeAll(accountId);
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
                        account.userId(),
                        QString::fromUtf8(accessTokenLoadingJob->binaryData()));
                });
    }
}

QStringList AccountRegistry::accountsLoading() const
{
    return m_accountsLoading;
}
