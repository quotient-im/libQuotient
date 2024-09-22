// SPDX-FileCopyrightText: 2022 Tobias Fella <fella@posteo.de>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "quotient_export.h"

#include <QtCore/QObject>
#include <QtCore/QPointer>

class QTimer;

namespace Quotient {
class Connection;
class Room;

struct QUOTIENT_API EmojiEntry {
    Q_GADGET
    //! \brief Unicode of the emoji
    Q_PROPERTY(QString emoji MEMBER emoji CONSTANT)
    //! \brief Textual description of the emoji
    //! This follows https://spec.matrix.org/v1.11/client-server-api/#sas-method-emoji
    Q_PROPERTY(QString description MEMBER description CONSTANT)

public:
    QString emoji;
    QString description;
    friend bool operator==(const EmojiEntry&, const EmojiEntry&) = default;
};

/** A key verification session. Listen for incoming sessions by connecting to Connection::newKeyVerificationSession.
    Start a new session using Connection::startKeyVerificationSession.
    The object is delete after finished is emitted.
*/
class QUOTIENT_API KeyVerificationSession : public QObject
{
    Q_OBJECT
public:
    enum State {
        CREATED, //! The verification request has been newly created by us.
        REQUESTED, //! The verification request was received from the other party.
        READY, //! The verification request is ready to start a verification flow.
        TRANSITIONED, //! The verification request has transitioned into a concrete verification flow. For example it transitioned into the emoji based SAS verification.
        DONE, //! The verification flow that was started with this request has finished.
        CANCELLED, //! The verification process has been cancelled.
        NOTFOUND,
    };
    Q_ENUM(State)

    enum SasState {
        STARTED, //! The verification has been started, the protocols that should be used have been proposed and can be accepted.
        ACCEPTED, //! The verification has been accepted and both sides agreed to a set of protocols that will be used for the verification process.
        KEYSEXCHANGED, //! The public keys have been exchanged and the short auth string can be presented to the user.
        CONFIRMED, //! The verification process has been confirmed from our side, we’re waiting for the other side to confirm as well.
        SASDONE, //! The verification process has been successfully concluded.
        SASCANCELLED, //! The verification process has been cancelled.
        SASNOTFOUND,
    };
    Q_ENUM(SasState)

    //! \brief The matrix id of the user we're verifying.
    //! For device verification this is our own id.
    Q_PROPERTY(QString remoteUserId MEMBER m_remoteUserId CONSTANT)

    //! \brief The device id of the device we're verifying
    //! Does not have a specified value when verifying a different user
    Q_PROPERTY(QString remoteDeviceId MEMBER m_remoteDeviceId CONSTANT)

    //! \brief The current state of the verification session
    //! Clients should use this to adapt their UI to the current stage of the verification
    Q_PROPERTY(State state MEMBER m_state NOTIFY stateChanged)

    //! \brief The current state of the sas verification
    //! Clients should use this to adapt their UI to the current stage of the verification
    Q_PROPERTY(SasState sasState MEMBER m_sasState NOTIFY sasStateChanged)

    //! \brief The sas emoji that should be shown to the user.
    //! Only has a specified value when the session is in TRANSITIONED state
    Q_PROPERTY(QVector<EmojiEntry> sasEmojis READ sasEmojis NOTIFY sasEmojisChanged)

    KeyVerificationSession(const QString& remoteUserId, const QString& verificationId, const QString& remoteDeviceId, Quotient::Connection* connection);
    KeyVerificationSession(Room* room, Quotient::Connection* connection, const QString& verificationId = {});

    //! \brief Accept an incoming verification session
    Q_INVOKABLE void accept();

    //! \brief Confirm that the emojis shown to the user match
    //! This will mark the remote session / user as verified and send an m.key.verification.mac event
    //! Only call this after the user has confirmed the correctness of the emoji!
    Q_INVOKABLE void confirm();

    //! \brief Start a SAS verification
    Q_INVOKABLE void startSas();

    QVector<EmojiEntry> sasEmojis();
    QString remoteUser() const;
    QString remoteDevice() const;
    QString verificationId() const;

    static KeyVerificationSession* requestDeviceVerification(const QString& userId, const QString& deviceId, Connection* connection);
    static KeyVerificationSession* requestUserVerification(Room* room, Connection* connection);

    static KeyVerificationSession* processIncomingUserVerification(Room* room, const QString& eventId);

    Quotient::Room* room() const;

private:
    QString m_remoteUserId;
    QString m_verificationId;
    QString m_remoteDeviceId;
    QPointer<Quotient::Connection> m_connection;
    QPointer<Quotient::Room> m_room;
    State m_state = REQUESTED;
    SasState m_sasState = SASNOTFOUND;
    bool weStarted = false;
    QTimer *m_processTimer = nullptr;

    void setState(State state);
    void setSasState(SasState state);
    void setVerificationId(const QString& verificationId);

    friend class Quotient::Connection;
Q_SIGNALS:
    void stateChanged();
    void sasStateChanged();
    void sasEmojisChanged();
};

} // namespace Quotient
Q_DECLARE_METATYPE(Quotient::EmojiEntry)
