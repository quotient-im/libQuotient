// SPDX-FileCopyrightText: 2018 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "networkaccessmanager.h"

#include <QtCore/QCoreApplication>
#include <QtNetwork/QNetworkReply>
#include "accountregistry.h"
#include "mxcreply.h"
#include "connection.h"

#include "room.h"

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
    if(request.url().scheme() == QStringLiteral("mxc")) {
        const auto fragment = request.url().fragment();
        const auto fragmentParts = fragment.split(QLatin1Char('/'));
        const auto mediaId = request.url().toString(QUrl::RemoveScheme | QUrl::RemoveFragment);
        if(fragmentParts.size() == 3) {
            auto connection = AccountRegistry::instance().get(fragmentParts[0]);
            if(!connection) {
                qWarning() << "Connection not found";
                return nullptr;
            }
            auto room = connection->room(fragmentParts[1]);
            if(!room) {
                qWarning() << "Room not found";
                return nullptr;
            }
            QNetworkRequest r(request);
            r.setUrl(QUrl(QStringLiteral("%1/_matrix/media/r0/download/%2").arg(connection->homeserver().toString(), mediaId)));
            auto reply = createRequest(QNetworkAccessManager::GetOperation, r);
            return new MxcReply(reply, room, fragmentParts[2]);
        } else if(fragmentParts.size() == 1) {
            auto connection = AccountRegistry::instance().get(fragment);
            if(!connection) {
                qWarning() << "Connection not found";
                return nullptr;
            }
            QNetworkRequest r(request);
            r.setUrl(QUrl(QStringLiteral("%1/_matrix/media/r0/download/%2").arg(connection->homeserver().toString(), mediaId)));
            auto reply = createRequest(QNetworkAccessManager::GetOperation, r);
            return new MxcReply(reply);
        } else {
            qWarning() << "Invalid request";
            return nullptr;
        }
    }
    auto reply = QNetworkAccessManager::createRequest(op, request, outgoingData);
    reply->ignoreSslErrors(d->ignoredSslErrors);
    return reply;
}

QStringList NetworkAccessManager::supportedSchemesImplementation() const
{
    auto schemes = QNetworkAccessManager::supportedSchemesImplementation();
    schemes += QStringLiteral("mxc");
    return schemes;
}

QUrl NetworkAccessManager::urlForRoomEvent(Room *room, const QString &eventId, const QString &mediaId)
{
    return QUrl(QStringLiteral("mxc:%1#%2/%3/%4").arg(mediaId, room->connection()->userId(), room->id(), eventId));
}

QUrl NetworkAccessManager::urlForFile(Connection *connection, const QString &mediaId)
{
    return QUrl(QStringLiteral("mxc:%1#%2").arg(mediaId, connection->userId()));
}