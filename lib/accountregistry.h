// SPDX-FileCopyrightText: 2020 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-FileCopyrightText: Tobias Fella <fella@posteo.de>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "quotient_export.h"

#include <QtCore/QAbstractListModel>

namespace Quotient {
class Connection;

class QUOTIENT_API AccountRegistry : public QAbstractListModel,
                                     private QVector<Connection*> {
    Q_OBJECT
public:
    using const_iterator = QVector::const_iterator;
    using const_reference = QVector::const_reference;

    enum EventRoles {
        AccountRole = Qt::UserRole + 1,
        ConnectionRole = AccountRole
    };

    [[deprecated("Use Accounts variable instead")]] //
    static AccountRegistry& instance();

    // Expose most of QVector's const-API but only provide add() and drop()
    // for changing it. In theory other changing operations could be supported
    // too; but then boilerplate begin/end*() calls has to be tucked into each
    // and this class gives no guarantees on the order of entries, so why care.

    const QVector<Connection*>& accounts() const { return *this; }
    void add(Connection* a);
    void drop(Connection* a);
    const_iterator begin() const { return QVector::begin(); }
    const_iterator end() const { return QVector::end(); }
    const_reference front() const { return QVector::front(); }
    const_reference back() const { return QVector::back(); }
    bool isLoggedIn(const QString& userId) const;
    Connection* get(const QString& userId);

    using QVector::isEmpty, QVector::empty;
    using QVector::size, QVector::count, QVector::capacity;
    using QVector::cbegin, QVector::cend, QVector::contains;

    // QAbstractItemModel interface implementation

    [[nodiscard]] QVariant data(const QModelIndex& index,
                                int role) const override;
    [[nodiscard]] int rowCount(
        const QModelIndex& parent = QModelIndex()) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;
};

inline QUOTIENT_API AccountRegistry Accounts {};

inline AccountRegistry& AccountRegistry::instance() { return Accounts; }
}
