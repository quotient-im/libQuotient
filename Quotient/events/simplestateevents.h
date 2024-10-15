// SPDX-FileCopyrightText: 2017 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-FileCopyrightText: 2024 James Graham <james.h.graham@protonmail.com>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "stateevent.h"
#include "single_key_value.h"

namespace Quotient {

#define DEFINE_SIMPLE_STATE_EVENT(Name_, TypeId_, ValueType_, GetterName_, JsonKey_)              \
    constexpr inline auto Name_##Key = JsonKey_##_L1;                                             \
    class QUOTIENT_API Name_ : public ::Quotient::KeylessStateEventBase<                          \
                                   Name_, EventContent::SingleKeyValue<ValueType_, Name_##Key>> { \
    public:                                                                                       \
        using value_type = ValueType_;                                                            \
        QUO_EVENT(Name_, TypeId_)                                                                 \
        using KeylessStateEventBase::KeylessStateEventBase;                                       \
        auto GetterName_() const { return content().value; }                                      \
    };                                                                                            \
    // End of macro

DEFINE_SIMPLE_STATE_EVENT(RoomNameEvent, "m.room.name", QString, name, "name")
DEFINE_SIMPLE_STATE_EVENT(RoomTopicEvent, "m.room.topic", QString, topic,
                          "topic")
DEFINE_SIMPLE_STATE_EVENT(RoomPinnedEventsEvent, "m.room.pinned_events",
                          QStringList, pinnedEvents, "pinned")

class QUOTIENT_API RoomServerAclEvent : public StateEvent {
public:
    QUO_EVENT(RoomServerAclEvent, "m.room.server_acl")

    using StateEvent::StateEvent;

    QUO_CONTENT_GETTER(QStringList, allow)
    QUO_CONTENT_GETTER(bool, allowIpLiterals)
    QUO_CONTENT_GETTER(QStringList, deny)
};
} // namespace Quotient
