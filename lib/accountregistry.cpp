// SPDX-FileCopyrightText: Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-FileCopyrightText: Tobias Fella <fella@posteo.de>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "accountregistry.h"

#include "connection.h"

using namespace Quotient;

void AccountRegistry::add(Connection* a)
{
    if (contains(a))
        return;
    beginInsertRows(QModelIndex(), size(), size());
    push_back(a);
    endInsertRows();
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
