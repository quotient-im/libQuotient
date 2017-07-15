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
#include <cstdlib>

using namespace QMatrixClient;

QNetworkAccessManager* getNam()
{
    static QNetworkAccessManager* _nam = new QNetworkAccessManager();
    return _nam;
}

struct ConnectionData::Private
{
    QUrl baseUrl;
    QString accessToken;
    QString lastEvent;

    mutable unsigned int txnCounter = 0;
    const int id = std::rand(); // We don't really care about pure randomness
};

ConnectionData::ConnectionData(QUrl baseUrl)
    : d(new Private)
{
    d->baseUrl = baseUrl;
}

ConnectionData::~ConnectionData()
{
    delete d;
}

QString ConnectionData::accessToken() const
{
    return d->accessToken;
}

QUrl ConnectionData::baseUrl() const
{
    return d->baseUrl;
}

QNetworkAccessManager* ConnectionData::nam() const
{
    return getNam();
}

void ConnectionData::setToken(QString token)
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
