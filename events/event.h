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

#include <memory>

namespace QMatrixClient
{
    template <typename EventT>
    using event_ptr_tt = std::unique_ptr<EventT>;

    /** Create an event with proper type from a JSON object
     * Use this factory template to detect the type from the JSON object
     * contents (the detected event type should derive from the template
     * parameter type) and create an event object of that type.
     */
    template <typename EventT>
    event_ptr_tt<EventT> makeEvent(const QJsonObject& obj);

    class Event
    {
            Q_GADGET
        public:
            enum class Type : quint16
            {
                Unknown = 0,
                Typing, Receipt,
                RoomEventBase = 0x1000,
                RoomMessage = RoomEventBase + 1,
                RoomEncryptedMessage, Redaction,
                RoomStateEventBase = 0x1800,
                RoomName = RoomStateEventBase + 1,
                RoomAliases, RoomCanonicalAlias, RoomMember, RoomTopic,
                RoomAvatar, RoomEncryption, RoomCreate, RoomJoinRules,
                RoomPowerLevels,
                Reserved = 0x2000
            };

            explicit Event(Type type) : _type(type) { }
            Event(Type type, const QJsonObject& rep);
            Event(const Event&) = delete;
            virtual ~Event();

            Type type() const { return _type; }
            bool isStateEvent() const
            {
                return (quint16(_type) & 0x1800) == 0x1800;
            }
            QByteArray originalJson() const;
            QJsonObject originalJsonObject() const;

            // According to the CS API spec, every event also has
            // a "content" object; but since its structure is different for
            // different types, we're implementing it per-event type
            // (and in most cases it will be a combination of other fields
            // instead of "content" field).

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
    using EventPtr = event_ptr_tt<Event>;

    template <>
    EventPtr makeEvent<Event>(const QJsonObject& obj);

    /**
     * \brief A vector of pointers to events with deserialisation capabilities
     *
     * This is a simple wrapper over a generic vector type that adds
     * a convenience method to deserialise events from QJsonArray.
     * Note that this type does not own pointers to events. If owning
     * semantics is needed, one should use the Owning<> wrapper around
     * the container (e.g. \code Owning<EventsBatch<Event>> \endcode).
     * \tparam EventT base type of all events in the vector
     */
    template <typename EventT>
    class EventsBatch : public std::vector<event_ptr_tt<EventT>>
    {
        public:
            /**
             * \brief Deserialise events from an array
             *
             * Given the following JSON construct, creates events from
             * the array stored at key "node":
             * \code
             * "container": {
             *     "node": [ { "event_id": "!evt1:srv.org", ... }, ... ]
             * }
             * \endcode
             * \param container - the wrapping JSON object
             * \param node - the key in container that holds the array of events
             */
            void fromJson(const QJsonObject& container, const QString& node)
            {
                const auto objs = container.value(node).toArray();
                using size_type = typename std::vector<EventT*>::size_type;
                // The below line accommodates the difference in size types of
                // STL and Qt containers.
                this->reserve(static_cast<size_type>(objs.size()));
                for (auto objValue: objs)
                {
                    const auto o = objValue.toObject();
                    auto&& e = makeEvent<EventT>(o);
                    if (!e)
                        e.reset(new EventT(EventType::Unknown, o));
                    this->emplace_back(std::move(e));
                }
            }
    };
    using Events = EventsBatch<Event>;

    class RedactionEvent;

    /** This class corresponds to m.room.* events */
    class RoomEvent : public Event
    {
            Q_GADGET
            Q_PROPERTY(QString id READ id)
            Q_PROPERTY(QDateTime timestamp READ timestamp CONSTANT)
            Q_PROPERTY(QString roomId READ roomId CONSTANT)
            Q_PROPERTY(QString senderId READ senderId CONSTANT)
            Q_PROPERTY(QString redactionReason READ redactionReason)
            Q_PROPERTY(bool isRedacted READ isRedacted)
            Q_PROPERTY(QString transactionId READ transactionId)
        public:
            // RedactionEvent is an incomplete type here so we cannot inline
            // constructors and destructors
            explicit RoomEvent(Type type);
            RoomEvent(Type type, const QJsonObject& rep);
            ~RoomEvent();

            const QString& id() const { return _id; }
            const QDateTime& timestamp() const { return _serverTimestamp; }
            const QString& roomId() const { return _roomId; }
            const QString& senderId() const { return _senderId; }
            bool isRedacted() const { return bool(_redactedBecause); }
            const RedactionEvent* redactedBecause() const
            {
                return _redactedBecause.get();
            }
            QString redactionReason() const;
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

        private:
            QString _id;
            QString _roomId;
            QString _senderId;
            QDateTime _serverTimestamp;
            event_ptr_tt<RedactionEvent> _redactedBecause;
            QString _txnId;
    };
    using RoomEvents = EventsBatch<RoomEvent>;
    using RoomEventPtr = event_ptr_tt<RoomEvent>;

    template <>
    RoomEventPtr makeEvent<RoomEvent>(const QJsonObject& obj);

    /**
     * Conceptually similar to QStringView (but much more primitive), it's a
     * simple abstraction over a pair of RoomEvents::const_iterator values
     * referring to the beginning and the end of a range in a RoomEvents
     * container.
     */
    struct RoomEventsRange
    {
        RoomEvents::iterator from;
        RoomEvents::iterator to;

        RoomEvents::size_type size() const
        {
            Q_ASSERT(std::distance(from, to) >= 0);
            return RoomEvents::size_type(std::distance(from, to));
        }
        bool empty() const { return from == to; }
        RoomEvents::const_iterator begin() const { return from; }
        RoomEvents::const_iterator end() const { return to; }
        RoomEvents::iterator begin() { return from; }
        RoomEvents::iterator end() { return to; }
    };

    template <typename ContentT>
    class StateEvent: public RoomEvent
    {
        public:
            using content_type = ContentT;

            template <typename... ContentParamTs>
            explicit StateEvent(Type type, const QJsonObject& obj,
                                ContentParamTs&&... contentParams)
                : RoomEvent(obj.contains("state_key") ? type : Type::Unknown,
                            obj)
                , _content(contentJson(),
                           std::forward<ContentParamTs>(contentParams)...)
            {
                if (obj.contains("prev_content"))
                    _prev.reset(new ContentT(
                            obj["prev_content"].toObject(),
                            std::forward<ContentParamTs>(contentParams)...));
            }
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
