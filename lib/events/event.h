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

#include <typeindex>

namespace QMatrixClient
{
    // === event_ptr_tt<> and type casting facilities ===

    template <typename EventT>
    using event_ptr_tt = std::unique_ptr<EventT>;

    template <typename EventT>
    inline EventT* rawPtr(const event_ptr_tt<EventT>& ptr)
    {
        return ptr.get();
    }

    template <typename TargetEventT, typename EventT>
    inline TargetEventT* weakPtrCast(const event_ptr_tt<EventT>& ptr)
    {
        return static_cast<TargetEventT*>(rawPtr(ptr));
    }

    template <typename TargetT, typename SourceT>
    inline event_ptr_tt<TargetT> ptrCast(event_ptr_tt<SourceT>&& ptr)
    {
        return unique_ptr_cast<TargetT>(ptr);
    }

    // === Standard Matrix key names and basicEventJson() ===

    static const auto TypeKey = QStringLiteral("type");
    static const auto ContentKey = QStringLiteral("content");
    static const auto EventIdKey = QStringLiteral("event_id");
    static const auto TypeKeyL = "type"_ls;
    static const auto ContentKeyL = "content"_ls;
    static const auto EventIdKeyL = "event_id"_ls;
    static const auto UnsignedKeyL = "unsigned"_ls;
    static const auto RedactedCauseKeyL = "redacted_because"_ls;
    static const auto PrevContentKeyL = "prev_content"_ls;

    // Minimal correct Matrix event JSON
    template <typename StrT>
    inline QJsonObject basicEventJson(StrT matrixType,
                                      const QJsonObject& content)
    {
        return { { TypeKey, std::forward<StrT>(matrixType) },
                 { ContentKey, content } };
    }

    // === Event factory ===

    using event_type_t = size_t;
    using event_mtype_t = const char*;

    template <typename EventT>
    struct EventTypeTraits
    {
        static const event_type_t id;
    };

    template <>
    struct EventTypeTraits<void>
    {
        static constexpr event_type_t id = 0;
    };

    event_type_t nextTypeId();

    template <typename EventT>
    const event_type_t EventTypeTraits<EventT>::id = nextTypeId();

    template <typename EventT>
    inline event_type_t typeId() { return EventTypeTraits<std::decay_t<EventT>>::id; }

    inline event_type_t unknownEventTypeId() { return typeId<void>(); }

    template <typename EventT, typename... ArgTs>
    inline event_ptr_tt<EventT> makeEvent(ArgTs&&... args)
    {
        return std::make_unique<EventT>(std::forward<ArgTs>(args)...);
    }

    template <typename BaseEventT>
    class EventFactory
    {
        public:
            template <typename FnT>
            static void addMethod(FnT&& method)
            {
                factories().emplace_back(std::forward<FnT>(method));
            }

            /** Chain two type factories
             * Adds the factory class of EventT2 (EventT2::factory_t) to
             * the list in factory class of EventT1 (EventT1::factory_t) so
             * that when EventT1::factory_t::make() is invoked, types of
             * EventT2 factory are looked through as well. This is used
             * to include RoomEvent types into the more general Event factory,
             * and state event types into the RoomEvent factory.
             */
            template <typename EventT>
            static auto chainFactory()
            {
                addMethod(&EventT::factory_t::make);
                return 0;
            }

            static event_ptr_tt<BaseEventT> make(const QJsonObject& json,
                                                 const QString& matrixType)
            {
                for (const auto& f: factories())
                    if (auto e = f(json, matrixType))
                        return e;
                return makeEvent<BaseEventT>(unknownEventTypeId(), json);
            }

        private:
            static auto& factories()
            {
                using inner_factory_tt =
                    std::function<event_ptr_tt<BaseEventT>(const QJsonObject&,
                                                           const QString&)>;
                static std::vector<inner_factory_tt> _factories {};
                return _factories;
            }
    };

    /** Add a type to its default factory
     * Adds a standard factory method (via makeEvent<>) for a given
     * type to EventT::factory_t factory class so that it can be
     * created dynamically from loadEvent<>().
     *
     * \tparam EventT the type to enable dynamic creation of
     * \return the registered type id
     * \sa loadEvent, Event::type
     */
    template <typename EventT>
    inline void setupFactory()
    {
        EventT::factory_t::addMethod(
            [] (const QJsonObject& json, const QString& jsonMatrixType)
            {
                return EventT::matrixTypeId() == jsonMatrixType
                        ? makeEvent<EventT>(json) : nullptr;
            });
    }

    /** Create an event with proper type from a JSON object
     * Use this factory template to detect the type from the JSON object
     * contents (the detected event type should derive from the template
     * parameter type) and create an event object of that type.
     */
    template <typename BaseEventT>
    inline event_ptr_tt<BaseEventT> loadEvent(const QJsonObject& fullJson)
    {
        return EventFactory<BaseEventT>
                ::make(fullJson, fullJson[TypeKeyL].toString());
    }

    /** Create an event from a type string and content JSON
     * Use this factory template to resolve the C++ type from the Matrix
     * type string in \p matrixType and create an event of that type that has
     * its content part set to \p content.
     */
    template <typename BaseEventT>
    inline event_ptr_tt<BaseEventT> loadEvent(const QString& matrixType,
                                              const QJsonObject& content)
    {
        return EventFactory<BaseEventT>
                ::make(basicEventJson(matrixType, content), matrixType);
    }

    template <typename EventT> struct FromJson<event_ptr_tt<EventT>>
    {
        auto operator()(const QJsonValue& jv) const
        {
            return loadEvent<EventT>(jv.toObject());
        }
    };

    // === Event ===

    class Event
    {
            Q_GADGET
            Q_PROPERTY(Type type READ type CONSTANT)
            Q_PROPERTY(QJsonObject contentJson READ contentJson CONSTANT)
        public:
            using Type = event_type_t;
            using factory_t = EventFactory<Event>;

            explicit Event(Type type, const QJsonObject& json);
            explicit Event(Type type, event_mtype_t matrixType,
                           const QJsonObject& contentJson = {});
            Event(const Event&) = delete;
            Event(Event&&) = default;
            Event& operator=(const Event&) = delete;
            Event& operator=(Event&&) = delete;
            virtual ~Event();

            Type type() const { return _type; }
            QString matrixType() const;
            QByteArray originalJson() const;
            QJsonObject originalJsonObject() const { return fullJson(); }

            const QJsonObject& fullJson() const { return _json; }

            // According to the CS API spec, every event also has
            // a "content" object; but since its structure is different for
            // different types, we're implementing it per-event type.

            const QJsonObject contentJson() const;
            const QJsonObject unsignedJson() const;

            virtual bool isStateEvent() const { return false; }

        protected:
            QJsonObject& editJson() { return _json; }

        private:
            Type _type;
            QJsonObject _json;
    };
    using EventPtr = event_ptr_tt<Event>;

    template <typename EventT>
    using EventsArray = std::vector<event_ptr_tt<EventT>>;
    using Events = EventsArray<Event>;

    // === Macros used with event class definitions ===

    // This macro should be used in a public section of an event class to
    // provide matrixTypeId() and typeId().
#define DEFINE_EVENT_TYPEID(_Id, _Type) \
    static constexpr event_mtype_t matrixTypeId() { return _Id; } \
    static auto typeId() { return QMatrixClient::typeId<_Type>(); } \
    // End of macro

    // This macro should be put after an event class definition (in .h or .cpp)
    // to enable its deserialisation from a /sync and other
    // polymorphic event arrays
#define REGISTER_EVENT_TYPE(_Type) \
    namespace { \
        [[gnu::unused]] \
        static const auto _factoryAdded##_Type = ( setupFactory<_Type>(), 0); \
    } \
    // End of macro

    // This macro provides constants in EventType:: namespace for
    // back-compatibility with libQMatrixClient 0.3 event type system.
#define DEFINE_EVENTTYPE_ALIAS(_Id, _Type) \
    namespace EventType \
    { \
        [[deprecated("Use typeId<>(), is<>() or visit<>()")]] \
        static const auto _Id = typeId<_Type>(); \
    } \
    // End of macro

    // === is<>() and visit<>() ===

    template <typename EventT>
    inline bool is(const Event& e) { return e.type() == typeId<EventT>(); }

    inline bool isUnknown(const Event& e) { return e.type() == unknownEventTypeId(); }

    template <typename FnT>
    inline fn_return_t<FnT> visit(const Event& event, FnT visitor)
    {
        using event_type = fn_arg_t<FnT>;
        if (is<event_type>(event))
            return visitor(static_cast<event_type>(event));
        return fn_return_t<FnT>();
    }

    template <typename FnT, typename... FnTs>
    inline auto visit(const Event& event, FnT visitor1, FnTs&&... visitors)
    {
        using event_type1 = fn_arg_t<FnT>;
        if (is<event_type1>(event))
            return visitor1(static_cast<event_type1&>(event));

        return visit(event, std::forward<FnTs>(visitors)...);
    }

    template <typename BaseEventT, typename... FnTs>
    inline auto visit(const event_ptr_tt<BaseEventT>& eptr, FnTs&&... visitors)
    {
        using return_type = decltype(visit(*eptr, visitors...));
        if (eptr)
            return visit(*eptr, visitors...);
        return return_type();
    }

    // === RoomEvent ===

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
            using factory_t = EventFactory<RoomEvent>;

            // RedactionEvent is an incomplete type here so we cannot inline
            // constructors and destructors and we cannot use 'using'.
            RoomEvent(Type type, event_mtype_t matrixType,
                      const QJsonObject& contentJson = {});
            RoomEvent(Type type, const QJsonObject& json);
            ~RoomEvent() override;

            QString id() const;
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
            void addId(const QString& newId);

        private:
            event_ptr_tt<RedactionEvent> _redactedBecause;
            QString _txnId;
    };
    using RoomEventPtr = event_ptr_tt<RoomEvent>;
    using RoomEvents = EventsArray<RoomEvent>;
    using RoomEventsRange = Range<RoomEvents>;

    // === State events ===

    class StateEventBase: public RoomEvent
    {
        public:
            using factory_t = EventFactory<StateEventBase>;

            using RoomEvent::RoomEvent;
            ~StateEventBase() override = default;

            bool isStateEvent() const override { return true; }
            virtual bool repeatsState() const;
    };
    using StateEventPtr = event_ptr_tt<StateEventBase>;
    using StateEvents = EventsArray<StateEventBase>;

    template <typename ContentT>
    struct Prev
    {
        template <typename... ContentParamTs>
        explicit Prev(const QJsonObject& unsignedJson,
                      ContentParamTs&&... contentParams)
            : senderId(unsignedJson.value("prev_sender"_ls).toString())
            , content(unsignedJson.value(PrevContentKeyL).toObject(),
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
            explicit StateEvent(Type type, const QJsonObject& fullJson,
                                ContentParamTs&&... contentParams)
                : StateEventBase(type, fullJson)
                , _content(contentJson(),
                           std::forward<ContentParamTs>(contentParams)...)
            {
                const auto& unsignedData = unsignedJson();
                if (unsignedData.contains(PrevContentKeyL))
                    _prev = std::make_unique<Prev<ContentT>>(unsignedData,
                                std::forward<ContentParamTs>(contentParams)...);
            }
            template <typename... ContentParamTs>
            explicit StateEvent(Type type, event_mtype_t matrixType,
                                ContentParamTs&&... contentParams)
                : StateEventBase(type, matrixType)
                , _content(std::forward<ContentParamTs>(contentParams)...)
            {
                editJson().insert(ContentKey, _content.toJson());
            }

            const ContentT& content() const { return _content; }
            [[deprecated("Use prevContent instead")]]
            const ContentT* prev_content() const { return prevContent(); }
            const ContentT* prevContent() const
            { return _prev ? &_prev->content : nullptr; }
            QString prevSenderId() const
                { return _prev ? _prev->senderId : QString(); }

        protected:
            ContentT _content;
            std::unique_ptr<Prev<ContentT>> _prev;
    };
}  // namespace QMatrixClient
Q_DECLARE_METATYPE(QMatrixClient::Event*)
Q_DECLARE_METATYPE(QMatrixClient::RoomEvent*)
Q_DECLARE_METATYPE(const QMatrixClient::Event*)
Q_DECLARE_METATYPE(const QMatrixClient::RoomEvent*)
