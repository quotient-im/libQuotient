#include "settings.h"

#include <QtCore/QUrl>
#include <QtCore/QDebug>

using namespace QMatrixClient;

Settings::~Settings()
{ }

void Settings::setValue(const QString& key, const QVariant& value)
{
//    qCDebug() << "Setting" << key << "to" << value;
    QSettings::setValue(key, value);
}

QVariant Settings::value(const QString& key, const QVariant& defaultValue) const
{
    return QSettings::value(key, defaultValue);
}

SettingsGroup::~SettingsGroup()
{ }

void SettingsGroup::setValue(const QString& key, const QVariant& value)
{
    Settings::setValue(groupPath + "/" + key, value);
}

bool SettingsGroup::contains(const QString& key) const
{
    return Settings::contains(groupPath + "/" + key);
}

QVariant SettingsGroup::value(const QString& key, const QVariant& defaultValue) const
{
    return Settings::value(groupPath + "/" + key, defaultValue);
}

QString SettingsGroup::group() const
{
    return groupPath;
}

QStringList SettingsGroup::childGroups() const
{
    const_cast<SettingsGroup*>(this)->beginGroup(groupPath);
    QStringList l { Settings::childGroups() };
    const_cast<SettingsGroup*>(this)->endGroup();
    return l;
}

void SettingsGroup::remove(const QString& key)
{
    QString fullKey { groupPath };
    if (!key.isEmpty())
        fullKey += "/" + key;
    Settings::remove(fullKey);
}

AccountSettings::~AccountSettings()
{ }

bool AccountSettings::keepLoggedIn() const
{
    return value("keep_logged_in", false).toBool();
}

void AccountSettings::setKeepLoggedIn(bool newSetting)
{
    setValue("keep_logged_in", newSetting);
}

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
    setValue("access_token", accessToken);
}

void AccountSettings::clearAccessToken()
{
    remove("access_token");
}
