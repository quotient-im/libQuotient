// SPDX-FileCopyrightText: 2017 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "stateevent.h"
#include "single_key_value.h"

namespace Quotient {
#define DEFINE_SIMPLE_STATE_EVENT(_Name, _TypeId, _ValueType, _ContentKey) \
    constexpr auto _Name##Key = #_ContentKey##_ls;                         \
    class QUOTIENT_API _Name                                               \
        : public StateEvent<                                               \
              EventContent::SingleKeyValue<_ValueType, &_Name##Key>> {     \
    public:                                                                \
        using value_type = _ValueType;                                     \
        QUO_EVENT(_Name, _TypeId)                                \
        template <typename T>                                              \
        explicit _Name(T&& value)                                          \
            : StateEvent(TypeId, matrixTypeId(), QString(),                \
                         std::forward<T>(value))                           \
        {}                                                                 \
        explicit _Name(QJsonObject obj)                                    \
            : StateEvent(TypeId, std::move(obj))                           \
        {}                                                                 \
        auto _ContentKey() const { return content().value; }               \
    };                                                                     \
    // End of macro

DEFINE_SIMPLE_STATE_EVENT(RoomNameEvent, "m.room.name", QString, name)
DEFINE_SIMPLE_STATE_EVENT(RoomTopicEvent, "m.room.topic", QString, topic)
DEFINE_SIMPLE_STATE_EVENT(RoomPinnedEvent, "m.room.pinned_messages",
                          QStringList, pinnedEvents)

constexpr auto RoomAliasesEventKey = "aliases"_ls;
class QUOTIENT_API RoomAliasesEvent
    : public StateEvent<
          EventContent::SingleKeyValue<QStringList, &RoomAliasesEventKey>> {
public:
    QUO_EVENT(RoomAliasesEvent, "m.room.aliases")
    explicit RoomAliasesEvent(const QJsonObject& obj)
        : StateEvent(typeId(), obj)
    {}
    Q_DECL_DEPRECATED_X(
        "m.room.aliases events are deprecated by the Matrix spec; use"
        " RoomCanonicalAliasEvent::altAliases() to get non-authoritative aliases")
    QString server() const { return stateKey(); }
    Q_DECL_DEPRECATED_X(
        "m.room.aliases events are deprecated by the Matrix spec; use"
        " RoomCanonicalAliasEvent::altAliases() to get non-authoritative aliases")
    QStringList aliases() const { return content().value; }
};
} // namespace Quotient
