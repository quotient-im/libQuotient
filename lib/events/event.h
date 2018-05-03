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

#include "converters.h"
#include "util.h"

namespace QMatrixClient
{
    template <typename EventT>
    using event_ptr_tt = std::unique_ptr<EventT>;

    template <typename EventT>
    inline EventT* rawPtr(const event_ptr_tt<EventT>& ptr)
    {
        return ptr.get();
    }

    template <typename TargetEventT, typename EventT>
    inline TargetEventT* weakPtr(const event_ptr_tt<EventT>& ptr)
    {
        return static_cast<TargetEventT*>(rawPtr(ptr));
    }

    template <typename TargetT, typename SourceT>
    inline event_ptr_tt<TargetT> ptrCast(event_ptr_tt<SourceT>&& ptr)
    {
        return unique_ptr_cast<TargetT>(ptr);
    }

    namespace _impl
    {
        template <typename EventT, typename... ArgTs>
        inline event_ptr_tt<EventT> create(ArgTs&&... args)
        {
            return std::make_unique<EventT>(std::forward<ArgTs>(args)...);
        }

        template <typename EventT>
        inline event_ptr_tt<EventT> doMakeEvent(const QJsonObject& obj)
        {
            return create<EventT>(obj);
        }
    }

    class Event
    {
            Q_GADGET
        public:
            enum class Type : quint16
            {
                Unknown = 0,
                Typing, Receipt, Tag, DirectChat, ReadMarker,
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
            QString jsonType() const;
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

    /** Create an event with proper type from a JSON object
     * Use this factory template to detect the type from the JSON object
     * contents (the detected event type should derive from the template
     * parameter type) and create an event object of that type.
     */
    template <typename EventT>
    inline event_ptr_tt<EventT> makeEvent(const QJsonObject& obj)
    {
        auto e = _impl::doMakeEvent<EventT>(obj);
        if (!e)
            e = _impl::create<EventT>(EventType::Unknown, obj);
        return e;
    }

    namespace _impl
    {
        template <>
        EventPtr doMakeEvent<Event>(const QJsonObject& obj);
    }

    template <typename EventT> struct FromJson<event_ptr_tt<EventT>>
    {
        auto operator()(const QJsonValue& jv) const
        {
            return makeEvent<EventT>(jv.toObject());
        }
    };

    template <typename EventT>
    using EventsArray = std::vector<event_ptr_tt<EventT>>;
    using Events = EventsArray<Event>;

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
            ~RoomEvent() override;

            QString id() const { return _id; }
            QDateTime timestamp() const;
            QString roomId() const;
            QString senderId() const;
            bool isRedacted() const { return bool(_redactedBecause); }
            const event_ptr_tt<RedactionEvent>& redactedBecause() const
            {
                return _redactedBecause;
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
            event_ptr_tt<RedactionEvent> _redactedBecause;
            QString _txnId;
    };
    using RoomEventPtr = event_ptr_tt<RoomEvent>;
    using RoomEvents = EventsArray<RoomEvent>;
    using RoomEventsRange = Range<RoomEvents>;

    namespace _impl
    {
        template <>
        RoomEventPtr doMakeEvent<RoomEvent>(const QJsonObject& obj);
    }

    class StateEventBase: public RoomEvent
    {
        public:
            explicit StateEventBase(Type type, const QJsonObject& obj)
                : RoomEvent(obj.contains("state_key") ? type : Type::Unknown,
                            obj)
            { }
            explicit StateEventBase(Type type)
                : RoomEvent(type)
            { }
            ~StateEventBase() override = default;

            virtual bool repeatsState() const;
    };
    using StateEventPtr = event_ptr_tt<StateEventBase>;
    using StateEvents = EventsArray<StateEventBase>;

    namespace _impl
    {
        template <>
        StateEventPtr doMakeEvent<StateEventBase>(const QJsonObject& obj);
    }

    template <typename ContentT>
    struct Prev
    {
        template <typename... ContentParamTs>
        explicit Prev(const QJsonObject& unsignedJson,
                      ContentParamTs&&... contentParams)
            : senderId(unsignedJson.value("prev_sender").toString())
            , content(unsignedJson.value("prev_content").toObject(),
                       std::forward<ContentParamTs>(contentParams)...)
        { }

        QString senderId;
        ContentT content;
    };

    template <typename ContentT>
    class StateEvent: public StateEventBase
    {
        public:
            using content_type = ContentT;

            template <typename... ContentParamTs>
            explicit StateEvent(Type type, const QJsonObject& obj,
                                ContentParamTs&&... contentParams)
                : StateEventBase(type, obj)
                , _content(contentJson(),
                           std::forward<ContentParamTs>(contentParams)...)
            {
                auto unsignedData = obj.value("unsigned").toObject();
                if (unsignedData.contains("prev_content"))
                    _prev = std::make_unique<Prev<ContentT>>(unsignedData,
                        std::forward<ContentParamTs>(contentParams)...);
            }
            template <typename... ContentParamTs>
            explicit StateEvent(Type type, ContentParamTs&&... contentParams)
                : StateEventBase(type)
                , _content(std::forward<ContentParamTs>(contentParams)...)
            { }

            QJsonObject toJson() const { return _content.toJson(); }

            const ContentT& content() const { return _content; }
            /** @deprecated Use prevContent instead */
            const ContentT* prev_content() const { return prevContent(); }
            const ContentT* prevContent() const
            { return _prev ? &_prev->content : nullptr; }
            QString prevSenderId() const { return _prev ? _prev->senderId : ""; }

        protected:
            ContentT _content;
            std::unique_ptr<Prev<ContentT>> _prev;
    };
}  // namespace QMatrixClient
Q_DECLARE_METATYPE(QMatrixClient::Event*)
Q_DECLARE_METATYPE(QMatrixClient::RoomEvent*)
Q_DECLARE_METATYPE(const QMatrixClient::Event*)
Q_DECLARE_METATYPE(const QMatrixClient::RoomEvent*)
