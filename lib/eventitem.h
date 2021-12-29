// SPDX-FileCopyrightText: 2018 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "events/stateevent.h"
#include "quotient_common.h"

#include <any>
#include <utility>

namespace Quotient {

namespace EventStatus {
    QUO_NAMESPACE

    /** Special marks an event can assume
     *
     * This is used to hint at a special status of some events in UI.
     * All values except Redacted and Hidden are mutually exclusive.
     */
    enum Code {
        Normal = 0x0, //< No special designation
        Submitted = 0x01, //< The event has just been submitted for sending
        FileUploaded = 0x02, //< The file attached to the event has been
                             // uploaded to the server
        Departed = 0x03, //< The event has left the client
        ReachedServer = 0x04, //< The server has received the event
        SendingFailed = 0x05, //< The server could not receive the event
        Redacted = 0x08, //< The event has been redacted
        Replaced = 0x10, //< The event has been replaced
        Hidden = 0x100, //< The event should not be shown in the timeline
    };
    Q_ENUM_NS(Code)
} // namespace EventStatus

class QUOTIENT_API EventItemBase {
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

    /// Store arbitrary data with the event item
    void setUserData(std::any userData) { data = userData; }
    /// Obtain custom data previously stored with the event item
    const std::any& userdata() const { return data; }
    std::any& userData() { return data; }

protected:
    template <typename EventT>
    EventT* getAs()
    {
        return eventCast<EventT>(evt);
    }

private:
    RoomEventPtr evt;
    std::any data;
};

class QUOTIENT_API TimelineItem : public EventItemBase {
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

class QUOTIENT_API PendingEventItem : public EventItemBase {
public:
    using EventItemBase::EventItemBase;

    EventStatus::Code deliveryStatus() const { return _status; }
    QDateTime lastUpdated() const { return _lastUpdated; }
    QString annotation() const { return _annotation; }

    void setDeparted() { setStatus(EventStatus::Departed); }
    void setFileUploaded(const QUrl& remoteUrl);
    void setReachedServer(const QString& eventId)
    {
        setStatus(EventStatus::ReachedServer);
        (*this)->addId(eventId);
    }
    void setSendingFailed(QString errorText)
    {
        setStatus(EventStatus::SendingFailed);
        _annotation = std::move(errorText);
    }
    void resetStatus() { setStatus(EventStatus::Submitted); }

private:
    EventStatus::Code _status = EventStatus::Submitted;
    QDateTime _lastUpdated = QDateTime::currentDateTimeUtc();
    QString _annotation;

    void setStatus(EventStatus::Code status)
    {
        _status = status;
        _lastUpdated = QDateTime::currentDateTimeUtc();
        _annotation.clear();
    }
};

inline QDebug& operator<<(QDebug& d, const TimelineItem& ti)
{
    QDebugStateSaver dss(d);
    d.nospace() << "(" << ti.index() << "|" << ti->id() << ")";
    return d;
}
} // namespace Quotient
