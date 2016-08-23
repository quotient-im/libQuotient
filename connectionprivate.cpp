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

#include "connectionprivate.h"
#include "connection.h"
#include "state.h"
#include "room.h"
#include "user.h"
#include "jobs/passwordlogin.h"
#include "jobs/syncjob.h"
#include "jobs/joinroomjob.h"
#include "jobs/roommembersjob.h"
#include "events/event.h"
#include "events/roommessageevent.h"
#include "events/roommemberevent.h"

#include <QtCore/QDebug>
#include <QtNetwork/QDnsLookup>

using namespace QMatrixClient;

ConnectionPrivate::ConnectionPrivate(Connection* parent)
    : q(parent)
{
    isConnected = false;
    data = nullptr;
}

ConnectionPrivate::~ConnectionPrivate()
{
    delete data;
}

void ConnectionPrivate::resolveServer(QString domain)
{
    // Find the Matrix server for the given domain.
    QDnsLookup* dns = new QDnsLookup();
    dns->setType(QDnsLookup::SRV);
    dns->setName("_matrix._tcp." + domain);

    connect(dns, &QDnsLookup::finished, [this,dns]() {
        // Check the lookup succeeded.
        if (dns->error() != QDnsLookup::NoError ||
                dns->serviceRecords().isEmpty()) {
            emit q->resolveError("DNS lookup failed");
            dns->deleteLater();
            return;
        }

        // Handle the results.
        QDnsServiceRecord record = dns->serviceRecords().first();
        data->setHost(record.target());
        data->setPort(record.port());
        emit q->resolved();
        dns->deleteLater();
    });
    dns->lookup();
}

void ConnectionPrivate::processState(State* state)
{
    if( state->event()->type() == QMatrixClient::EventType::RoomMember )
    {
        QMatrixClient::RoomMemberEvent* e = static_cast<QMatrixClient::RoomMemberEvent*>(state->event());
        User* user = q->user(e->userId());
        user->processEvent(e);
    }

    if ( Room* r = provideRoom(state->event()->roomId()) )
        r->addInitialState(state);
}

Room* ConnectionPrivate::provideRoom(QString id)
{
    if (id.isEmpty())
    {
        qDebug() << "ConnectionPrivate::provideRoom() with empty id, doing nothing";
        return nullptr;
    }

    if (roomMap.contains(id))
        return roomMap.value(id);

    // Not yet in the map, create a new one.
    Room* room = q->createRoom(id);
    if (!room)
        qCritical() << "Failed to create a room!!!" << id;

    roomMap.insert( id, room );
    emit q->newRoom(room);
    return room;
}

void ConnectionPrivate::gotRoomMembers(BaseJob* job)
{
    RoomMembersJob* membersJob = static_cast<RoomMembersJob*>(job);
    if( !membersJob->error() )
    {
        for( State* state: membersJob->states() )
        {
            processState(state);
        }
        qDebug() << membersJob->states().count() << " processed...";
    }
    else
    {
        qDebug() << "MembersJob error: " <<membersJob->errorString();
        if( membersJob->error() == BaseJob::NetworkError )
            emit q->connectionError( membersJob->errorString() );
    }
}
