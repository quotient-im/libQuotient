// SPDX-FileCopyrightText: 2018 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "event.h"

#include <QtCore/QDateTime>

namespace Quotient {
class RedactionEvent;

/** This class corresponds to m.room.* events */
class QUOTIENT_API RoomEvent : public Event {
public:
    static inline EventFactory<RoomEvent> factory { "RoomEvent" };

    // RedactionEvent is an incomplete type here so we cannot inline
    // constructors and destructors and we cannot use 'using'.
    RoomEvent(Type type, event_mtype_t matrixType,
              const QJsonObject& contentJson = {});
    RoomEvent(Type type, const QJsonObject& json);
    ~RoomEvent() override;

    QString id() const;
    QDateTime originTimestamp() const;
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

    //! \brief Fill the pending event object with the room id
    void setRoomId(const QString& roomId);
    //! \brief Fill the pending event object with the sender id
    void setSender(const QString& senderId);
    //! \brief Fill the pending event object with the transaction id
    //! \param txnId - transaction id, normally obtained from
    //!        Connection::generateTxnId()
    void setTransactionId(const QString& txnId);

    //! \brief Add an event id to locally created events after they are sent
    //!
    //! When a new event is created locally, it has no id; the homeserver
    //! assigns it once the event is sent. This function allows to add the id
    //! once the confirmation from the server is received. There should be no id
    //! set previously in the event. It's the responsibility of the code calling
    //! addId() to notify clients about the change; there's no signal or
    //! callback for that in RoomEvent.
    void addId(const QString& newId);

#ifdef Quotient_E2EE_ENABLED
    void setOriginalEvent(event_ptr_tt<RoomEvent>&& originalEvent);
    const RoomEvent* originalEvent() { return _originalEvent.get(); }
    const QJsonObject encryptedJson() const;
#endif

protected:
    void dumpTo(QDebug dbg) const override;

private:
    event_ptr_tt<RedactionEvent> _redactedBecause;

#ifdef Quotient_E2EE_ENABLED
    event_ptr_tt<RoomEvent> _originalEvent;
#endif
};
using RoomEventPtr = event_ptr_tt<RoomEvent>;
using RoomEvents = EventsArray<RoomEvent>;
using RoomEventsRange = Range<RoomEvents>;

class QUOTIENT_API CallEventBase : public RoomEvent {
public:
    CallEventBase(Type type, event_mtype_t matrixType, const QString& callId,
                  int version, const QJsonObject& contentJson = {});
    CallEventBase(Type type, const QJsonObject& json);
    ~CallEventBase() override = default;
    bool isCallEvent() const override { return true; }

    QString callId() const { return contentPart<QString>("call_id"_ls); }
    int version() const { return contentPart<int>("version"_ls); }
};
} // namespace Quotient
Q_DECLARE_METATYPE(Quotient::RoomEvent*)
Q_DECLARE_METATYPE(const Quotient::RoomEvent*)
