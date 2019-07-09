/******************************************************************************
 * Copyright (C) 2016 Kitsune Ral <kitsune-ral@users.sf.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

#pragma once

#include <QtCore/QSettings>
#include <QtCore/QUrl>
#include <QtCore/QVector>

class QVariant;

namespace QMatrixClient
{
class Settings : public QSettings
{
    Q_OBJECT
public:
    /**
     * Use this function before creating any Settings objects in order
     * to setup a read-only location where configuration has previously
     * been stored. This will provide an additional fallback in case of
     * renaming the organisation/application.
     */
    static void setLegacyNames(const QString& organizationName,
                               const QString& applicationName = {});

#if defined(_MSC_VER) && _MSC_VER < 1900
    // VS 2013 (and probably older) aren't friends with 'using' statements
    // that involve private constructors
    explicit Settings(QObject* parent = 0)
        : QSettings(parent)
    {}
#else
    using QSettings::QSettings;
#endif

    Q_INVOKABLE void setValue(const QString& key, const QVariant& value);
    Q_INVOKABLE QVariant value(const QString& key,
                               const QVariant& defaultValue = {}) const;

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

class SettingsGroup : public Settings
{
public:
    template <typename... ArgTs>
    explicit SettingsGroup(QString path, ArgTs&&... qsettingsArgs)
        : Settings(std::forward<ArgTs>(qsettingsArgs)...)
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

#define QMC_DECLARE_SETTING(type, propname, setter)      \
    Q_PROPERTY(type propname READ propname WRITE setter) \
public:                                                  \
    type propname() const;                               \
    void setter(type newValue);                          \
                                                         \
private:

#define QMC_DEFINE_SETTING(classname, type, propname, qsettingname,   \
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

class AccountSettings : public SettingsGroup
{
    Q_OBJECT
    Q_PROPERTY(QString userId READ userId CONSTANT)
    QMC_DECLARE_SETTING(QString, deviceId, setDeviceId)
    QMC_DECLARE_SETTING(QString, deviceName, setDeviceName)
    QMC_DECLARE_SETTING(bool, keepLoggedIn, setKeepLoggedIn)
    /** \deprecated \sa setAccessToken */
    Q_PROPERTY(QString accessToken READ accessToken WRITE setAccessToken)
    Q_PROPERTY(QByteArray encryptionAccountPickle READ encryptionAccountPickle
                   WRITE setEncryptionAccountPickle)
public:
    template <typename... ArgTs>
    explicit AccountSettings(const QString& accountId, ArgTs... qsettingsArgs)
        : SettingsGroup("Accounts/" + accountId, qsettingsArgs...)
    {}

    QString userId() const;

    QUrl homeserver() const;
    void setHomeserver(const QUrl& url);

    /** \deprecated \sa setToken */
    QString accessToken() const;
    /** \deprecated Storing accessToken in QSettings is unsafe,
     * see QMatrixClient/Quaternion#181 */
    void setAccessToken(const QString& accessToken);
    Q_INVOKABLE void clearAccessToken();

    QByteArray encryptionAccountPickle();
    void setEncryptionAccountPickle(const QByteArray& encryptionAccountPickle);
    Q_INVOKABLE void clearEncryptionAccountPickle();
};
} // namespace QMatrixClient
