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

#include "networksettings.h"

using namespace QMatrixClient;

void NetworkSettings::setupApplicationProxy() const
{
    QNetworkProxy::setApplicationProxy(
        { proxyType(), proxyHostName(), proxyPort() });
}

QMC_DEFINE_SETTING(NetworkSettings, QNetworkProxy::ProxyType, proxyType, "proxy_type", QNetworkProxy::DefaultProxy, setProxyType)
QMC_DEFINE_SETTING(NetworkSettings, QString, proxyHostName, "proxy_hostname", {}, setProxyHostName)
QMC_DEFINE_SETTING(NetworkSettings, quint16, proxyPort, "proxy_port", -1, setProxyPort)
