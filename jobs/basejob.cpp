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
            : connection(c), reply(nullptr), type(t), needsToken(nt), errorCode(NoError)
        {}
        
        ConnectionData* connection;
        QScopedPointer<QNetworkReply, NetworkReplyDeleter> reply;
        JobHttpType type;
        bool needsToken;

        int errorCode;
        QString errorText;
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
    if (checkReply(d->reply.data()))
        parseReply(d->reply->readAll());
    // FIXME: we should not hold parseReply()/parseJson() responsible for
    // emitting the result; it should be done here instead.
}

bool BaseJob::checkReply(QNetworkReply* reply)
{
    switch( reply->error() )
    {
    case QNetworkReply::NoError:
        return true;

    case QNetworkReply::AuthenticationRequiredError:
    case QNetworkReply::ContentAccessDenied:
    case QNetworkReply::ContentOperationNotPermittedError:
        qDebug() << "Content access error, Qt error code:" << reply->error();
        fail( ContentAccessError, reply->errorString() );
        return false;

    default:
        qDebug() << "NetworkError, Qt error code:" << reply->error();
        fail( NetworkError, reply->errorString() );
        return false;
    }
}

void BaseJob::parseReply(QByteArray data)
{
    QJsonParseError error;
    QJsonDocument json = QJsonDocument::fromJson(data, &error);
    if( error.error == QJsonParseError::NoError )
        parseJson(json);
    else
        fail( JsonParseError, error.errorString() );
}

void BaseJob::parseJson(const QJsonDocument&)
{
    // Do nothing by default
    emitResult();
}

void BaseJob::finishJob(bool emitResult)
{
    if (!d->reply)
    {
        qWarning() << objectName()
                   << ": empty network reply (finish() called more than once?)";
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

int BaseJob::error() const
{
    return d->errorCode;
}

QString BaseJob::errorString() const
{
    return d->errorText;
}

void BaseJob::setError(int errorCode)
{
    d->errorCode = errorCode;
}

void BaseJob::setErrorText(QString errorText)
{
    d->errorText = errorText;
}

void BaseJob::emitResult()
{
    finishJob(true);
}

void BaseJob::abandon()
{
    finishJob(false);
}

void BaseJob::fail(int errorCode, QString errorString)
{
    setError( errorCode );
    setErrorText( errorString );
    qWarning() << "Job" << objectName() << "failed:" << errorString;
    emitResult();
}

void BaseJob::timeout()
{
    fail( TimeoutError, "The job has timed out" );
}

void BaseJob::sslErrors(const QList<QSslError>& errors)
{
    foreach (const QSslError &error, errors) {
        qWarning() << "SSL ERROR" << error.errorString();
    }
    d->reply->ignoreSslErrors(); // TODO: insecure! should prompt user first
}
