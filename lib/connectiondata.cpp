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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include "connectiondata.h"

#include "logging.h"
#include "networkaccessmanager.h"

using namespace Quotient;

struct ConnectionData::Private {
    explicit Private(QUrl url) : baseUrl(std::move(url)) {}

    QUrl baseUrl;
    QByteArray accessToken;
    QString lastEvent;
    QString userId;
    QString deviceId;

    mutable unsigned int txnCounter = 0;
    const qint64 txnBase = QDateTime::currentMSecsSinceEpoch();
};

ConnectionData::ConnectionData(QUrl baseUrl)
    : d(std::make_unique<Private>(std::move(baseUrl)))
{}

ConnectionData::~ConnectionData() = default;

QByteArray ConnectionData::accessToken() const { return d->accessToken; }

QUrl ConnectionData::baseUrl() const { return d->baseUrl; }

QNetworkAccessManager* ConnectionData::nam() const
{
    return NetworkAccessManager::instance();
}

void ConnectionData::setBaseUrl(QUrl baseUrl)
{
    d->baseUrl = std::move(baseUrl);
    qCDebug(MAIN) << "updated baseUrl to" << d->baseUrl;
}

void ConnectionData::setToken(QByteArray token)
{
    d->accessToken = std::move(token);
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

const QString& ConnectionData::deviceId() const { return d->deviceId; }

const QString& ConnectionData::userId() const { return d->userId; }

void ConnectionData::setDeviceId(const QString& deviceId)
{
    d->deviceId = deviceId;
}

void ConnectionData::setUserId(const QString& userId) { d->userId = userId; }

QString ConnectionData::lastEvent() const { return d->lastEvent; }

void ConnectionData::setLastEvent(QString identifier)
{
    d->lastEvent = std::move(identifier);
}

QByteArray ConnectionData::generateTxnId() const
{
    return d->deviceId.toLatin1() + QByteArray::number(d->txnBase)
           + QByteArray::number(++d->txnCounter);
}
