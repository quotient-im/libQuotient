// SPDX-FileCopyrightText: 2016 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "settings.h"

#include "logging_categories_p.h"
#include "util.h"

#include <QtCore/QUrl>

using namespace Quotient;

QString Settings::legacyOrganizationName {};
QString Settings::legacyApplicationName {};

void Settings::setLegacyNames(const QString& organizationName,
                              const QString& applicationName)
{
    legacyOrganizationName = organizationName;
    legacyApplicationName = applicationName;
}

Settings::Settings(QObject* parent) : QSettings(parent)
{}

void Settings::setValue(const QString& key, const QVariant& value)
{
    QSettings::setValue(key, value);
    if (legacySettings.contains(key))
        legacySettings.remove(key);
}

void Settings::remove(const QString& key)
{
    QSettings::remove(key);
    if (legacySettings.contains(key))
        legacySettings.remove(key);
}

QVariant Settings::value(const QString& key, const QVariant& defaultValue) const
{
    auto value = QSettings::value(key, legacySettings.value(key, defaultValue));
    // QML's Qt.labs.Settings stores boolean values as strings, which, if loaded
    // through the usual QSettings interface, confuses QML
    // (QVariant("false") == true in JavaScript). Since we have a mixed
    // environment where both QSettings and Qt.labs.Settings may potentially
    // work with same settings, better ensure compatibility.
    return value.toString() == QStringLiteral("false") ? QVariant(false) : value;
}

bool Settings::contains(const QString& key) const
{
    return QSettings::contains(key) || legacySettings.contains(key);
}

QStringList Settings::childGroups() const
{
    auto groups = QSettings::childGroups();
    const auto& legacyGroups = legacySettings.childGroups();
    for (const auto& g: legacyGroups)
        if (!groups.contains(g))
            groups.push_back(g);
    return groups;
}

void SettingsGroup::setValue(const QString& key, const QVariant& value)
{
    Settings::setValue(groupPath + u'/' + key, value);
}

bool SettingsGroup::contains(const QString& key) const
{
    return Settings::contains(groupPath + u'/' + key);
}

QVariant SettingsGroup::value(const QString& key,
                              const QVariant& defaultValue) const
{
    return Settings::value(groupPath + u'/' + key, defaultValue);
}

QString SettingsGroup::group() const { return groupPath; }

QStringList SettingsGroup::childGroups() const
{
    const_cast<SettingsGroup*>(this)->beginGroup(groupPath);
    const_cast<QSettings&>(legacySettings).beginGroup(groupPath);
    QStringList l = Settings::childGroups();
    const_cast<SettingsGroup*>(this)->endGroup();
    const_cast<QSettings&>(legacySettings).endGroup();
    return l;
}

void SettingsGroup::remove(const QString& key)
{
    QString fullKey { groupPath };
    if (!key.isEmpty())
        fullKey += u'/' + key;
    Settings::remove(fullKey);
}

QUO_DEFINE_SETTING(AccountSettings, QString, deviceId, "device_id", {}, setDeviceId)
QUO_DEFINE_SETTING(AccountSettings, QString, deviceName, "device_name", {}, setDeviceName)
QUO_DEFINE_SETTING(AccountSettings, bool, keepLoggedIn, "keep_logged_in", false, setKeepLoggedIn)
QUO_DEFINE_SETTING(AccountSettings, QString, clientId, "client_id", {}, setClientId)
QUO_DEFINE_SETTING(AccountSettings, QString, tokenEndpoint, "token_endpoint", {}, setTokenEndpoint)

namespace {
constexpr auto HomeserverKey = "homeserver"_ls;
constexpr auto EncryptionAccountPickleKey = "encryption_account_pickle"_ls;
}

QUrl AccountSettings::homeserver() const
{
    return QUrl::fromUserInput(value(HomeserverKey).toString());
}

void AccountSettings::setHomeserver(const QUrl& url)
{
    setValue(HomeserverKey, url.toString());
}

QString AccountSettings::userId() const { return group().section(u'/', -1); }

QByteArray AccountSettings::encryptionAccountPickle()
{
    return value("encryption_account_pickle"_ls, QString()).toByteArray();
}

void AccountSettings::setEncryptionAccountPickle(
    const QByteArray& encryptionAccountPickle)
{
    setValue("encryption_account_pickle"_ls, encryptionAccountPickle);
}

void AccountSettings::clearEncryptionAccountPickle()
{
    remove(EncryptionAccountPickleKey); // TODO: Force to re-issue it?
}
