/******************************************************************************
 * Copyright (C) 2018 Kitsune Ral <kitsune-ral@users.sf.net>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

#pragma once

#include "events/stateevent.h"

#include <utility>

namespace Quotient {
class StateEventBase;

class EventStatus {
    Q_GADGET
public:
    /** Special marks an event can assume
     *
     * This is used to hint at a special status of some events in UI.
     * All values except Redacted and Hidden are mutually exclusive.
     */
    enum Code {
        Normal = 0x0, //< No special designation

        /// The event is not queued for sending yet; either it's just
        /// created or pending additional preparation, e.g. during file upload
        Pending = 0x01,
        ReadyToDepart = 0x02, //< The event is waiting in the queue to depart
        FileUploaded = ReadyToDepart, //< Back-comp synonym for ReadyToDepart
        Departing = 0x03, //< The event is being sent right now
        Departed = 0x04, //< The event has left the client
        ReachedServer = 0x05, //< The server has received the event
        SendingFailed = 0x06, //< The server could not receive the event
        Redacted = 0x08, //< The event has been redacted
        Hidden = 0x10, //< The event should not be shown in the timeline
    };
    Q_DECLARE_FLAGS(Status, Code)
    Q_FLAG(Status)
};

class EventItemBase {
public:
    explicit EventItemBase(RoomEventPtr&& e) : evt(std::move(e))
    {
        Q_ASSERT(evt);
    }

    const RoomEvent* event() const { return rawPtr(evt); }
    const RoomEvent* get() const { return event(); }
    template <typename EventT>
    const EventT* viewAs() const
    {
        return eventCast<const EventT>(evt);
    }
    const RoomEventPtr& operator->() const { return evt; }
    const RoomEvent& operator*() const { return *evt; }

    // Used for event redaction
    RoomEventPtr replaceEvent(RoomEventPtr&& other)
    {
        return std::exchange(evt, move(other));
    }

protected:
    template <typename EventT>
    EventT* getAs()
    {
        return eventCast<EventT>(evt);
    }

private:
    RoomEventPtr evt;
};

class TimelineItem : public EventItemBase {
public:
    // For compatibility with Qt containers, even though we use
    // a std:: container now for the room timeline
    using index_t = int;

    TimelineItem(RoomEventPtr&& e, index_t number)
        : EventItemBase(std::move(e)), idx(number)
    {}

    index_t index() const { return idx; }

private:
    index_t idx;
};

template <>
inline const StateEventBase* EventItemBase::viewAs<StateEventBase>() const
{
    return evt->isStateEvent() ? weakPtrCast<const StateEventBase>(evt)
                               : nullptr;
}

template <>
inline const CallEventBase* EventItemBase::viewAs<CallEventBase>() const
{
    return evt->isCallEvent() ? weakPtrCast<const CallEventBase>(evt) : nullptr;
}

class PendingEventItem : public EventItemBase {
    Q_GADGET
public:
    using EventItemBase::EventItemBase;
    PendingEventItem(RoomEventPtr&& e, EventStatus::Code initialStatus)
        : EventItemBase(std::move(e)), _status(initialStatus)
    {}

    EventStatus::Code deliveryStatus() const { return _status; }
    bool isInFlight() const
    {
        return _status == EventStatus::Departing
               || _status == EventStatus::Departed;
    }
    QDateTime lastUpdated() const { return _lastUpdated; }
    QString annotation() const { return _annotation; }

    void setStatus(EventStatus::Code status, const QString& annotation = {})
    {
        _status = status;
        _lastUpdated = QDateTime::currentDateTimeUtc();
        _annotation = annotation;
    }
    void updateUploadedFile(const QUrl& remoteUrl);

private:
    EventStatus::Code _status = EventStatus::ReadyToDepart;
    QDateTime _lastUpdated = QDateTime::currentDateTimeUtc();
    QString _annotation;
};

inline QDebug& operator<<(QDebug& d, const TimelineItem& ti)
{
    QDebugStateSaver dss(d);
    d.nospace() << "(" << ti.index() << "|" << ti->id() << ")";
    return d;
}
} // namespace Quotient
Q_DECLARE_METATYPE(Quotient::EventStatus)
