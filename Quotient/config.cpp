// SPDX-FileCopyrightText: 2024 Tobias Fella <tobias.fella@kde.org>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "config.h"

#include <QSettings>

class QSettingsConfig : public Config
{
    //TODO this backend currently ignores the type and stores everything in the same backend
    QString load(const QString& group, const QString& childGroup, const QString& key, Type type) override
    {
        settings.beginGroup(group);
        settings.beginGroup(childGroup);
        auto value = settings.value(key).toString();
        settings.endGroup();
        settings.endGroup();
        return value;
    }

    void store(const QString& group, const QString& childGroup, const QString& key, const QString& value, Type type) override
    {
        settings.beginGroup(group);
        settings.beginGroup(childGroup);
        settings.setValue(key, value);
        settings.endGroup();
        settings.endGroup();
        settings.sync();
    }

    QStringList childGroups(const QString& group, Type type) override
    {
        settings.beginGroup(group);
        auto childGroups = settings.childGroups();
        settings.endGroup();
        return childGroups;
    }

    QSettings settings;
};

Config* Config::s_instance = nullptr;

Config* Config::instance()
{
    if (!s_instance) {
        s_instance = new QSettingsConfig();
    }
    return s_instance;
}

void Config::setInstance(Config* config)
{
    s_instance = config;
}
