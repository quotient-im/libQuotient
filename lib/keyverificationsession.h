// SPDX-FileCopyrightText: 2022 Tobias Fella <fella@posteo.de>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "events/keyverificationevent.h"

#include <QtCore/QObject>

struct OlmSAS;

namespace Quotient {
class Connection;

struct QUOTIENT_API EmojiEntry {
    QString emoji;
    QString description;

    Q_GADGET
    Q_PROPERTY(QString emoji MEMBER emoji CONSTANT)
    Q_PROPERTY(QString description MEMBER description CONSTANT)

public:
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
        INCOMING, ///< There is a request for verification incoming
        //! We sent a request for verification and are waiting for ready
        WAITINGFORREADY,
        //! Either party sent a ready as a response to a request; the user
        //! selects a method
        READY,
        WAITINGFORACCEPT, ///< We sent a start and are waiting for an accept
        ACCEPTED, ///< The other party sent an accept and is waiting for a key
        WAITINGFORKEY, ///< We're waiting for a key
        //! We're waiting for the *user* to verify the emojis
        WAITINGFORVERIFICATION,
        WAITINGFORMAC, ///< We're waiting for the mac
        CANCELED, ///< The session has been canceled
        DONE, ///< The verification is done
    };
    Q_ENUM(State)

    enum Error {
        NONE,
        TIMEOUT,
        REMOTE_TIMEOUT,
        USER,
        REMOTE_USER,
        UNEXPECTED_MESSAGE,
        REMOTE_UNEXPECTED_MESSAGE,
        UNKNOWN_TRANSACTION,
        REMOTE_UNKNOWN_TRANSACTION,
        UNKNOWN_METHOD,
        REMOTE_UNKNOWN_METHOD,
        KEY_MISMATCH,
        REMOTE_KEY_MISMATCH,
        USER_MISMATCH,
        REMOTE_USER_MISMATCH,
        INVALID_MESSAGE,
        REMOTE_INVALID_MESSAGE,
        SESSION_ACCEPTED,
        REMOTE_SESSION_ACCEPTED,
        MISMATCHED_COMMITMENT,
        REMOTE_MISMATCHED_COMMITMENT,
        MISMATCHED_SAS,
        REMOTE_MISMATCHED_SAS,
    };
    Q_ENUM(Error)

    Q_PROPERTY(QString remoteDeviceId READ remoteDeviceId CONSTANT)
    Q_PROPERTY(QVector<EmojiEntry> sasEmojis READ sasEmojis NOTIFY sasEmojisChanged)
    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    Q_PROPERTY(Error error READ error NOTIFY stateChanged)

    KeyVerificationSession(QString remoteUserId,
                           const KeyVerificationRequestEvent& event,
                           Connection* connection, bool encrypted);
    KeyVerificationSession(QString userId, QString deviceId,
                           Connection* connection);

    void handleEvent(const KeyVerificationEvent& baseEvent);

    QString remoteDeviceId() const;
    QString transactionId() const;
    QVector<EmojiEntry> sasEmojis() const;
    State state() const;
    Error error() const;

public Q_SLOTS:
    void setReady();
    void sasVerified();
    void cancelVerification(Error error);

Q_SIGNALS:
    void keyReceived();
    void sasEmojisChanged();
    void stateChanged();
    void finished();

private:
    class Private;
    ImplPtr<Private> d;
};

} // namespace Quotient
Q_DECLARE_METATYPE(Quotient::EmojiEntry)
