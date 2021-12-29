// SPDX-FileCopyrightText: 2016 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "converters.h"
#include "logging.h"
#include "function_traits.h"

namespace Quotient {
// === event_ptr_tt<> and type casting facilities ===

template <typename EventT>
using event_ptr_tt = std::unique_ptr<EventT>;

/// Unwrap a plain pointer from a smart pointer
template <typename EventT>
inline EventT* rawPtr(const event_ptr_tt<EventT>& ptr)
{
    return ptr.get();
}

/// Unwrap a plain pointer and downcast it to the specified type
template <typename TargetEventT, typename EventT>
inline TargetEventT* weakPtrCast(const event_ptr_tt<EventT>& ptr)
{
    return static_cast<TargetEventT*>(rawPtr(ptr));
}

// === Standard Matrix key names and basicEventJson() ===

constexpr auto TypeKeyL = "type"_ls;
constexpr auto BodyKeyL = "body"_ls;
constexpr auto ContentKeyL = "content"_ls;
constexpr auto EventIdKeyL = "event_id"_ls;
constexpr auto SenderKeyL = "sender"_ls;
constexpr auto RoomIdKeyL = "room_id"_ls;
constexpr auto UnsignedKeyL = "unsigned"_ls;
constexpr auto RedactedCauseKeyL = "redacted_because"_ls;
constexpr auto PrevContentKeyL = "prev_content"_ls;
constexpr auto StateKeyKeyL = "state_key"_ls;
const QString TypeKey { TypeKeyL };
const QString BodyKey { BodyKeyL };
const QString ContentKey { ContentKeyL };
const QString EventIdKey { EventIdKeyL };
const QString SenderKey { SenderKeyL };
const QString RoomIdKey { RoomIdKeyL };
const QString UnsignedKey { UnsignedKeyL };
const QString StateKeyKey { StateKeyKeyL };

/// Make a minimal correct Matrix event JSON
inline QJsonObject basicEventJson(const QString& matrixType,
                                  const QJsonObject& content)
{
    return { { TypeKey, matrixType }, { ContentKey, content } };
}

// === Event types and event types registry ===

using event_type_t = size_t;
using event_mtype_t = const char*;

class QUOTIENT_API EventTypeRegistry {
public:
    ~EventTypeRegistry() = default;

    static event_type_t initializeTypeId(event_mtype_t matrixTypeId);

    template <typename EventT>
    static event_type_t initializeTypeId()
    {
        return initializeTypeId(EventT::matrixTypeId());
    }

    static QString getMatrixType(event_type_t typeId);

private:
    EventTypeRegistry() = default;
    Q_DISABLE_COPY_MOVE(EventTypeRegistry)

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
struct EventTypeTraits {
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

// === Event creation facilities ===

//! Create an event of arbitrary type from its arguments
template <typename EventT, typename... ArgTs>
inline event_ptr_tt<EventT> makeEvent(ArgTs&&... args)
{
    return std::make_unique<EventT>(std::forward<ArgTs>(args)...);
}

namespace _impl {
    template <class EventT, class BaseEventT>
    event_ptr_tt<BaseEventT> makeIfMatches(const QJsonObject& json,
                                           const QString& matrixType)
    {
        return QLatin1String(EventT::matrixTypeId()) == matrixType
                   ? makeEvent<EventT>(json)
                   : nullptr;
    }

    //! \brief A family of event factories to create events from CS API responses
    //!
    //! Each of these factories, as instantiated by event base types (Event,
    //! RoomEvent etc.) is capable of producing an event object derived from
    //! \p BaseEventT, using the JSON payload and the event type passed to its
    //! make() method. Don't use these directly to make events; use loadEvent()
    //! overloads as the frontend for these. Never instantiate new factories
    //! outside of base event classes.
    //! \sa loadEvent, setupFactory, Event::factory, RoomEvent::factory,
    //!     StateEventBase::factory
    template <typename BaseEventT>
    class EventFactory
        : private std::vector<event_ptr_tt<BaseEventT> (*)(const QJsonObject&,
                                                           const QString&)> {
        // Actual makeIfMatches specialisations will differ in the first
        // template parameter but that doesn't affect the function type
    public:
        explicit EventFactory(const char* name = "")
            : name(name)
        {
            static auto yetToBeConstructed = true;
            Q_ASSERT(yetToBeConstructed);
            if (!yetToBeConstructed) // For Release builds that pass Q_ASSERT
                qCritical(EVENTS)
                    << "Another EventFactory for the same base type is being "
                       "created - event creation logic will be splintered";
            yetToBeConstructed = false;
        }
        EventFactory(const EventFactory&) = delete;

        //! \brief Add a method to create events of a given type
        //!
        //! Adds a standard factory method (makeIfMatches) for \p EventT so that
        //! event objects of this type can be created dynamically by loadEvent.
        //! The caller is responsible for ensuring this method is called only
        //! once per type.
        //! \sa makeIfMatches, loadEvent, Quotient::loadEvent
        template <class EventT>
        bool addMethod()
        {
            this->emplace_back(&makeIfMatches<EventT, BaseEventT>);
            qDebug(EVENTS) << "Added factory method for"
                           << EventT::matrixTypeId() << "events;" << this->size()
                           << "methods in the" << name << "chain by now";
            return true;
        }

        auto loadEvent(const QJsonObject& json, const QString& matrixType)
        {
            for (const auto& f : *this)
                if (auto e = f(json, matrixType))
                    return e;
            return makeEvent<BaseEventT>(unknownEventTypeId(), json);
        }

        const char* const name;
    };
} // namespace _impl

// === Event ===

class QUOTIENT_API Event {
public:
    using Type = event_type_t;
    static inline _impl::EventFactory<Event> factory { "Event" };

    explicit Event(Type type, const QJsonObject& json);
    explicit Event(Type type, event_mtype_t matrixType,
                   const QJsonObject& contentJson = {});
    Q_DISABLE_COPY(Event)
    Event(Event&&) = default;
    Event& operator=(Event&&) = delete;
    virtual ~Event();

    Type type() const { return _type; }
    QString matrixType() const;
    [[deprecated("Use fullJson() and stringify it with QJsonDocument::toJson() "
                 "or by other means")]]
    QByteArray originalJson() const;
    [[deprecated("Use fullJson() instead")]] //
    QJsonObject originalJsonObject() const { return fullJson(); }

    const QJsonObject& fullJson() const { return _json; }

    // According to the CS API spec, every event also has
    // a "content" object; but since its structure is different for
    // different types, we're implementing it per-event type.

    // NB: const return types below are meant to catch accidental attempts
    //     to change event JSON (e.g., consider contentJson()["inexistentKey"]).

    const QJsonObject contentJson() const;

    template <typename T = QJsonValue, typename KeyT>
    const T contentPart(KeyT&& key) const
    {
        return fromJson<T>(contentJson()[std::forward<KeyT>(key)]);
    }

    template <typename T>
    [[deprecated("Use contentPart() to get a part of the event content")]] //
    T content(const QString& key) const
    {
        return contentPart<T>(key);
    }

    const QJsonObject unsignedJson() const;

    template <typename T = QJsonValue, typename KeyT>
    const T unsignedPart(KeyT&& key) const
    {
        return fromJson<T>(unsignedJson()[std::forward<KeyT>(key)]);
    }

    friend QUOTIENT_API QDebug operator<<(QDebug dbg, const Event& e)
    {
        QDebugStateSaver _dss { dbg };
        dbg.noquote().nospace() << e.matrixType() << '(' << e.type() << "): ";
        e.dumpTo(dbg);
        return dbg;
    }

    virtual bool isStateEvent() const { return false; }
    virtual bool isCallEvent() const { return false; }

protected:
    QJsonObject& editJson() { return _json; }
    virtual void dumpTo(QDebug dbg) const;

private:
    Type _type;
    QJsonObject _json;
};
using EventPtr = event_ptr_tt<Event>;

template <typename EventT>
using EventsArray = std::vector<event_ptr_tt<EventT>>;
using Events = EventsArray<Event>;

// === Facilities for event class definitions ===

// This macro should be used in a public section of an event class to
// provide matrixTypeId() and typeId().
#define DEFINE_EVENT_TYPEID(_Id, _Type)                                        \
    static QUOTIENT_EXPORT constexpr event_mtype_t matrixTypeId()              \
    {                                                                          \
        return _Id;                                                            \
    }                                                                          \
    static QUOTIENT_EXPORT auto typeId() { return Quotient::typeId<_Type>(); } \
    // End of macro

// This macro should be put after an event class definition (in .h or .cpp)
// to enable its deserialisation from a /sync and other
// polymorphic event arrays
#define REGISTER_EVENT_TYPE(_Type)                                         \
    [[maybe_unused]] QUOTIENT_API inline const auto _factoryAdded##_Type = \
        _Type::factory.addMethod<_Type>();                                 \
    // End of macro

// === Event loading ===
// (see also event_loader.h)

//! \brief Point of customisation to dynamically load events
//!
//! The default specialisation of this calls BaseEventT::factory and if that
//! fails (i.e. returns nullptr) creates an unknown event of BaseEventT.
//! Other specialisations may reuse other factories, add validations common to
//! BaseEventT, and so on
template <class BaseEventT>
event_ptr_tt<BaseEventT> doLoadEvent(const QJsonObject& json,
                                     const QString& matrixType)
{
    return BaseEventT::factory.loadEvent(json, matrixType);
}

// === is<>(), eventCast<>() and switchOnType<>() ===

template <class EventT>
inline bool is(const Event& e)
{
    return e.type() == typeId<EventT>();
}

inline bool isUnknown(const Event& e)
{
    return e.type() == unknownEventTypeId();
}

template <class EventT, typename BasePtrT>
inline auto eventCast(const BasePtrT& eptr)
    -> decltype(static_cast<EventT*>(&*eptr))
{
    Q_ASSERT(eptr);
    return is<std::decay_t<EventT>>(*eptr) ? static_cast<EventT*>(&*eptr)
                                           : nullptr;
}

// A trivial generic catch-all "switch"
template <class BaseEventT, typename FnT>
inline auto switchOnType(const BaseEventT& event, FnT&& fn)
    -> decltype(fn(event))
{
    return fn(event);
}

namespace _impl {
    // Using bool instead of auto below because auto apparently upsets MSVC
    template <class BaseT, typename FnT>
    inline constexpr bool needs_downcast =
        std::is_base_of_v<BaseT, std::decay_t<fn_arg_t<FnT>>>
        && !std::is_same_v<BaseT, std::decay_t<fn_arg_t<FnT>>>;
}

// A trivial type-specific "switch" for a void function
template <class BaseT, typename FnT>
inline auto switchOnType(const BaseT& event, FnT&& fn)
    -> std::enable_if_t<_impl::needs_downcast<BaseT, FnT>
                        && std::is_void_v<fn_return_t<FnT>>>
{
    using event_type = fn_arg_t<FnT>;
    if (is<std::decay_t<event_type>>(event))
        fn(static_cast<event_type>(event));
}

// A trivial type-specific "switch" for non-void functions with an optional
// default value; non-voidness is guarded by defaultValue type
template <class BaseT, typename FnT>
inline auto switchOnType(const BaseT& event, FnT&& fn,
                         fn_return_t<FnT>&& defaultValue = {})
    -> std::enable_if_t<_impl::needs_downcast<BaseT, FnT>, fn_return_t<FnT>>
{
    using event_type = fn_arg_t<FnT>;
    if (is<std::decay_t<event_type>>(event))
        return fn(static_cast<event_type>(event));
    return std::move(defaultValue);
}

// A switch for a chain of 2 or more functions
template <class BaseT, typename FnT1, typename FnT2, typename... FnTs>
inline std::common_type_t<fn_return_t<FnT1>, fn_return_t<FnT2>>
switchOnType(const BaseT& event, FnT1&& fn1, FnT2&& fn2, FnTs&&... fns)
{
    using event_type1 = fn_arg_t<FnT1>;
    if (is<std::decay_t<event_type1>>(event))
        return fn1(static_cast<event_type1&>(event));
    return switchOnType(event, std::forward<FnT2>(fn2),
                        std::forward<FnTs>(fns)...);
}

template <class BaseT, typename... FnTs>
[[deprecated("The new name for visit() is switchOnType()")]] //
inline std::common_type_t<fn_return_t<FnTs>...>
visit(const BaseT& event, FnTs&&... fns)
{
    return switchOnType(event, std::forward<FnTs>(fns)...);
}

    // A facility overload that calls void-returning switchOnType() on each event
// over a range of event pointers
// TODO: replace with ranges::for_each once all standard libraries have it
template <typename RangeT, typename... FnTs>
inline auto visitEach(RangeT&& events, FnTs&&... fns)
    -> std::enable_if_t<std::is_void_v<
        decltype(switchOnType(**begin(events), std::forward<FnTs>(fns)...))>>
{
    for (auto&& evtPtr: events)
        switchOnType(*evtPtr, std::forward<FnTs>(fns)...);
}
} // namespace Quotient
Q_DECLARE_METATYPE(Quotient::Event*)
Q_DECLARE_METATYPE(const Quotient::Event*)
