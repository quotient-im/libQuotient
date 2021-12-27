// SPDX-FileCopyrightText: 2016 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "settings.h"

#include "logging.h"

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
{
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    setIniCodec("UTF-8");
#endif
}

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
    Settings::setValue(groupPath + '/' + key, value);
}

bool SettingsGroup::contains(const QString& key) const
{
    return Settings::contains(groupPath + '/' + key);
}

QVariant SettingsGroup::value(const QString& key,
                              const QVariant& defaultValue) const
{
    return Settings::value(groupPath + '/' + key, defaultValue);
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
        fullKey += "/" + key;
    Settings::remove(fullKey);
}

QTNT_DEFINE_SETTING(AccountSettings, QString, deviceId, "device_id", {},
                   setDeviceId)
QTNT_DEFINE_SETTING(AccountSettings, QString, deviceName, "device_name", {},
                   setDeviceName)
QTNT_DEFINE_SETTING(AccountSettings, bool, keepLoggedIn, "keep_logged_in", false,
                   setKeepLoggedIn)

static const auto HomeserverKey = QStringLiteral("homeserver");
static const auto AccessTokenKey = QStringLiteral("access_token");
static const auto EncryptionAccountPickleKey =
    QStringLiteral("encryption_account_pickle");

QUrl AccountSettings::homeserver() const
{
    return QUrl::fromUserInput(value(HomeserverKey).toString());
}

void AccountSettings::setHomeserver(const QUrl& url)
{
    setValue(HomeserverKey, url.toString());
}

QString AccountSettings::userId() const { return group().section('/', -1); }

void AccountSettings::clearAccessToken()
{
    legacySettings.remove(AccessTokenKey);
    legacySettings.remove(QStringLiteral("device_id")); // Force the server to
                                                        // re-issue it
    remove(AccessTokenKey);
}

QByteArray AccountSettings::encryptionAccountPickle()
{
    QString passphrase = ""; // FIXME: add QtKeychain
    return value("encryption_account_pickle", "").toByteArray();
}

void AccountSettings::setEncryptionAccountPickle(
    const QByteArray& encryptionAccountPickle)
{
    qCWarning(MAIN)
        << "Saving encryption_account_pickle to QSettings is insecure."
           " Developers, do it manually or contribute to share QtKeychain "
           "logic to libQuotient.";
    QString passphrase = ""; // FIXME: add QtKeychain
    setValue("encryption_account_pickle", encryptionAccountPickle);
}

void AccountSettings::clearEncryptionAccountPickle()
{
    remove(EncryptionAccountPickleKey); // TODO: Force to re-issue it?
}
