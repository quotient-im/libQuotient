// SPDX-FileCopyrightText: 2017 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "settings.h"

#include <QtNetwork/QNetworkProxy>

Q_DECLARE_METATYPE(QNetworkProxy::ProxyType)

namespace Quotient {
class NetworkSettings : public SettingsGroup {
    Q_OBJECT
    QUO_DECLARE_SETTING(QNetworkProxy::ProxyType, proxyType, setProxyType)
    QUO_DECLARE_SETTING(QString, proxyHostName, setProxyHostName)
    QUO_DECLARE_SETTING(quint16, proxyPort, setProxyPort)
    Q_PROPERTY(QString proxyHost READ proxyHostName WRITE setProxyHostName)
public:
    template <typename... ArgTs>
    explicit NetworkSettings(ArgTs... qsettingsArgs)
        : SettingsGroup(QStringLiteral("Network"), qsettingsArgs...)
    {}
    ~NetworkSettings() override = default;

    Q_INVOKABLE void setupApplicationProxy() const;
};
} // namespace Quotient
