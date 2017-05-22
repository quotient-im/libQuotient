/******************************************************************************
 * Copyright (C) 2015 Felix Rohrbach <kde@fxrh.de>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#pragma once

#include <QtCore/QString>
#include <QtCore/QDateTime>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>

#include "util.h"

namespace QMatrixClient
{
    class Event
    {
            Q_GADGET
        public:
            enum class Type
            {
                RoomMessage, RoomName, RoomAliases, RoomCanonicalAlias,
                RoomMember, RoomTopic, Typing, Receipt, Unknown
            };

            explicit Event(Type type, const QJsonObject& rep);
            Event(const Event&) = delete;

            Type type() const { return _type; }
            QByteArray originalJson() const;

            // Every event also has a "content" object but since its structure is
            // different for different types, we're implementing it per-event type
            // (and in most cases it will be a combination of other fields
            // instead of "content" field).

            static Event* fromJson(const QJsonObject& obj);

        protected:
            static QDateTime toTimestamp(const QJsonValue& v);
            static QStringList toStringList(const QJsonValue& v);

            const QJsonObject contentJson() const;

        private:
            Type _type;
            QJsonObject _originalJson;

            REGISTER_ENUM(Type)
    };
    using EventType = Event::Type;
    template <typename EventT>
    using EventsBatch = std::vector<EventT*>;
    using Events = EventsBatch<Event>;

    template <typename BaseEventT>
    BaseEventT* makeEvent(const QJsonObject& obj)
    {
        if (auto e = BaseEventT::fromJson(obj))
            return e;

        return new BaseEventT(EventType::Unknown, obj);
    }

    template <typename BaseEventT = Event,
              typename BatchT = EventsBatch<BaseEventT> >
    BatchT makeEvents(const QJsonArray& objs)
    {
        BatchT evs;
        // The below line accommodates the difference in size types of
        // STL and Qt containers.
        evs.reserve(static_cast<typename BatchT::size_type>(objs.size()));
        for (auto obj: objs)
            evs.push_back(makeEvent<BaseEventT>(obj.toObject()));
        return evs;
    }

    class RoomEvent : public Event
    {
        public:
            RoomEvent(Type type, const QJsonObject& rep);

            const QString& id() const          { return _id; }
            const QDateTime& timestamp() const { return _serverTimestamp; }
            const QString& roomId() const      { return _roomId; }
            const QString& senderId() const    { return _senderId; }

            // "Static override" of the one in Event
            static RoomEvent* fromJson(const QJsonObject& obj);

        private:
            QString _id;
            QDateTime _serverTimestamp;
            QString _roomId;
            QString _senderId;
    };
    using RoomEvents = EventsBatch<RoomEvent>;
}  // namespace QMatrixClient
