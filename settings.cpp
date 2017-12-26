#include "settings.h"

#include "logging.h"

#include <QtCore/QUrl>

using namespace QMatrixClient;

QString Settings::legacyOrganizationName {};
QString Settings::legacyApplicationName {};

void Settings::setLegacyNames(const QString& organizationName,
                              const QString& applicationName)
{
    legacyOrganizationName = organizationName;
    legacyApplicationName = applicationName;
}

void Settings::setValue(const QString& key, const QVariant& value)
{
//    qCDebug() << "Setting" << key << "to" << value;
    QSettings::setValue(key, value);
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
    auto l = QSettings::childGroups();
    return !l.isEmpty() ? l : legacySettings.childGroups();
}

void SettingsGroup::setValue(const QString& key, const QVariant& value)
{
    Settings::setValue(groupPath + '/' + key, value);
}

bool SettingsGroup::contains(const QString& key) const
{
    return Settings::contains(groupPath + '/' + key);
}

QVariant SettingsGroup::value(const QString& key, const QVariant& defaultValue) const
{
    return Settings::value(groupPath + '/' + key, defaultValue);
}

QString SettingsGroup::group() const
{
    return groupPath;
}

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

QMC_DEFINE_SETTING(AccountSettings, QString, deviceId, "device_id", "", setDeviceId)
QMC_DEFINE_SETTING(AccountSettings, QString, deviceName, "device_name", "", setDeviceName)
QMC_DEFINE_SETTING(AccountSettings, bool, keepLoggedIn, "keep_logged_in", false, setKeepLoggedIn)

QUrl AccountSettings::homeserver() const
{
    return QUrl::fromUserInput(value("homeserver").toString());
}

void AccountSettings::setHomeserver(const QUrl& url)
{
    setValue("homeserver", url.toString());
}

QString AccountSettings::userId() const
{
    return group().section('/', -1);
}

QString AccountSettings::accessToken() const
{
    return value("access_token").toString();
}

void AccountSettings::setAccessToken(const QString& accessToken)
{
    qCWarning(MAIN) << "Saving access_token to QSettings is insecure."
                       " Developers, please save access_token separately.";
    setValue("access_token", accessToken);
}

void AccountSettings::clearAccessToken()
{
    legacySettings.remove("access_token");
    legacySettings.remove("device_id"); // Force the server to re-issue it
    remove("access_token");
}
