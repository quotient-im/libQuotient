// SPDX-FileCopyrightText: 2022 Tobias Fella <fella@posteo.de>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "keyverificationsession.h"

#include <QTimer>

#include "connection.h"
#include "connection_p.h"
#include "room.h"

using namespace Quotient;

//TODO: separate constructors
KeyVerificationSession::KeyVerificationSession(const QString& remoteUserId, const QString& verificationId, const QString& remoteDeviceId, Connection* connection)
    : QObject(connection)
    , m_remoteUserId(remoteUserId)
    , m_verificationId(verificationId)
    , m_remoteDeviceId(remoteDeviceId)
    , m_connection(connection)
    , m_processTimer(new QTimer)
{
    connect(connection, &Connection::syncDone, this, [this](){
        setState(m_connection->d->keyVerificationSessionState(this));
        setSasState(m_connection->d->sasState(this));
    });
    if (verificationId.isEmpty()) {
        setState(CREATED);
        m_connection->d->requestDeviceVerification(this);
    }
    m_processTimer->setInterval(250);
    m_processTimer->setSingleShot(false);
    m_processTimer->start();
    connect(m_processTimer, &QTimer::timeout, this, [this](){
        m_connection->d->processOutgoingRequests();
    });
    //TODO make sure that objects are deleted when finished
}

KeyVerificationSession::KeyVerificationSession(Room* room, Connection* connection, const QString& verificationId)
    : QObject(connection)
    , m_verificationId(verificationId)
    , m_connection(connection)
    , m_room(room)
    , m_processTimer(new QTimer)
{
    const auto& ids = room->joinedMemberIds();
    if (ids.size() != 2) {
        //TODO: error
        return;
    }
    m_remoteUserId = ids[0] == connection->userId() ? ids[1] : ids[0];

    connect(connection, &Connection::syncDone, this, [this](){
        setState(m_connection->d->keyVerificationSessionState(this));
        setSasState(m_connection->d->sasState(this));
    });

    connect(connection, &Connection::verificationEventProcessed, this, [this](){
        setState(m_connection->d->keyVerificationSessionState(this));
        setSasState(m_connection->d->sasState(this));
    });

    if (m_verificationId.isEmpty()) { //TODO this check seems pointless
        setState(CREATED);
        m_connection->d->requestUserVerification(this);
    } else {
        setState(REQUESTED);
    }
    m_processTimer->setInterval(250);
    m_processTimer->setSingleShot(false);
    m_processTimer->start();
    connect(m_processTimer, &QTimer::timeout, this, [this](){
        m_connection->d->processOutgoingRequests();
    });
}

KeyVerificationSession* KeyVerificationSession::requestDeviceVerification(const QString& userId, const QString& deviceId, Connection* connection)
{
    return new KeyVerificationSession(userId, {}, deviceId, connection);
}

KeyVerificationSession* KeyVerificationSession::requestUserVerification(Room* room, Connection* connection)
{
    return new KeyVerificationSession(room, connection);
}


void KeyVerificationSession::accept()
{
    m_connection->d->acceptKeyVerification(this);
}

void KeyVerificationSession::confirm()
{
    m_connection->d->confirmKeyVerification(this);
}

void KeyVerificationSession::setState(State state)
{
    if (m_state == state) {
        return;
    }

    if (state == NOTFOUND) {
        return;
    }


    m_state = state;
    Q_EMIT stateChanged();

    qWarning() << "Verification state" << m_state;
}

void KeyVerificationSession::setSasState(SasState state)
{
    if (m_sasState == state || state == SASNOTFOUND) {
        return;
    }

    m_sasState = state;
    Q_EMIT sasStateChanged();

    if (m_sasState == KEYSEXCHANGED) { //TODO move to sasstate
        emit sasEmojisChanged();
    }
    qWarning() << "Sas state" << m_sasState;

    // TODO: only accept if the verification started from a request, otherwise we're just auto-accepting things.
    // I'm not sure if unexpected m.key.verification.start's will even work, though. And i honestly don't care.
    if (m_sasState == STARTED && !weStarted) {
        qWarning() << "accepting";
        m_connection->d->acceptSas(this);
    }
}

QVector<EmojiEntry> KeyVerificationSession::sasEmojis()
{
    auto raw = m_connection->d->keyVerificationSasEmoji(this);

    QVector<EmojiEntry> out;
    for (const auto& [symbol, description] : raw) {
        out += {
            symbol, description
        };
    }
    return out;
}

QString KeyVerificationSession::remoteDevice() const
{
    return m_remoteDeviceId;
}

void KeyVerificationSession::setVerificationId(const QString& verificationId)
{
    m_verificationId = verificationId;
}

QString KeyVerificationSession::remoteUser() const
{
    return m_remoteUserId;
}

QString KeyVerificationSession::verificationId() const
{
    return m_verificationId;
}

Room* KeyVerificationSession::room() const
{
    return m_room;
}

KeyVerificationSession* KeyVerificationSession::processIncomingUserVerification(Room* room, const QString& eventId)
{
    return new KeyVerificationSession(room, room->connection(), eventId);
}

void KeyVerificationSession::startSas()
{
    if (m_state != READY) {
        return;
    }
    //TODO: resolve glare
    weStarted = true;
    m_connection->d->startKeyVerification(this);
}
