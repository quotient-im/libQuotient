/******************************************************************************
 * Copyright (C) 2016 Kitsune Ral <kitsune-ral@users.sf.net>
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

#include "call.h"

#include "connectiondata.h"

#include <QtCore/QStringBuilder>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QtCore/QTimer>

using namespace QMatrixClient;
using namespace QMatrixClient::ServerApi;

struct NetworkReplyDeleter : public QScopedPointerDeleteLater
{
    static inline void cleanup(QNetworkReply* reply)
    {
        if (reply && reply->isRunning())
            reply->abort();
        QScopedPointerDeleteLater::cleanup(reply);
    }
};

class Call::Private
{
    public:
        Private(ConnectionData* _c) : connection(_c), reply(nullptr) { }

        inline void sendRequest(const RequestConfig& params);

        ConnectionData* connection;
        QScopedPointer<QNetworkReply, NetworkReplyDeleter> reply;
        QTimer timer;
};

void Call::Private::sendRequest(const RequestConfig& params)
{
    QUrl url = connection->baseUrl();
    url.setPath( url.path() + params.apiPath() );
    QUrlQuery query = params.query();
    if( params.needsToken() )
        query.addQueryItem("access_token", connection->accessToken());
    url.setQuery(query);

    QNetworkRequest req {url};
    req.setHeader(QNetworkRequest::ContentTypeHeader,
                  params.contentType().name());
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
    req.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    req.setMaximumRedirectsAllowed(10);
#endif
    switch( params.type() )
    {
        case JobHttpType::GetJob:
            reply.reset( connection->nam()->get(req) );
            break;
        case JobHttpType::PostJob:
            reply.reset( connection->nam()->post(req, params.data()) );
            break;
        case JobHttpType::PutJob:
            reply.reset( connection->nam()->put(req, params.data()) );
            break;
        case JobHttpType::DeleteJob:
            reply.reset( connection->nam()->deleteResource(req) );
            break;
    }
    timer.start( 120*1000 );
}

Call::Call(ConnectionData* data, const RequestConfig& config)
    : d(new Private(data))
{
    setObjectName(config.name());
    connect (&d->timer, &QTimer::timeout, this, &Call::timeout);

    d->sendRequest(config);
    connect( reply(), &QNetworkReply::sslErrors, this, &Call::sslErrors );
    connect( reply(), &QNetworkReply::finished, this, &Call::gotReply );
}

Call::~Call()
{ }

void Call::timeout()
{
    d->reply->disconnect(); // To avoid "Operation cancelled" false alarms
    finish({ TimeoutError, "The job has timed out" }, false);
}

void Call::sslErrors(const QList<QSslError>& errors)
{
    foreach (const QSslError &error, errors) {
        qWarning() << "SSL ERROR" << error.errorString();
    }
    d->reply->ignoreSslErrors(); // TODO: insecure! should prompt user first
}

QNetworkReply*Call::reply() const
{
    return d->reply.data();
}

void Call::finish(Status status, bool emitResult)
{
    if (!status.good())
    {
        qWarning() << "Server call" << objectName() << "failed:"
                   << status.message << ", code" << status.code;
    }

    if (!d->reply)
    {
        qWarning() << objectName()
                   << ": empty network reply (finish() called more than once?)";
        return;
    }
    if (emitResult)
    {
        emit resultReady();
        if (status.good())
            emit success();
        else
            emit failure(status);
    }

    d->reply.reset();
    deleteLater();
}

