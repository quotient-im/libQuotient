/******************************************************************************
 * Copyright (C) 2017 Kitsune Ral <kitsune-ral@users.sf.net>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#pragma once

#include "settings.h"

#include <QtNetwork/QNetworkProxy>

Q_DECLARE_METATYPE(QNetworkProxy::ProxyType)

namespace QMatrixClient {
    class NetworkSettings: public SettingsGroup
    {
            Q_OBJECT
            QMC_DECLARE_SETTING(QNetworkProxy::ProxyType, proxyType, setProxyType)
            QMC_DECLARE_SETTING(QString, proxyHostName, setProxyHostName)
            QMC_DECLARE_SETTING(quint16, proxyPort, setProxyPort)
            Q_PROPERTY(QString proxyHost READ proxyHostName WRITE setProxyHostName)
        public:
            template <typename... ArgTs>
            explicit NetworkSettings(ArgTs... qsettingsArgs)
                : SettingsGroup(QStringLiteral("Network"), qsettingsArgs...)
            { }
            ~NetworkSettings() override = default;

            Q_INVOKABLE void setupApplicationProxy() const;
    };
}
