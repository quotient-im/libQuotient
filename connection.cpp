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

#include "connection.h"
#include "connectiondata.h"
#include "user.h"
#include "events/event.h"
#include "room.h"
#include "serverapi/passwordlogin.h"
#include "serverapi/generated/logout.h"
#include "jobs/postmessagejob.h"
#include "serverapi/joinroom.h"
#include "jobs/leaveroomjob.h"
#include "jobs/roommessagesjob.h"
#include "jobs/syncjob.h"
#include "jobs/mediathumbnailjob.h"

#include <QtNetwork/QDnsLookup>
#include <QtCore/QDebug>

using namespace QMatrixClient;

class Connection::Private
{
    public:
        explicit Private(QUrl serverUrl)
            : q(nullptr)
            , data(new ConnectionData(serverUrl))
            , isConnected(false)
            , syncJob(nullptr)
        { }
        Private(Private&) = delete;
        ~Private() { delete data; }

        Connection* q;
        ConnectionData* data;
        QHash<QString, Room*> roomMap;
        QHash<QString, User*> userMap;
        bool isConnected;
        QString username;
        QString password;
        QString userId;

        SyncJob* syncJob;
};

Connection::Connection(QUrl server, QObject* parent)
    : QObject(parent)
    , d(new Private(server))
{
    d->q = this; // All d initialization should occur before this line
}

Connection::Connection()
    : Connection(QUrl("https://matrix.org"))
{
}

Connection::~Connection()
{
    qDebug() << "deconstructing connection object for" << d->userId;
    delete d;
}

void Connection::resolveServer(QString domain)
{
    // Find the Matrix server for the given domain.
    QScopedPointer<QDnsLookup, QScopedPointerDeleteLater> dns { new QDnsLookup() };
    dns->setType(QDnsLookup::SRV);
    dns->setName("_matrix._tcp." + domain);

    dns->lookup();
    connect(dns.data(), &QDnsLookup::finished, [&]() {
        // Check the lookup succeeded.
        if (dns->error() != QDnsLookup::NoError ||
                dns->serviceRecords().isEmpty()) {
            emit resolveError("DNS lookup failed");
            return;
        }

        // Handle the results.
        auto record = dns->serviceRecords().front();
        d->data->setHost(record.target());
        d->data->setPort(record.port());
        emit resolved();
    });
}

void Connection::connectToServer(QString user, QString password)
{
    using namespace ServerApi;
    auto loginJob = callServer(PasswordLogin(user, password));
    loginJob->onSuccess([=] (const AccessData& result) {
        connectWithToken(result.id, result.token);
    });
    loginJob->onFailure([=] (Status status) {
        emit loginError(status.message);
    });
    d->username = user; // to be able to reconnect
    d->password = password;
}

void Connection::connectWithToken(QString userId, QString token)
{
    d->isConnected = true;
    d->userId = userId;
    d->data->setToken(token);
    qDebug() << "Accessing" << d->data->baseUrl()
             << "by user" << userId
             << "with the following access token:";
    qDebug() << token;
    emit connected();
}

void Connection::reconnect()
{
    using namespace ServerApi;
    auto loginJob = callServer(PasswordLogin(d->username, d->password));
    connect( loginJob, &Call::success, [=]() {
        d->userId = loginJob->result().id;
        emit reconnected();
    });
    connect( loginJob, &Call::failure, [=](Status status) {
        emit loginError(status.message);
        d->isConnected = false;
    });
}

void Connection::disconnectFromServer()
{
    stopSync();
    d->isConnected = false;
}

void Connection::logout()
{
    callServer(ServerApi::Logout())
        ->onSuccess([=]() {
            d->data->setToken({});
            emit loggedOut();
        });
}

void Connection::sync(int timeout)
{
    if (d->syncJob)
        return;

    const QString filter = "{\"room\": { \"timeline\": { \"limit\": 100 } } }";
    auto job = d->syncJob =
            callApi<SyncJob>(d->data->lastEvent(), filter, timeout);
    connect( job, &SyncJob::success, [=] () {
        d->data->setLastEvent(job->nextBatch());
        for( auto& roomData: job->roomData() )
        {
            if ( Room* r = provideRoom(roomData.roomId) )
                r->updateData(roomData);
        }
        d->syncJob = nullptr;
        emit syncDone();
    });
    connect( job, &SyncJob::retryScheduled, this, &Connection::networkError);
    connect( job, &SyncJob::failure, [=] () {
        d->syncJob = nullptr;
        if (job->error() == BaseJob::ContentAccessError)
            emit loginError(job->errorString());
        else
            emit syncError(job->errorString());
    });
}

void Connection::stopSync()
{
    if (d->syncJob)
    {
        d->syncJob->abandon();
        d->syncJob = nullptr;
    }
}

void Connection::postMessage(Room* room, QString type, QString message)
{
    PostMessageJob* job = new PostMessageJob(d->data, room->id(), type, message);
    job->start();
}

JoinRoomJob* Connection::joinRoom(QString roomAlias)
{
    callServer(ServerApi::JoinRoom(roomAlias))
        ->onSuccess([=] (const QString& roomId) {
            if ( Room* r = provideRoom(roomId) )
                emit joinedRoom(r);
        });
}

void Connection::leaveRoom(Room* room)
{
    LeaveRoomJob* job = new LeaveRoomJob(d->data, room);
    job->start();
}

RoomMessagesJob* Connection::getMessages(Room* room, QString from)
{
    RoomMessagesJob* job = new RoomMessagesJob(d->data, room->id(), from);
    job->start();
    return job;
}

QUrl Connection::homeserver() const
{
    return d->data->baseUrl();
}

User* Connection::user(QString userId)
{
    if( d->userMap.contains(userId) )
        return d->userMap.value(userId);
    User* user = createUser(userId);
    d->userMap.insert(userId, user);
    return user;
}

User *Connection::user()
{
    if( d->userId.isEmpty() )
        return nullptr;
    return user(d->userId);
}

QString Connection::userId() const
{
    return d->userId;
}

QString Connection::token() const
{
    return accessToken();
}

QString Connection::accessToken() const
{
    return d->data->accessToken();
}

SyncJob* Connection::syncJob() const
{
    return d->syncJob;
}

int Connection::millisToReconnect() const
{
    return d->syncJob ? d->syncJob->millisToRetry() : 0;
}

QHash< QString, Room* > Connection::roomMap() const
{
    return d->roomMap;
}

bool Connection::isConnected()
{
    return d->isConnected;
}

ConnectionData* Connection::connectionData()
{
    return d->data;
}

Room* Connection::provideRoom(QString id)
{
    if (id.isEmpty())
    {
        qDebug() << "Connection::provideRoom() with empty id, doing nothing";
        return nullptr;
    }

    if (d->roomMap.contains(id))
        return d->roomMap.value(id);

    // Not yet in the map, create a new one.
    Room* room = createRoom(id);
    if (room)
    {
        d->roomMap.insert( id, room );
        emit newRoom(room);
    } else {
        qCritical() << "Failed to create a room!!!" << id;
    }

    return room;
}

User* Connection::createUser(QString userId)
{
    return new User(userId, this);
}

Room* Connection::createRoom(QString roomId)
{
    return new Room(this, roomId);
}
