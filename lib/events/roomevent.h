// SPDX-FileCopyrightText: 2018 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "event.h"

#include <QtCore/QDateTime>

namespace Quotient {
class RedactionEvent;

/** This class corresponds to m.room.* events */
class RoomEvent : public Event {
    Q_GADGET
    Q_PROPERTY(QString id READ id)
    Q_PROPERTY(QDateTime timestamp READ timestamp CONSTANT)
    Q_PROPERTY(QString roomId READ roomId CONSTANT)
    Q_PROPERTY(QString senderId READ senderId CONSTANT)
    Q_PROPERTY(QString redactionReason READ redactionReason)
    Q_PROPERTY(bool isRedacted READ isRedacted)
    Q_PROPERTY(QString transactionId READ transactionId WRITE setTransactionId)
public:
    using factory_t = EventFactory<RoomEvent>;

    // RedactionEvent is an incomplete type here so we cannot inline
    // constructors and destructors and we cannot use 'using'.
    RoomEvent(Type type, event_mtype_t matrixType,
              const QJsonObject& contentJson = {});
    RoomEvent(Type type, const QJsonObject& json);
    ~RoomEvent() override;

    QString id() const;
    QDateTime originTimestamp() const;
    [[deprecated("Use originTimestamp()")]] QDateTime timestamp() const {
        return originTimestamp();
    }
    QString roomId() const;
    QString senderId() const;
    //! \brief Determine whether the event has been replaced
    //!
    //! \return true if this event has been overridden by another event
    //!         with `"rel_type": "m.replace"`; false otherwise
    bool isReplaced() const;
    QString replacedBy() const;
    bool isRedacted() const { return bool(_redactedBecause); }
    const event_ptr_tt<RedactionEvent>& redactedBecause() const
    {
        return _redactedBecause;
    }
    QString redactionReason() const;
    QString transactionId() const;
    QString stateKey() const;

    void setRoomId(const QString& roomId);
    void setSender(const QString& senderId);

    /**
     * Sets the transaction id for locally created events. This should be
     * done before the event is exposed to any code using the respective
     * Q_PROPERTY.
     *
     * \param txnId - transaction id, normally obtained from
     * Connection::generateTxnId()
     */
    void setTransactionId(const QString& txnId);

    /**
     * Sets event id for locally created events
     *
     * When a new event is created locally, it has no server id yet.
     * This function allows to add the id once the confirmation from
     * the server is received. There should be no id set previously
     * in the event. It's the responsibility of the code calling addId()
     * to notify clients that use Q_PROPERTY(id) about its change
     */
    void addId(const QString& newId);

protected:
    void dumpTo(QDebug dbg) const override;

private:
    event_ptr_tt<RedactionEvent> _redactedBecause;
};
using RoomEventPtr = event_ptr_tt<RoomEvent>;
using RoomEvents = EventsArray<RoomEvent>;
using RoomEventsRange = Range<RoomEvents>;

class CallEventBase : public RoomEvent {
public:
    CallEventBase(Type type, event_mtype_t matrixType, const QString& callId,
                  int version, const QJsonObject& contentJson = {});
    CallEventBase(Type type, const QJsonObject& json);
    ~CallEventBase() override = default;
    bool isCallEvent() const override { return true; }

    QString callId() const { return content<QString>("call_id"_ls); }
    int version() const { return content<int>("version"_ls); }
};
} // namespace Quotient
Q_DECLARE_METATYPE(Quotient::RoomEvent*)
Q_DECLARE_METATYPE(const Quotient::RoomEvent*)
