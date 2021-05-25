// SPDX-FileCopyrightText: 2017 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "stateevent.h"

namespace Quotient {
namespace EventContent {
    template <typename T>
    class SimpleContent {
    public:
        using value_type = T;

        // The constructor is templated to enable perfect forwarding
        template <typename TT>
        SimpleContent(QString keyName, TT&& value)
            : value(std::forward<TT>(value)), key(std::move(keyName))
        {}
        SimpleContent(const QJsonObject& json, QString keyName)
            : value(fromJson<T>(json[keyName])), key(std::move(keyName))
        {}
        QJsonObject toJson() const
        {
            return { { key, Quotient::toJson(value) } };
        }

    public:
        T value;

    protected:
        QString key;
    };
} // namespace EventContent

#define DEFINE_SIMPLE_STATE_EVENT(_Name, _TypeId, _ValueType, _ContentKey)     \
    class _Name : public StateEvent<EventContent::SimpleContent<_ValueType>> { \
    public:                                                                    \
        using value_type = content_type::value_type;                           \
        DEFINE_EVENT_TYPEID(_TypeId, _Name)                                    \
        explicit _Name() : _Name(value_type()) {}                              \
        template <typename T>                                                  \
        explicit _Name(T&& value)                                              \
            : StateEvent(typeId(), matrixTypeId(), QString(),                  \
                         QStringLiteral(#_ContentKey), std::forward<T>(value)) \
        {}                                                                     \
        explicit _Name(QJsonObject obj)                                        \
            : StateEvent(typeId(), std::move(obj),                             \
                         QStringLiteral(#_ContentKey))                         \
        {}                                                                     \
        auto _ContentKey() const { return content().value; }                   \
    };                                                                         \
    REGISTER_EVENT_TYPE(_Name)                                                 \
    // End of macro

DEFINE_SIMPLE_STATE_EVENT(RoomNameEvent, "m.room.name", QString, name)
DEFINE_SIMPLE_STATE_EVENT(RoomTopicEvent, "m.room.topic", QString, topic)
DEFINE_SIMPLE_STATE_EVENT(RoomPinnedEvent, "m.room.pinned_messages", QStringList, pinnedEvents)

class RoomAliasesEvent
    : public StateEvent<EventContent::SimpleContent<QStringList>> {
public:
    DEFINE_EVENT_TYPEID("m.room.aliases", RoomAliasesEvent)
    explicit RoomAliasesEvent(const QJsonObject& obj)
        : StateEvent(typeId(), obj, QStringLiteral("aliases"))
    {}
    RoomAliasesEvent(const QString& server, const QStringList& aliases)
        : StateEvent(typeId(), matrixTypeId(), server,
                     QStringLiteral("aliases"), aliases)
    {}
    QString server() const { return stateKey(); }
    QStringList aliases() const { return content().value; }
};
REGISTER_EVENT_TYPE(RoomAliasesEvent)
} // namespace Quotient
