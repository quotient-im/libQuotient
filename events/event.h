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
                RoomMember, RoomTopic, RoomAvatar,
                RoomEncryption, RoomEncryptedMessage,
                Typing, Receipt, Unknown
            };

            explicit Event(Type type) : _type(type) { }
            Event(Type type, const QJsonObject& rep);
            Event(const Event&) = delete;

            Type type() const { return _type; }
            QByteArray originalJson() const;
            QJsonObject originalJsonObject() const;

            // According to the CS API spec, every event also has
            // a "content" object; but since its structure is different for
            // different types, we're implementing it per-event type
            // (and in most cases it will be a combination of other fields
            // instead of "content" field).

            static Event* fromJson(const QJsonObject& obj);

        protected:
            const QJsonObject contentJson() const;

        private:
            Type _type;
            QJsonObject _originalJson;

            REGISTER_ENUM(Type)
            Q_PROPERTY(Type type READ type CONSTANT)
            Q_PROPERTY(QJsonObject contentJson READ contentJson CONSTANT)
    };
    using EventType = Event::Type;
    template <typename EventT>
    using EventsBatch = std::vector<EventT*>;
    using Events = EventsBatch<Event>;

    template <typename BaseEventT = Event,
              typename BatchT = EventsBatch<BaseEventT> >
    inline BatchT makeEvents(const QJsonArray& objs)
    {
        BatchT evs;
        // The below line accommodates the difference in size types of
        // STL and Qt containers.
        evs.reserve(static_cast<typename BatchT::size_type>(objs.size()));
        for (auto objValue: objs)
        {
            const auto o = objValue.toObject();
            auto e = BaseEventT::fromJson(o);
            evs.push_back(e ? e : new BaseEventT(EventType::Unknown, o));
        }
        return evs;
    }

    /** This class corresponds to m.room.* events */
    class RoomEvent : public Event
    {
            Q_GADGET
            Q_PROPERTY(QString id READ id)
            Q_PROPERTY(QDateTime timestamp READ timestamp CONSTANT)
            Q_PROPERTY(QString roomId READ roomId CONSTANT)
            Q_PROPERTY(QString senderId READ senderId CONSTANT)
            Q_PROPERTY(QString transactionId READ transactionId CONSTANT)
        public:
            explicit RoomEvent(Type type) : Event(type) { }
            RoomEvent(Type type, const QJsonObject& rep);

            const QString& id() const { return _id; }
            const QDateTime& timestamp() const { return _serverTimestamp; }
            const QString& roomId() const { return _roomId; }
            const QString& senderId() const { return _senderId; }
            const QString& transactionId() const { return _txnId; }

            /**
             * Sets the transaction id for locally created events. This should be
             * done before the event is exposed to any code using the respective
             * Q_PROPERTY.
             *
             * \param txnId - transaction id, normally obtained from
             * Connection::generateTxnId()
             */
            void setTransactionId(const QString& txnId) { _txnId = txnId; }

            /**
             * Sets event id for locally created events
             *
             * When a new event is created locally, it has no server id yet.
             * This function allows to add the id once the confirmation from
             * the server is received. There should be no id set previously
             * in the event. It's the responsibility of the code calling addId()
             * to notify clients that use Q_PROPERTY(id) about its change
             */
            void addId(const QString& id);

            // "Static override" of the one in Event
            static RoomEvent* fromJson(const QJsonObject& obj);

        private:
            QString _id;
            QDateTime _serverTimestamp;
            QString _roomId;
            QString _senderId;
            QString _txnId;
    };
    using RoomEvents = EventsBatch<RoomEvent>;

    template <typename ContentT>
    class StateEvent: public RoomEvent
    {
        public:
            using content_type = ContentT;

            template <typename... ContentParamTs>
            explicit StateEvent(Type type, const QJsonObject& obj,
                                ContentParamTs&&... contentParams)
                : RoomEvent(type, obj)
                , _content(contentJson(),
                           std::forward<ContentParamTs>(contentParams)...)
                , _prev(new ContentT(obj["prev_content"].toObject(),
                            std::forward<ContentParamTs>(contentParams)...))
            { }
            template <typename... ContentParamTs>
            explicit StateEvent(Type type, ContentParamTs&&... contentParams)
                : RoomEvent(type)
                , _content(std::forward<ContentParamTs>(contentParams)...)
            { }

            QJsonObject toJson() const { return _content.toJson(); }

            ContentT content() const { return _content; }
            ContentT* prev_content() const { return _prev.data(); }

        protected:
            ContentT _content;
            QScopedPointer<ContentT> _prev;
    };
}  // namespace QMatrixClient
Q_DECLARE_OPAQUE_POINTER(QMatrixClient::Event*)
Q_DECLARE_METATYPE(QMatrixClient::Event*)
Q_DECLARE_OPAQUE_POINTER(QMatrixClient::RoomEvent*)
Q_DECLARE_METATYPE(QMatrixClient::RoomEvent*)
