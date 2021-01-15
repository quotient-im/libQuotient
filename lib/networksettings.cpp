/******************************************************************************
 * SPDX-FileCopyrightText: 2017 Kitsune Ral <kitsune-ral@users.sf.net>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "networksettings.h"

using namespace Quotient;

void NetworkSettings::setupApplicationProxy() const
{
    QNetworkProxy::setApplicationProxy(
        { proxyType(), proxyHostName(), proxyPort() });
}

QTNT_DEFINE_SETTING(NetworkSettings, QNetworkProxy::ProxyType, proxyType,
                   "proxy_type", QNetworkProxy::DefaultProxy, setProxyType)
QTNT_DEFINE_SETTING(NetworkSettings, QString, proxyHostName, "proxy_hostname",
                   {}, setProxyHostName)
QTNT_DEFINE_SETTING(NetworkSettings, quint16, proxyPort, "proxy_port", -1,
                   setProxyPort)
