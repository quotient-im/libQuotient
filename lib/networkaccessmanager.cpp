// SPDX-FileCopyrightText: 2018 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "networkaccessmanager.h"

#include <QtCore/QCoreApplication>
#include <QtNetwork/QNetworkReply>

using namespace Quotient;

class NetworkAccessManager::Private {
public:
    QList<QSslError> ignoredSslErrors;
};

NetworkAccessManager::NetworkAccessManager(QObject* parent)
    : QNetworkAccessManager(parent), d(std::make_unique<Private>())
{}

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
    auto nam = new NetworkAccessManager(QCoreApplication::instance());
#if (QT_VERSION < QT_VERSION_CHECK(5, 15, 0))
    // See #109; in newer Qt, bearer management is deprecated altogether
    NetworkAccessManager::connect(nam,
        &QNetworkAccessManager::networkAccessibleChanged, [nam] {
            nam->setNetworkAccessible(QNetworkAccessManager::Accessible);
        });
#endif
    return nam;
}

NetworkAccessManager* NetworkAccessManager::instance()
{
    static auto* nam = createNam();
    return nam;
}

NetworkAccessManager::~NetworkAccessManager() = default;

QNetworkReply* NetworkAccessManager::createRequest(
    Operation op, const QNetworkRequest& request, QIODevice* outgoingData)
{
    auto reply = QNetworkAccessManager::createRequest(op, request, outgoingData);
    reply->ignoreSslErrors(d->ignoredSslErrors);
    return reply;
}
