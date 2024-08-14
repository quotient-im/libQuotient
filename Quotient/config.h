// SPDX-FileCopyrightText: 2024 Tobias Fella <tobias.fella@kde.org>
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include <QString>

class Config
{
public:
    enum Type
    {
        Settings,
        Data,
    };
    virtual QString load(const QString& group, const QString& childGroup, const QString& key, Type type) = 0;
    virtual void store(const QString& group, const QString& childGroup, const QString& key, const QString& value, Type type) = 0;
    virtual QStringList childGroups(const QString& group, Type type) = 0;

    static Config* instance();
    static void setInstance(Config* config);

private:
    static Config* s_instance;
};
