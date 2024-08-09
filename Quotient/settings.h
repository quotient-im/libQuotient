// SPDX-FileCopyrightText: 2016 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "quotient_export.h"
#include "util.h"

#include <QtCore/QSettings>
#include <QtCore/QUrl>
#include <QtCore/QVector>

class QVariant;

namespace Quotient {

class QUOTIENT_API Settings : public QSettings {
    Q_OBJECT
public:
    /// Add a legacy organisation/application name to migrate settings from
    /*!
     * Use this function before creating any Settings objects in order
     * to set a legacy location where configuration has previously been stored.
     * This will provide an additional fallback in case of renaming
     * the organisation/application. Values in legacy locations are _removed_
     * when setValue() or remove() is called.
     */
    static void setLegacyNames(const QString& organizationName,
                               const QString& applicationName = {});

    explicit Settings(QObject* parent = nullptr);

    /// Set the value for a given key
    /*! If the key exists in the legacy location, it is removed. */
    Q_INVOKABLE void setValue(const QString& key, const QVariant& value);

    /// Remove the value from both the primary and legacy locations
    Q_INVOKABLE void remove(const QString& key);

    /// Obtain a value for a given key
    /*!
     * If the key doesn't exist in the primary settings location, the legacy
     * location is checked. If neither location has the key,
     * \p defaultValue is returned.
     *
     * This function returns a QVariant; use get<>() to get the unwrapped
     * value if you know the type upfront.
     *
     * \sa setLegacyNames, get
     */
    Q_INVOKABLE QVariant value(const QString& key,
                               const QVariant& defaultValue = {}) const;

    /// Obtain a value for a given key, coerced to the given type
    /*!
     * On top of value(), this function unwraps the QVariant and returns
     * its contents assuming the type passed as the template parameter.
     * If the type is different from the one stored inside the QVariant,
     * \p defaultValue is returned. In presence of legacy settings,
     * only the first found value is checked; if its type does not match,
     * further checks through legacy settings are not performed and
     * \p defaultValue is returned.
     */
    template <typename T>
    T get(const QString& key, const T& defaultValue = {}) const
    {
        const auto qv = value(key, QVariant());
        return qv.isValid() && qv.canConvert<T>() ? qv.value<T>() : defaultValue;
    }

    Q_INVOKABLE bool contains(const QString& key) const;
    Q_INVOKABLE QStringList childGroups() const;

private:
    static QString legacyOrganizationName;
    static QString legacyApplicationName;

protected:
    QSettings legacySettings { legacyOrganizationName, legacyApplicationName };
};

class QUOTIENT_API SettingsGroup : public Settings {
public:
    explicit SettingsGroup(QString path, QObject* parent = nullptr)
        : Settings(parent)
        , groupPath(std::move(path))
    {}

    Q_INVOKABLE bool contains(const QString& key) const;
    Q_INVOKABLE QVariant value(const QString& key,
                               const QVariant& defaultValue = {}) const;

    template <typename T>
    T get(const QString& key, const T& defaultValue = {}) const
    {
        const auto qv = value(key, QVariant());
        return qv.isValid() && qv.canConvert<T>() ? qv.value<T>() : defaultValue;
    }

    Q_INVOKABLE QString group() const;
    Q_INVOKABLE QStringList childGroups() const;
    Q_INVOKABLE void setValue(const QString& key, const QVariant& value);

    Q_INVOKABLE void remove(const QString& key);

private:
    QString groupPath;
};

#define QUO_DECLARE_SETTING(type, propname, setter)      \
    Q_PROPERTY(type propname READ propname WRITE setter) \
public:                                                  \
    type propname() const;                               \
    void setter(type newValue);                          \
                                                         \
private:

#define QUO_DEFINE_SETTING(classname, type, propname, qsettingname,   \
                           defaultValue, setter)                      \
    type classname::propname() const                                  \
    {                                                                 \
        return get<type>(QStringLiteral(qsettingname), defaultValue); \
    }                                                                 \
                                                                      \
    void classname::setter(type newValue)                             \
    {                                                                 \
        setValue(QStringLiteral(qsettingname), std::move(newValue));  \
    }

class QUOTIENT_API AccountSettings : public SettingsGroup {
    Q_OBJECT
    Q_PROPERTY(QString userId READ userId CONSTANT)
    QUO_DECLARE_SETTING(QString, deviceId, setDeviceId)
    QUO_DECLARE_SETTING(QString, deviceName, setDeviceName)
    QUO_DECLARE_SETTING(bool, keepLoggedIn, setKeepLoggedIn)
    QUO_DECLARE_SETTING(QString, clientId, setClientId)
    QUO_DECLARE_SETTING(QString, tokenEndpoint, setTokenEndpoint)
    Q_PROPERTY(QByteArray encryptionAccountPickle READ encryptionAccountPickle
                   WRITE setEncryptionAccountPickle)
public:
    explicit AccountSettings(const QString& accountId, QObject* parent = nullptr)
        : SettingsGroup("Accounts/"_ls + accountId, parent)
    {}

    QString userId() const;

    QUrl homeserver() const;
    void setHomeserver(const QUrl& url);

    QByteArray encryptionAccountPickle();
    void setEncryptionAccountPickle(const QByteArray& encryptionAccountPickle);
    Q_INVOKABLE void clearEncryptionAccountPickle();
};
} // namespace Quotient
