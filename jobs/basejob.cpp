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

#include "basejob.h"

#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QSslError>
#include <QtCore/QTimer>

#include "../connectiondata.h"

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

class BaseJob::Private
{
    public:
        Private(ConnectionData* c, JobHttpType t, bool nt)
            : connection(c), type(t), needsToken(nt)
            , reply(nullptr), status(NoError)
        {}
        
        ConnectionData* connection;
        JobHttpType type;
        bool needsToken;

        QScopedPointer<QNetworkReply, NetworkReplyDeleter> reply;
        Status status;
};

BaseJob::BaseJob(ConnectionData* connection, JobHttpType type, QString name, bool needsToken)
    : d(new Private(connection, type, needsToken))
{
    setObjectName(name);
    qDebug() << "Job" << objectName() << " created";
}

BaseJob::~BaseJob()
{
    qDebug() << "Job" << objectName() << " destroyed";
}

ConnectionData* BaseJob::connection() const
{
    return d->connection;
}

QJsonObject BaseJob::data() const
{
    return QJsonObject();
}

QUrlQuery BaseJob::query() const
{
    return QUrlQuery();
}

void BaseJob::start()
{
    QUrl url = d->connection->baseUrl();
    url.setPath( url.path() + "/" + apiPath() );
    QUrlQuery query = this->query();
    if( d->needsToken )
        query.addQueryItem("access_token", connection()->token());
    url.setQuery(query);
    QNetworkRequest req = QNetworkRequest(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
    req.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    req.setMaximumRedirectsAllowed(10);
#endif
    QJsonDocument data = QJsonDocument(this->data());
    switch( d->type )
    {
        case JobHttpType::GetJob:
            d->reply.reset( d->connection->nam()->get(req) );
            break;
        case JobHttpType::PostJob:
            d->reply.reset( d->connection->nam()->post(req, data.toJson()) );
            break;
        case JobHttpType::PutJob:
            d->reply.reset( d->connection->nam()->put(req, data.toJson()) );
            break;
    }
    connect( d->reply.data(), &QNetworkReply::sslErrors, this, &BaseJob::sslErrors );
    connect( d->reply.data(), &QNetworkReply::finished, this, &BaseJob::gotReply );
    QTimer::singleShot( 120*1000, this, SLOT(timeout()) );
//     connect( d->reply, static_cast<void(QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error),
//              this, &BaseJob::networkError ); // http://doc.qt.io/qt-5/qnetworkreply.html#error-1
}

void BaseJob::gotReply()
{
    setStatus(checkReply(d->reply.data()));
    if (status().good())
        setStatus(parseReply(d->reply->readAll()));

    finishJob(true);
}

BaseJob::Status BaseJob::checkReply(QNetworkReply* reply) const
{
    switch( reply->error() )
    {
    case QNetworkReply::NoError:
        return NoError;

    case QNetworkReply::AuthenticationRequiredError:
    case QNetworkReply::ContentAccessDenied:
    case QNetworkReply::ContentOperationNotPermittedError:
        qDebug() << "Content access error, Qt error code:" << reply->error();
        return { ContentAccessError, reply->errorString() };

    default:
        qDebug() << "NetworkError, Qt error code:" << reply->error();
        return { NetworkError, reply->errorString() };
    }
}

BaseJob::Status BaseJob::parseReply(QByteArray data)
{
    QJsonParseError error;
    QJsonDocument json = QJsonDocument::fromJson(data, &error);
    if( error.error == QJsonParseError::NoError )
        return parseJson(json);
    else
        return { JsonParseError, error.errorString() };
}

BaseJob::Status BaseJob::parseJson(const QJsonDocument&)
{
    return Success;
}

void BaseJob::finishJob(bool emitResult)
{
    if (!d->reply)
    {
        qWarning() << objectName()
                   << ": empty network reply (finishJob() called more than once?)";
        return;
    }

    // Notify those that are interested in any completion of the job (including killing)
    emit finished(this);

    if (emitResult) {
        emit result(this);
        if (error())
            emit failure(this);
        else
            emit success(this);
    }

    d->reply.reset();
    deleteLater();
}

BaseJob::Status BaseJob::status() const
{
    return d->status;
}

int BaseJob::error() const
{
    return d->status.code;
}

QString BaseJob::errorString() const
{
    return d->status.message;
}

void BaseJob::setStatus(Status s)
{
    d->status = s;
    if (!s.good())
    {
        qWarning() << QString("Job %1 status: %2, code %3")
                      .arg(objectName()).arg(s.message).arg(s.code);
    }
}

void BaseJob::setStatus(int code, QString message)
{
    setStatus({ code, message });
}

void BaseJob::abandon()
{
    finishJob(false);
}

void BaseJob::timeout()
{
    setStatus( TimeoutError, "The job has timed out" );
    finishJob(true);
}

void BaseJob::sslErrors(const QList<QSslError>& errors)
{
    foreach (const QSslError &error, errors) {
        qWarning() << "SSL ERROR" << error.errorString();
    }
    d->reply->ignoreSslErrors(); // TODO: insecure! should prompt user first
}
