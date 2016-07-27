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
#include <QtNetwork/QNetworkReply>
#include <QtCore/QUrlQuery>
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

class CallBase::Private
{
    public:
        Private(CallBase* _q, ConnectionData* _c)
            : q(_q), connection(_c), reply(nullptr), status(CallStatus::PendingResult)
        { }

        inline void sendRequest(const RequestParams& params);
        void gotReply();

    public:
        CallBase* q;

        ConnectionData* connection;

        QScopedPointer<QNetworkReply, NetworkReplyDeleter> reply;
        QByteArray rawData;

        CallStatus status;
};

void CallBase::Private::sendRequest(const RequestParams& params)
{
    QUrl url = connection->baseUrl();
    url.setPath( url.path() + "/" + params.apiPath() );
    QUrlQuery query = params.query();
    if( params.needsToken() )
        query.addQueryItem("access_token", connection->token());
    url.setQuery(query);

    QNetworkRequest req {url};
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
    req.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    req.setMaximumRedirectsAllowed(10);
#endif
    switch( params.type() )
    {
        case HttpType::Get:
            reply.reset( connection->nam()->get(req) );
            break;
        case HttpType::Post:
            reply.reset( connection->nam()->post(req, params.data()) );
            break;
        case HttpType::Put:
            reply.reset( connection->nam()->put(req, params.data()) );
            break;
    }
    connect( reply.data(), &QNetworkReply::sslErrors, q, &CallBase::sslErrors );
    connect( reply.data(), &QNetworkReply::finished, q, &CallBase::gotReply );
    QTimer::singleShot( 120*1000, q, SLOT(timeout()) );
}

void CallBase::gotReply()
{
    d->gotReply();
}

void CallBase::Private::gotReply()
{
    if( reply->error() == QNetworkReply::NoError )
    {
        rawData = reply->readAll();
        status = q->makeResult(rawData);
    }
    else
    {
        status.set(CallStatus::NetworkError,
                      tr("Network Error %1: %2")
                            .arg(reply->error())
                            .arg(reply->errorString())
                  );
    }
    q->finish(true);
}

void CallBase::timeout()
{
    setStatus({CallStatus::TimeoutError, "The job has timed out"});
    d->reply->disconnect(); // To avoid "Operation cancelled" false alarms
    abandon();
}

void CallBase::sslErrors(const QList<QSslError>& errors)
{
    foreach (const QSslError &error, errors) {
        qWarning() << "SSL ERROR" << error.errorString();
    }
    d->reply->ignoreSslErrors(); // TODO: insecure! should prompt user first
}

CallBase::CallBase(ConnectionData* data, QString name,
                               const RequestParams& params)
    : d(new Private(this, data))
{
    setObjectName(name);

    d->sendRequest(params);
}

CallBase::~CallBase()
{ }

void CallBase::abandon()
{
    finish(false);
}

CallStatus CallBase::status() const
{
    return d->status;
}

const QByteArray&CallBase::rawData() const
{
    return d->rawData;
}

void CallBase::setStatus(CallStatus cs)
{
    d->status = cs;
}

void CallBase::setStatus(int code)
{
    setStatus(CallStatus(code));
}

void CallBase::finish(bool emitResult)
{
    if (!status().good())
    {
        qWarning() << "Server call" << objectName() << "failed:" << status();
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
        if (status().good())
            emit success();
        else
            emit failure();
    }

    d->reply.reset();
    deleteLater();
}
