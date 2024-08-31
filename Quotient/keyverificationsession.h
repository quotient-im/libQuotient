// SPDX-FileCopyrightText: 2022 Tobias Fella <fella@posteo.de>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "events/keyverificationevent.h"
#include "events/roommessageevent.h"

#include <QtCore/QObject>
#include <QtCore/QPointer>

#include <vodozemac.h>

struct OlmSAS;

namespace Quotient {
class Connection;
class Room;

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

    Q_PROPERTY(QString remoteDeviceId MEMBER m_remoteDeviceId CONSTANT)
    Q_PROPERTY(QString remoteUserId MEMBER m_remoteUserId CONSTANT)
    Q_PROPERTY(QVector<EmojiEntry> sasEmojis READ sasEmojis NOTIFY sasEmojisChanged)
    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    Q_PROPERTY(Error error READ error NOTIFY errorChanged)
    // Whether this is a user verification (in contrast to a device verification)
    Q_PROPERTY(bool userVerification READ userVerification CONSTANT)

    // Incoming device verification
    KeyVerificationSession(QString remoteUserId,
                           const KeyVerificationRequestEvent& event,
                           Connection* connection, bool encrypted);

    // Outgoing device verification
    KeyVerificationSession(QString userId, QString deviceId,
                           Connection* connection);

    // Incoming user verification
    KeyVerificationSession(const RoomMessageEvent *event, Room *room);

    // Outgoing user verification
    explicit KeyVerificationSession(Room *room);

    void handleEvent(const KeyVerificationEvent& baseEvent);

    QVector<EmojiEntry> sasEmojis() const;
    State state() const;

    Error error() const;

    QString remoteDeviceId() const;
    QString transactionId() const;
    bool userVerification() const;

    void setRequestEventId(const QString &eventId);

public Q_SLOTS:
    void sendRequest();
    void sendReady();
    void sendMac();
    void sendStartSas();
    void sendKey();
    void sendDone();
    void cancelVerification(Error error);

Q_SIGNALS:
    void keyReceived();
    void sasEmojisChanged();
    void stateChanged();
    void errorChanged();
    void finished();

private:
    // Internal delegating constructors

    KeyVerificationSession(QString remoteUserId, Connection* connection, QString remoteDeviceId,
                           bool encrypted, QStringList methods, QDateTime startTimestamp,
                           QString transactionId, Room* room = nullptr, QString requestEventId = {});
    KeyVerificationSession(QString remoteUserId, Connection* connection, Room* room,
                           QString remoteDeviceId = {}, QString transactionId = {});

    Connection* const m_connection;
    QPointer<Room> m_room;
    const QString m_remoteUserId;
    QString m_remoteDeviceId;
    QString m_transactionId;
    bool m_encrypted = false;
    QStringList m_remoteSupportedMethods{};
    QStringList m_commonMacCodes{};
    rust::Box<sas::Sas> m_sas;
    std::optional<rust::Box<sas::EstablishedSas>> m_establishedSas;

    QVector<EmojiEntry> m_sasEmojis;
    bool startSentByUs = false;
    State m_state = INCOMING;
    Error m_error = NONE;
    QString m_startEvent{};
    QString m_commitment{};
    bool macReceived = false;
    bool m_verified = false;
    QString m_pendingEdKeyId{};
    QString m_pendingMasterKey{};
    QString m_requestEventId{};

    static CStructPtr<OlmSAS> makeOlmData();
    void handleReady(const KeyVerificationReadyEvent& event);
    void handleStart(const KeyVerificationStartEvent& event);
    void handleKey(const KeyVerificationKeyEvent& event);
    void handleMac(const KeyVerificationMacEvent& event);
    void setupTimeout(std::chrono::milliseconds timeout);
    void setState(State state);
    void setError(Error error);
    static QString errorToString(Error error);
    static Error stringToError(const QString& error);
    void trustKeys();
    void sendEvent(const QString &userId, const QString &deviceId, const KeyVerificationEvent &event, bool encrypted);

    QByteArray macInfo(bool verifying, const QString& key = "KEY_IDS"_ls);
    QString calculateMac(const QString& input, bool verifying, const QString& keyId = "KEY_IDS"_ls);
};

} // namespace Quotient
