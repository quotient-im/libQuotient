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

#include "servercall.h"

#include "connectiondata.h"
#include "servercallsetup.h"

#include <QtNetwork/QNetworkRequest>
#include <QtCore/QUrlQuery>
#include <QtCore/QTimer>

using namespace QMatrixClient;

struct NetworkReplyDeleter : public QScopedPointerDeleteLater
{
    static inline void cleanup(QNetworkReply* reply)
    {
        if (reply && reply->isRunning())
            reply->abort();
        QScopedPointerDeleteLater::cleanup(reply);
    }
};

class ServerCallBase::Private
{
    public:
        explicit Private(ConnectionData* c)
            : connection(c), reply(nullptr), pStatus(nullptr)
        { }

        inline void sendRequest(const RequestParams& params);

        ConnectionData* connection;

        QScopedPointer<QNetworkReply, NetworkReplyDeleter> reply;
        QByteArray rawData;

        // The actual status object is managed in ServerCall<>::setup
        CallStatus* pStatus;
};

void ServerCallBase::Private::sendRequest(const RequestParams& params)
{
    QUrl url = connection->baseUrl();
    url.setPath( url.path() + "/" + params.apiPath() );
    QUrlQuery query = params.query();
    if( params.needsToken() )
        query.addQueryItem("access_token", connection->token());
    url.setQuery(query);
    QNetworkRequest req = QNetworkRequest(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
    req.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    req.setMaximumRedirectsAllowed(10);
#endif
    switch( params.type() )
    {
        case RequestParams::HttpType::Get:
            reply.reset( connection->nam()->get(req) );
            break;
        case RequestParams::HttpType::Post:
            reply.reset( connection->nam()->post(req, params.data()) );
            break;
        case RequestParams::HttpType::Put:
            reply.reset( connection->nam()->put(req, params.data()) );
            break;
    }
}

ServerCallBase::ServerCallBase(ConnectionData* data, QString name)
    : d(new Private(data))
{
    setObjectName(name);
}

void ServerCallBase::init(ServerCallSetupBase& setup)
{
    d->pStatus = setup.statusPtr();
}

ServerCallBase::~ServerCallBase()
{ }

void ServerCallBase::doStart(const RequestParams& params)
{
    setStatus(CallStatus::PendingResult);

    d->sendRequest(params);
    connect( reply(), &QNetworkReply::sslErrors, this, &ServerCallBase::sslErrors );
    QTimer::singleShot( 120*1000, this, SLOT(timeout()) );
}

void ServerCallBase::abandon()
{
    finish(false);
}

bool ServerCallBase::pendingReply() const
{
    return d->reply && d->reply->isRunning();
}

CallStatus ServerCallBase::status() const
{
    Q_ASSERT(d->pStatus);
    return *(d->pStatus);
}

const QByteArray&ServerCallBase::rawData() const
{
    return d->rawData;
}

void ServerCallBase::setStatus(CallStatus cs)
{
    Q_ASSERT(d->pStatus);
    *(d->pStatus) = cs;
    if (!cs.good())
    {
        qWarning() << "Server call" << objectName() << "failed:"
                   << "CallStatus(" << cs.code << "," << cs.message << ")";
    }
}

void ServerCallBase::setStatus(int code)
{
    setStatus(CallStatus(code));
}

QNetworkReply* ServerCallBase::reply() const
{
    return d->reply.data();
}

bool ServerCallBase::readReply()
{
    if( reply()->error() == QNetworkReply::NoError )
    {
        setStatus(CallStatus::Success);
        d->rawData = reply()->readAll();
    }
    else
    {
        setStatus({ CallStatus::NetworkError,
              tr("%1 %2").arg(d->reply->error()).arg(d->reply->errorString()) });
    }
    return status().good();
}

void ServerCallBase::finish(bool emitResult)
{
    if (!d->reply)
    {
        qWarning() << objectName()
                   << ": empty network reply (finish() called more than once?)";
        return;
    }
    if (emitResult)
    {
        emit resultReady();
        if (status().good())
            emit success();
        else
            emit failure();
    }

    d->reply.reset();
    deleteLater();
}

void ServerCallBase::timeout()
{
    setStatus({ CallStatus::TimeoutError, "The job has timed out" });
    abandon();
}

void ServerCallBase::sslErrors(const QList<QSslError>& errors)
{
    foreach (const QSslError &error, errors) {
        qWarning() << "SSL ERROR" << error.errorString();
    }
    d->reply->ignoreSslErrors(); // TODO: insecure! should prompt user first
}
