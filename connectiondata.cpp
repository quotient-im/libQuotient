/******************************************************************************
 * Copyright (C) 2015 Felix Rohrbach <kde@fxrh.de>
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

#include "connectiondata.h"

#include "logging.h"

#include <QtNetwork/QNetworkAccessManager>

using namespace QMatrixClient;

struct ConnectionData::Private
{
    explicit Private(const QUrl& url) : baseUrl(url) { }

    QUrl baseUrl;
    QByteArray accessToken;
    QString lastEvent;
    QString deviceId;

    mutable unsigned int txnCounter = 0;
    const qint64 id = QDateTime::currentMSecsSinceEpoch();

    static QNetworkAccessManager* createNam();
    static nam_customizer_t customizeNam;
};

ConnectionData::nam_customizer_t ConnectionData::Private::customizeNam =
    [] (auto* /* unused */) { };

ConnectionData::ConnectionData(QUrl baseUrl)
    : d(std::make_unique<Private>(baseUrl))
{ }

ConnectionData::~ConnectionData() = default;

QByteArray ConnectionData::accessToken() const
{
    return d->accessToken;
}

QUrl ConnectionData::baseUrl() const
{
    return d->baseUrl;
}

QNetworkAccessManager* ConnectionData::Private::createNam()
{
    auto nam = new QNetworkAccessManager;
    // See #109. Once Qt bearer management gets better, this workaround
    // should become unnecessary.
    nam->connect(nam, &QNetworkAccessManager::networkAccessibleChanged,
                 [nam] { nam->setNetworkAccessible(QNetworkAccessManager::Accessible);
                 });
    customizeNam(nam);
    return nam;
}

QNetworkAccessManager* ConnectionData::nam() const
{
    static auto nam = d->createNam();
    return nam;
}

void ConnectionData::setBaseUrl(QUrl baseUrl)
{
    d->baseUrl = baseUrl;
    qCDebug(MAIN) << "updated baseUrl to" << d->baseUrl;
}

void ConnectionData::setToken(QByteArray token)
{
    d->accessToken = token;
}

void ConnectionData::setHost(QString host)
{
    d->baseUrl.setHost(host);
    qCDebug(MAIN) << "updated baseUrl to" << d->baseUrl;
}

void ConnectionData::setPort(int port)
{
    d->baseUrl.setPort(port);
    qCDebug(MAIN) << "updated baseUrl to" << d->baseUrl;
}

const QString& ConnectionData::deviceId() const
{
    return d->deviceId;
}

void ConnectionData::setDeviceId(const QString& deviceId)
{
    d->deviceId = deviceId;
    qCDebug(MAIN) << "updated deviceId to" << d->deviceId;
}

QString ConnectionData::lastEvent() const
{
    return d->lastEvent;
}

void ConnectionData::setLastEvent(QString identifier)
{
    d->lastEvent = identifier;
}

QByteArray ConnectionData::generateTxnId() const
{
    return QByteArray::number(d->id) + 'q' +
            QByteArray::number(++d->txnCounter);
}

void
ConnectionData::customizeNetworkAccess(ConnectionData::nam_customizer_t customizer)
{
    Private::customizeNam = customizer;
}

