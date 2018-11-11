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
#include "logging.h"

#ifdef ENABLE_EVENTTYPE_ALIAS
#define USE_EVENTTYPE_ALIAS 1
#endif

namespace QMatrixClient
{
    // === event_ptr_tt<> and type casting facilities ===

    template <typename EventT>
    using event_ptr_tt = std::unique_ptr<EventT>;

    template <typename EventT>
    inline EventT* rawPtr(const event_ptr_tt<EventT>& ptr) // unwrap
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
    static const auto UnsignedKey = QStringLiteral("unsigned");
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

    // === Event types and event types registry ===

    using event_type_t = size_t;
    using event_mtype_t = const char*;

    class EventTypeRegistry
    {
        public:
            ~EventTypeRegistry() = default;

            static event_type_t initializeTypeId(event_mtype_t matrixTypeId);

            template <typename EventT>
            static inline event_type_t initializeTypeId()
            {
                return initializeTypeId(EventT::matrixTypeId());
            }

            static QString getMatrixType(event_type_t typeId);

        private:
            EventTypeRegistry() = default;
            Q_DISABLE_COPY(EventTypeRegistry)
            DISABLE_MOVE(EventTypeRegistry)

            static EventTypeRegistry& get()
            {
                static EventTypeRegistry etr;
                return etr;
            }

            std::vector<event_mtype_t> eventTypes;
    };

    template <>
    inline event_type_t EventTypeRegistry::initializeTypeId<void>()
    {
        return initializeTypeId("");
    }

    template <typename EventT>
    struct EventTypeTraits
    {
        static event_type_t id()
        {
            static const auto id = EventTypeRegistry::initializeTypeId<EventT>();
            return id;
        }
    };

    template <typename EventT>
    inline event_type_t typeId()
    {
        return EventTypeTraits<std::decay_t<EventT>>::id();
    }

    inline event_type_t unknownEventTypeId() { return typeId<void>(); }

    // === EventFactory ===

    /** Create an event of arbitrary type from its arguments */
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
            static auto addMethod(FnT&& method)
            {
                factories().emplace_back(std::forward<FnT>(method));
                return 0;
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
                return addMethod(&EventT::factory_t::make);
            }

            static event_ptr_tt<BaseEventT> make(const QJsonObject& json,
                                                 const QString& matrixType)
            {
                for (const auto& f: factories())
                    if (auto e = f(json, matrixType))
                        return e;
                return nullptr;
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
    inline auto setupFactory()
    {
        qDebug(EVENTS) << "Adding factory method for" << EventT::matrixTypeId();
        return EventT::factory_t::addMethod(
            [] (const QJsonObject& json, const QString& jsonMatrixType)
            {
                return EventT::matrixTypeId() == jsonMatrixType
                        ? makeEvent<EventT>(json) : nullptr;
            });
    }

    template <typename EventT>
    inline auto registerEventType()
    {
        static const auto _ = setupFactory<EventT>();
        return _; // Only to facilitate usage in static initialisation
    }

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
            Q_DISABLE_COPY(Event)
            Event(Event&&) = default;
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

            template <typename T>
            T content(const QString& key) const
            {
                return fromJson<T>(contentJson()[key]);
            }

            template <typename T>
            T content(const QLatin1String& key) const
            {
                return fromJson<T>(contentJson()[key]);
            }

            virtual bool isStateEvent() const { return false; }
            virtual bool isCallEvent() const { return false; }

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
        static const auto _factoryAdded##_Type = registerEventType<_Type>(); \
    } \
    // End of macro

#ifdef USE_EVENTTYPE_ALIAS
    namespace EventType
    {
        inline event_type_t logEventType(event_type_t id, const char* idName)
        {
            qDebug(EVENTS) << "Using id" << id << "for" << idName;
            return id;
        }
    }

    // This macro provides constants in EventType:: namespace for
    // back-compatibility with libQMatrixClient 0.3 event type system.
#define DEFINE_EVENTTYPE_ALIAS(_Id, _Type) \
    namespace EventType \
    { \
        [[deprecated("Use is<>(), eventCast<>() or visit<>()")]] \
        static const auto _Id = logEventType(typeId<_Type>(), #_Id); \
    } \
    // End of macro
#else
#define DEFINE_EVENTTYPE_ALIAS(_Id, _Type) // Nothing
#endif

    // === is<>(), eventCast<>() and visit<>() ===

    template <typename EventT>
    inline bool is(const Event& e) { return e.type() == typeId<EventT>(); }

    inline bool isUnknown(const Event& e) { return e.type() == unknownEventTypeId(); }

    template <typename EventT, typename BasePtrT>
    inline auto eventCast(const BasePtrT& eptr)
        -> decltype(static_cast<EventT*>(&*eptr))
    {
        Q_ASSERT(eptr);
        return is<EventT>(*eptr) ? static_cast<EventT*>(&*eptr) : nullptr;
    }

    // A single generic catch-all visitor
    template <typename BaseEventT, typename FnT>
    inline auto visit(const BaseEventT& event, FnT&& visitor)
        -> decltype(visitor(event))
    {
        return visitor(event);
    }

    template <typename T>
    constexpr auto is_event()
    {
        return std::is_base_of<Event, std::decay_t<T>>::value;
    }

    template <typename T, typename FnT>
    constexpr auto needs_cast()
    {
        return !std::is_convertible<T, fn_arg_t<FnT>>::value;
    }

    // A single type-specific void visitor
    template <typename BaseEventT, typename FnT>
    inline
    std::enable_if_t<
        is_event<BaseEventT>() && needs_cast<BaseEventT, FnT>() &&
        std::is_void<fn_return_t<FnT>>::value>
    visit(const BaseEventT& event, FnT&& visitor)
    {
        using event_type = fn_arg_t<FnT>;
        if (is<event_type>(event))
            visitor(static_cast<event_type>(event));
    }

    // A single type-specific non-void visitor with an optional default value
    template <typename BaseEventT, typename FnT>
    inline
    std::enable_if_t<
        is_event<BaseEventT>() && needs_cast<BaseEventT, FnT>(),
        fn_return_t<FnT>> // non-voidness is guarded by defaultValue type
    visit(const BaseEventT& event, FnT&& visitor,
          fn_return_t<FnT>&& defaultValue = {})
    {
        using event_type = fn_arg_t<FnT>;
        if (is<event_type>(event))
            return visitor(static_cast<event_type>(event));
        return std::forward<fn_return_t<FnT>>(defaultValue);
    }

    // A chain of 2 or more visitors
    template <typename BaseEventT, typename FnT1, typename FnT2, typename... FnTs>
    inline
    std::enable_if_t<is_event<BaseEventT>(), fn_return_t<FnT1>>
    visit(const BaseEventT& event, FnT1&& visitor1, FnT2&& visitor2,
          FnTs&&... visitors)
    {
        using event_type1 = fn_arg_t<FnT1>;
        if (is<event_type1>(event))
            return visitor1(static_cast<event_type1&>(event));
        return visit(event, std::forward<FnT2>(visitor2),
                     std::forward<FnTs>(visitors)...);
    }
}  // namespace QMatrixClient
Q_DECLARE_METATYPE(QMatrixClient::Event*)
Q_DECLARE_METATYPE(const QMatrixClient::Event*)
