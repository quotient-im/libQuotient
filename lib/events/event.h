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

// === Event types ===

using event_type_t = QLatin1String;
using event_mtype_t = const char*;

class QUOTIENT_API EventTypeRegistry {
public:
    ~EventTypeRegistry() = default;

    [[deprecated("event_type_t is a string now, use it directly instead")]]
    static QString getMatrixType(event_type_t typeId);

private:
    EventTypeRegistry() = default;
    Q_DISABLE_COPY_MOVE(EventTypeRegistry)
};

template <typename EventT>
constexpr event_type_t typeId()
{
    return std::decay_t<EventT>::TypeId;
}

constexpr event_type_t UnknownEventTypeId = "?"_ls;
[[deprecated("Use UnknownEventTypeId")]]
constexpr event_type_t unknownEventTypeId() { return UnknownEventTypeId; }

// === Event creation facilities ===

//! Create an event of arbitrary type from its arguments
template <typename EventT, typename... ArgTs>
inline event_ptr_tt<EventT> makeEvent(ArgTs&&... args)
{
    return std::make_unique<EventT>(std::forward<ArgTs>(args)...);
}

namespace _impl {
    class QUOTIENT_API EventFactoryBase {
    public:
        EventFactoryBase(const EventFactoryBase&) = delete;

    protected: // This class is only to inherit from
        explicit EventFactoryBase(const char* name)
            : name(name)
        {}
        void logAddingMethod(event_type_t TypeId, size_t newSize);

    private:
        const char* const name;
    };
} // namespace _impl

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
class EventFactory : public _impl::EventFactoryBase {
private:
    using method_t = event_ptr_tt<BaseEventT> (*)(const QJsonObject&,
                                                  const QString&);
    std::vector<method_t> methods {};

    template <class EventT>
    static event_ptr_tt<BaseEventT> makeIfMatches(const QJsonObject& json,
                                                  const QString& matrixType)
    {
        // If your matrix event type is not all ASCII, it's your problem
        // (see https://github.com/matrix-org/matrix-doc/pull/2758)
        return EventT::TypeId == matrixType ? makeEvent<EventT>(json) : nullptr;
    }

public:
    explicit EventFactory(const char* fName)
        : EventFactoryBase { fName }
    {}

    //! \brief Add a method to create events of a given type
    //!
    //! Adds a standard factory method (makeIfMatches) for \p EventT so that
    //! event objects of this type can be created dynamically by loadEvent.
    //! The caller is responsible for ensuring this method is called only
    //! once per type.
    //! \sa loadEvent, Quotient::loadEvent
    template <class EventT>
    const auto& addMethod()
    {
        const auto m = &makeIfMatches<EventT>;
        const auto it = std::find(methods.cbegin(), methods.cend(), m);
        if (it != methods.cend())
            return *it;
        logAddingMethod(EventT::TypeId, methods.size() + 1);
        return methods.emplace_back(m);
    }

    auto loadEvent(const QJsonObject& json, const QString& matrixType)
    {
        for (const auto& f : methods)
            if (auto e = f(json, matrixType))
                return e;
        return makeEvent<BaseEventT>(UnknownEventTypeId, json);
    }
};

//! \brief Point of customisation to dynamically load events
//!
//! The default specialisation of this calls BaseEventT::factory.loadEvent()
//! and if that fails (i.e. returns nullptr) creates an unknown event of
//! BaseEventT. Other specialisations may reuse other factories, add validations
//! common to BaseEventT events, and so on.
template <class BaseEventT>
event_ptr_tt<BaseEventT> doLoadEvent(const QJsonObject& json,
                                     const QString& matrixType)
{
    return BaseEventT::factory.loadEvent(json, matrixType);
}

// === Event ===

class QUOTIENT_API Event {
public:
    using Type = event_type_t;
    static inline EventFactory<Event> factory { "Event" };

    explicit Event(Type type, const QJsonObject& json);
    explicit Event(Type type, event_mtype_t matrixType,
                   const QJsonObject& contentJson = {});
    Q_DISABLE_COPY(Event)
    Event(Event&&) = default;
    Event& operator=(Event&&) = delete;
    virtual ~Event();

    /// Make a minimal correct Matrix event JSON
    static QJsonObject basicJson(const QString& matrixType,
                                 const QJsonObject& content)
    {
        return { { TypeKey, matrixType }, { ContentKey, content } };
    }

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

//! \brief Define an inline method obtaining a content part
//!
//! This macro adds a const method that extracts a JSON value at the key
//! <tt>toSnakeCase(PartName_)</tt> (sic) and converts it to the type
//! \p PartType_. Effectively, the generated method is an equivalent of
//! \code
//! contentPart<PartType_>(Quotient::toSnakeCase(#PartName_##_ls));
//! \endcode
#define QUO_CONTENT_GETTER(PartType_, PartName_)                  \
    PartType_ PartName_() const                                   \
    {                                                             \
        static const auto JsonKey = toSnakeCase(#PartName_##_ls); \
        return contentPart<PartType_>(JsonKey);                   \
    }

// === Facilities for event class definitions ===

// This macro should be used in a public section of an event class to
// provide matrixTypeId() and typeId().
#define DEFINE_EVENT_TYPEID(Id_, Type_)                           \
    static constexpr event_type_t TypeId = Id_##_ls;              \
    [[deprecated("Use " #Type_ "::TypeId directly instead")]]     \
    static constexpr event_mtype_t matrixTypeId() { return Id_; } \
    [[deprecated("Use " #Type_ "::TypeId directly instead")]]     \
    static event_type_t typeId() { return TypeId; }               \
    // End of macro

// This macro should be put after an event class definition (in .h or .cpp)
// to enable its deserialisation from a /sync and other
// polymorphic event arrays
#define REGISTER_EVENT_TYPE(Type_)                                \
    [[maybe_unused]] inline const auto& factoryMethodFor##Type_ = \
        Type_::factory.addMethod<Type_>();                        \
    // End of macro

/// \brief Define a new event class with a single key-value pair in the content
///
/// This macro defines a new event class \p Name_ derived from \p Base_,
/// with Matrix event type \p TypeId_, providing a getter named \p GetterName_
/// for a single value of type \p ValueType_ inside the event content.
/// To retrieve the value the getter uses a JSON key name that corresponds to
/// its own (getter's) name but written in snake_case. \p GetterName_ must be
/// in camelCase, no quotes (an identifier, not a literal).
#define DEFINE_SIMPLE_EVENT(Name_, Base_, TypeId_, ValueType_, GetterName_)     \
    class QUOTIENT_API Name_ : public Base_ {                                   \
    public:                                                                     \
        using content_type = ValueType_;                                        \
        DEFINE_EVENT_TYPEID(TypeId_, Name_)                                     \
        explicit Name_(const QJsonObject& obj) : Base_(TypeId, obj) {}          \
        explicit Name_(const content_type& content)                             \
            : Name_(Base_::basicJson(TypeId, { { JsonKey, toJson(content) } })) \
        {}                                                                      \
        auto GetterName_() const                                                \
        {                                                                       \
            return contentPart<content_type>(JsonKey);                          \
        }                                                                       \
        static inline const auto JsonKey = toSnakeCase(#GetterName_##_ls);      \
    };                                                                          \
    REGISTER_EVENT_TYPE(Name_)                                                  \
    // End of macro

// === is<>(), eventCast<>() and switchOnType<>() ===

template <class EventT>
inline bool is(const Event& e)
{
    return e.type() == typeId<EventT>();
}

inline bool isUnknown(const Event& e)
{
    return e.type() == UnknownEventTypeId;
}

template <class EventT, typename BasePtrT>
inline auto eventCast(const BasePtrT& eptr)
    -> decltype(static_cast<EventT*>(&*eptr))
{
    Q_ASSERT(eptr);
    return is<std::decay_t<EventT>>(*eptr) ? static_cast<EventT*>(&*eptr)
                                           : nullptr;
}

namespace _impl {
    template <typename FnT, class BaseT>
    concept Invocable_With_Downcast =
        std::is_base_of_v<BaseT, std::remove_cvref_t<fn_arg_t<FnT>>>;
}

template <class BaseT, typename TailT>
inline auto switchOnType(const BaseT& event, TailT&& tail)
{
    if constexpr (std::is_invocable_v<TailT, BaseT>) {
        return tail(event);
    } else if constexpr (_impl::Invocable_With_Downcast<TailT, BaseT>) {
        using event_type = fn_arg_t<TailT>;
        if (is<std::decay_t<event_type>>(event))
            return tail(static_cast<event_type>(event));
        return std::invoke_result_t<TailT, event_type>(); // Default-constructed
    } else { // Treat it as a value to return
        return std::forward<TailT>(tail);
    }
}

template <class BaseT, typename FnT1, typename... FnTs>
inline auto switchOnType(const BaseT& event, FnT1&& fn1, FnTs&&... fns)
{
    using event_type1 = fn_arg_t<FnT1>;
    if (is<std::decay_t<event_type1>>(event))
        return fn1(static_cast<event_type1>(event));
    return switchOnType(event, std::forward<FnTs>(fns)...);
}

template <class BaseT, typename... FnTs>
[[deprecated("The new name for visit() is switchOnType()")]] //
inline auto visit(const BaseT& event, FnTs&&... fns)
{
    return switchOnType(event, std::forward<FnTs>(fns)...);
}

    // A facility overload that calls void-returning switchOnType() on each event
// over a range of event pointers
// TODO: replace with ranges::for_each once all standard libraries have it
template <typename RangeT, typename... FnTs>
inline auto visitEach(RangeT&& events, FnTs&&... fns)
    requires std::is_void_v<
        decltype(switchOnType(**begin(events), std::forward<FnTs>(fns)...))>
{
    for (auto&& evtPtr: events)
        switchOnType(*evtPtr, std::forward<FnTs>(fns)...);
}
} // namespace Quotient
Q_DECLARE_METATYPE(Quotient::Event*)
Q_DECLARE_METATYPE(const Quotient::Event*)
