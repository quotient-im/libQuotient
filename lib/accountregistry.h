// SPDX-FileCopyrightText: 2020 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-FileCopyrightText: Tobias Fella <fella@posteo.de>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "quotient_export.h"
#include "settings.h"

#include <QtCore/QAbstractListModel>

#if QT_VERSION_MAJOR >= 6
#    include <qt6keychain/keychain.h>
#else
#    include <qt5keychain/keychain.h>
#endif

namespace QKeychain {
class ReadPasswordJob;
}

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
}
