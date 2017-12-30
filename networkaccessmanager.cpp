/******************************************************************************
 * Copyright (C) 2018 Kitsune Ral <kitsune-ral@users.sf.net>
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

#include "networkaccessmanager.h"

#include <QtNetwork/QNetworkReply>

using namespace QMatrixClient;

class NetworkAccessManager::Private
{
    public:
        QList<QSslError> ignoredSslErrors;
};

NetworkAccessManager::NetworkAccessManager() : d(std::make_unique<Private>())
{ }

QList<QSslError> NetworkAccessManager::ignoredSslErrors() const
{
    return d->ignoredSslErrors;
}

void NetworkAccessManager::addIgnoredSslError(const QSslError& error)
{
    d->ignoredSslErrors << error;
}

void NetworkAccessManager::clearIgnoredSslErrors()
{
    d->ignoredSslErrors.clear();
}

static NetworkAccessManager* createNam()
{
    auto nam = new NetworkAccessManager;
    // See #109. Once Qt bearer management gets better, this workaround
    // should become unnecessary.
    nam->connect(nam, &QNetworkAccessManager::networkAccessibleChanged,
        [nam] { nam->setNetworkAccessible(QNetworkAccessManager::Accessible); });
    return nam;
}

NetworkAccessManager*NetworkAccessManager::instance()
{
    static auto* nam = createNam();
    return nam;
}

NetworkAccessManager::~NetworkAccessManager() = default;

QNetworkReply* NetworkAccessManager::createRequest(Operation op,
    const QNetworkRequest& request, QIODevice* outgoingData)
{
    auto reply =
            QNetworkAccessManager::createRequest(op, request, outgoingData);
    reply->ignoreSslErrors(d->ignoredSslErrors);
    return reply;
}
