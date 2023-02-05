// SPDX-FileCopyrightText: 2020 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-FileCopyrightText: Tobias Fella <fella@posteo.de>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "quotient_export.h"

#include <QtCore/QAbstractListModel>

#if QT_VERSION_MAJOR >= 6
#    include <qt6keychain/keychain.h>
#else
#    include <qt5keychain/keychain.h>
#endif

namespace Quotient {
class Connection;

class QUOTIENT_API AccountRegistry : public QAbstractListModel,
                                     private QVector<Connection*> {
    Q_OBJECT
    /// Number of accounts that are currently fully loaded
    Q_PROPERTY(int accountCount READ rowCount NOTIFY accountCountChanged)
    /// List of accounts that are currently in some stage of being loaded (Reading token from keychain, trying to contact server, etc).
    /// Can be used to inform the user or to show a login screen if size() == 0 and no accounts are loaded
    Q_PROPERTY(QStringList accountsLoading READ accountsLoading NOTIFY accountsLoadingChanged)
public:
    using vector_t = QVector<Connection*>;
    using const_iterator = vector_t::const_iterator;
    using const_reference = vector_t::const_reference;

    enum EventRoles {
        AccountRole = Qt::UserRole + 1,
        ConnectionRole = AccountRole,
        UserIdRole = Qt::DisplayRole
    };

    [[deprecated("Use Accounts variable instead")]] //
    static AccountRegistry& instance();

    // Expose most of vector_t's const-API but only provide add() and drop()
    // for changing it. In theory other changing operations could be supported
    // too; but then boilerplate begin/end*() calls has to be tucked into each
    // and this class gives no guarantees on the order of entries, so why care.

    const vector_t& accounts() const { return *this; }
    void add(Connection* a);
    void drop(Connection* a);
    const_iterator begin() const { return vector_t::begin(); }
    const_iterator end() const { return vector_t::end(); }
    const_reference front() const { return vector_t::front(); }
    const_reference back() const { return vector_t::back(); }
    bool isLoggedIn(const QString& userId) const;
    Connection* get(const QString& userId);

    using vector_t::isEmpty, vector_t::empty;
    using vector_t::size, vector_t::count, vector_t::capacity;
    using vector_t::cbegin, vector_t::cend, vector_t::contains;

    // QAbstractItemModel interface implementation

    [[nodiscard]] QVariant data(const QModelIndex& index,
                                int role) const override;
    [[nodiscard]] int rowCount(
        const QModelIndex& parent = QModelIndex()) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    QStringList accountsLoading() const;

    void invokeLogin();

Q_SIGNALS:
    void accountCountChanged();
    void accountsLoadingChanged();

    void keychainError(QKeychain::Error error);
    void loginError(Connection* connection, QString message, QString details);
    void resolveError(Connection* connection, QString error);

private:
    QKeychain::ReadPasswordJob* loadAccessTokenFromKeychain(const QString &userId);
    QStringList m_accountsLoading;
};

inline QUOTIENT_API AccountRegistry Accounts {};

inline AccountRegistry& AccountRegistry::instance() { return Accounts; }
} // namespace Quotient
