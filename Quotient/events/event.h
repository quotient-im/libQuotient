// SPDX-FileCopyrightText: 2016 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "single_key_value.h"

#include <Quotient/converters.h>
#include <Quotient/function_traits.h>

#include <span>

namespace Quotient {
// === event_ptr_tt<> and basic type casting facilities ===

template <typename EventT>
using event_ptr_tt = std::unique_ptr<EventT>;

// === Standard Matrix key names ===

constexpr inline auto TypeKey = "type"_L1;
constexpr inline auto BodyKey = "body"_L1;
constexpr inline auto ContentKey = "content"_L1;
constexpr inline auto SenderKey = "sender"_L1;
constexpr inline auto UnsignedKey = "unsigned"_L1;

using event_type_t = QLatin1String;

// === EventMetaType ===

class Event;

template <typename EventT, typename BaseEventT = Event>
concept EventClass = std::derived_from<EventT, BaseEventT>;

template <EventClass EventT>
bool is(const Event& e);

//! \brief The base class for event metatypes
//!
//! You should not normally have to use this directly, unless you need to devise
//! a whole new kind of event metatypes.
class QUOTIENT_API AbstractEventMetaType {
public:
    // The public fields here are const and are not to be changeable anyway.
    // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
    const char* const className; ///< C++ class name this metatype is for
    const AbstractEventMetaType* const baseType;
    const event_type_t matrixId;
    // NOLINTEND(misc-non-private-member-variables-in-classes)

    explicit AbstractEventMetaType(const char* className,
                                   AbstractEventMetaType* nearestBase = nullptr,
                                   const char* matrixId = nullptr)
        : className(className), baseType(nearestBase), matrixId(matrixId)
    {
        if (nearestBase)
            nearestBase->addDerived(this);
    }

    void addDerived(const AbstractEventMetaType* newType);
    auto derivedTypes() const { return std::span(_derivedTypes); }

    virtual ~AbstractEventMetaType() = default;

protected:
    // Allow template specialisations to call into one another
    template <class EventT>
    friend class EventMetaType;

    // The returned value indicates whether a generic object has to be created
    // on the top level when `event` is empty, instead of returning nullptr
    virtual bool doLoadFrom(const QJsonObject& fullJson, const QString& type,
                            Event*& event) const = 0;

private:
    std::vector<const AbstractEventMetaType*> _derivedTypes{};
    Q_DISABLE_COPY_MOVE(AbstractEventMetaType)
};

// Any event metatype is unique (note Q_DISABLE_COPY_MOVE above) so can be
// identified by its address
inline bool operator==(const AbstractEventMetaType& lhs,
                       const AbstractEventMetaType& rhs)
{
    return &lhs == &rhs;
}

//! \brief A family of event meta-types to load and match events
//!
//! TL;DR for the loadFrom() story:
//! - for base event types, use QUO_BASE_EVENT and, if you have additional
//!   validation (e.g., JSON has to contain a certain key - see StateEvent
//!   for a real example), define it in the static EventT::isValid() member
//!   function accepting QJsonObject and returning bool.
//! - for leaf (specific) event types - simply use QUO_EVENT and it will do
//!   everything necessary, including the TypeId definition.
//! \sa QUO_EVENT, QUO_BASE_EVENT
template <class EventT>
class QUOTIENT_API EventMetaType : public AbstractEventMetaType {
    // Above: can't constrain EventT to be EventClass because it's incomplete
    // at the point of EventMetaType<EventT> instantiation.
public:
    using AbstractEventMetaType::AbstractEventMetaType;

    //! \brief Try to load an event from JSON, with dynamic type resolution
    //!
    //! The generic logic defined in this class template and invoked applies to
    //! all event types defined in the library and boils down to the following:
    //! 1.
    //!    a. If EventT has TypeId defined (which normally is a case of
    //!       all leaf - specific - event types, via QUO_EVENT macro) and
    //!       \p type doesn't exactly match it, nullptr is immediately returned.
    //!    b. In absence of TypeId, an event type is assumed to be a base;
    //!       its derivedTypes are examined, and this algorithm is applied
    //!       recursively on each.
    //! 2. Optional validation: if EventT (or, due to the way inheritance works,
    //!    any of its base event types) has a static isValid() predicate and
    //!    the event JSON does not satisfy it, nullptr is immediately returned
    //!    to the upper level or to the loadFrom() caller. This is how existence
    //!    of `state_key` is checked in any type derived from StateEvent.
    //! 3. If step 1b above returned non-nullptr, immediately return it.
    //! 4.
    //!    a. If EventT::isValid() or EventT::TypeId (either, or both) exist and
    //!       are satisfied (see steps 1a and 2 above), an object of this type
    //!       is created from the passed JSON and returned. In case of a base
    //!       event type, this will be a generic (aka "unknown") event.
    //!    b. If neither exists, a generic event is only created and returned
    //!       when on the top level (i.e., outside of recursion into
    //!       derivedTypes); lower levels return nullptr instead and the type
    //!       lookup continues. The latter is a case of a derived base event
    //!       metatype (e.g. RoomEvent) called from its base event metatype
    //!       (i.e., Event). If no matching type derived from RoomEvent is found,
    //!       the nested lookup returns nullptr rather than a generic RoomEvent,
    //!       so that other types derived from Event could be examined.
    event_ptr_tt<EventT> loadFrom(const QJsonObject& fullJson,
                                  const QString& type) const
    {
        Event* event = nullptr;
        const bool goodEnough = doLoadFrom(fullJson, type, event);
        if (!event && goodEnough)
            return event_ptr_tt<EventT>{ new EventT(fullJson) };
        return event_ptr_tt<EventT>{ static_cast<EventT*>(event) };
    }

private:
    bool doLoadFrom(const QJsonObject& fullJson, const QString& type,
                    Event*& event) const override
    {
        if constexpr (requires { EventT::TypeId; }) {
            if (EventT::TypeId != type)
                return false;
        } else {
            for (const auto& p : _derivedTypes) {
                p->doLoadFrom(fullJson, type, event);
                if (event) {
                    Q_ASSERT(is<EventT>(*event));
                    return false;
                }
            }
        }
        if constexpr (requires { EventT::isValid; }) {
            if (!EventT::isValid(fullJson))
                return false;
        } else if constexpr (!requires { EventT::TypeId; })
            return true; // Create a generic event object if on the top level
        event = new EventT(fullJson);
        return false;
    }
};

// === Event creation facilities ===

//! \brief Create an event of arbitrary type from its arguments
//!
//! This should not be used to load events from JSON - use loadEvent() for that.
//! \sa loadEvent
template <EventClass EventT, typename... ArgTs>
inline event_ptr_tt<EventT> makeEvent(ArgTs&&... args)
{
    return std::make_unique<EventT>(std::forward<ArgTs>(args)...);
}

template <EventClass EventT>
constexpr const auto& mostSpecificMetaType()
{
    if constexpr (requires { EventT::MetaType; })
        return EventT::MetaType;
    else
        return EventT::BaseMetaType;
}

//! \brief Create an event with proper type from a JSON object
//!
//! Use this factory template to detect the type from the JSON object
//! contents (the detected event type should derive from the template
//! parameter type) and create an event object of that type.
template <EventClass EventT>
inline event_ptr_tt<EventT> loadEvent(const QJsonObject& fullJson)
{
    return mostSpecificMetaType<EventT>().loadFrom(
        fullJson, fullJson[TypeKey].toString());
}

//! \brief Create an event from a type string and content JSON
//!
//! Use this template to resolve the C++ type from the Matrix type string in
//! \p matrixType and create an event of that type by passing all parameters
//! to EventT::basicJson().
template <EventClass EventT>
inline event_ptr_tt<EventT> loadEvent(const QString& matrixType,
                                      const auto&... otherBasicJsonParams)
{
    return mostSpecificMetaType<EventT>().loadFrom(
        EventT::basicJson(matrixType, otherBasicJsonParams...), matrixType);
}

template <EventClass EventT>
struct JsonConverter<event_ptr_tt<EventT>>
    : JsonObjectUnpacker<event_ptr_tt<EventT>> {
    // No dump() to avoid any ambiguity on whether a given export to JSON uses
    // fullJson() or only contentJson()
    using JsonObjectUnpacker<event_ptr_tt<EventT>>::load;
    static auto load(const QJsonObject& jo)
    {
        return loadEvent<EventT>(jo);
    }
};

// === Event ===

class QUOTIENT_API Event {
public:
    static inline EventMetaType<Event> BaseMetaType { "Event" };
    virtual const AbstractEventMetaType& metaType() const
    {
        return BaseMetaType;
    }

    Q_DISABLE_COPY(Event)
    Event(Event&&) noexcept = default;
    Event& operator=(Event&&) = delete;
    virtual ~Event();

    /// Make a minimal correct Matrix event JSON
    static QJsonObject basicJson(const QString& matrixType,
                                 const QJsonObject& content)
    {
        return { { TypeKey, matrixType }, { ContentKey, content } };
    }

    //! \brief Exact Matrix type stored in JSON
    QString matrixType() const;

    template <EventClass EventT>
    bool is() const
    {
        return Quotient::is<EventT>(*this);
    }

    //! \brief Apply one of the visitors based on the actual event type
    //!
    //! This function uses function_traits template and is() to find the first
    //! of the passed visitor invocables that can be called with this event
    //! object, downcasting `*this` in a type-safe way to the most specific type
    //! accepted by the visitor. Without this function, you can still write
    //! a stack of, for example,
    //! `(else) if (const auto* evtPtr = eventCast<...>(baseEvtPtr))`
    //! blocks but switchType() provides a more concise and isolating syntax:
    //! there's no `else` or trailing `return/break` to forget, for one.
    //! The visitors have to all return the same type (possibly void).
    //! Here's how you might use this function:
    //! \code
    //! RoomEventPtr eptr = /* get the event pointer from somewhere */;
    //! const auto result = eptr->switchOnType(
    //!     [](const RoomMemberEvent& memberEvent) {
    //!         // Do what's needed if eptr points to a RoomMemberEvent
    //!         return 1;
    //!     },
    //!     [](const CallEvent& callEvent) {
    //!         // Do what's needed if eptr points to a CallEvent or any
    //!         // class derived from it
    //!         return 2;
    //!     },
    //!     3); /* the default value to return if nothing above matched */
    //! \endcode
    //! As the example shows, the last parameter can optionally be
    //! a plain returned value instead of a visitor.
    template <typename... VisitorTs>
    auto switchOnType(VisitorTs&&... visitors) const;

    const QJsonObject& fullJson() const { return _json; }

    // According to the CS API spec, every event also has
    // a "content" object; but since its structure is different for
    // different types, we're implementing it per-event type.

    // NB: const return types below are meant to catch accidental attempts
    //     to change event JSON (e.g., consider contentJson()["inexistentKey"]).

    const QJsonObject contentJson() const;

    //! \brief Get a part of the content object, assuming a given type
    //!
    //! This retrieves the value under `content.<key>` from the event JSON and
    //! then converts it to \p T using fromJson().
    //! \sa contentJson, fromJson
    template <typename T, typename KeyT>
    const T contentPart(KeyT&& key) const
    {
        return fromJson<T>(contentJson()[std::forward<KeyT>(key)]);
    }

    const QJsonObject unsignedJson() const;

    //! \brief Get a part of the unsigned object, assuming a given type
    //!
    //! This retrieves the value under `unsigned.<key>` from the event JSON and
    //! then converts it to \p T using fromJson().
    //! \sa unsignedJson, fromJson
    template <typename T, typename KeyT>
    const T unsignedPart(KeyT&& key) const
    {
        return fromJson<T>(unsignedJson()[std::forward<KeyT>(key)]);
    }

    friend QUOTIENT_API QDebug operator<<(QDebug dbg, const Event& e)
    {
        const QDebugStateSaver _dss { dbg };
        dbg.noquote().nospace()
            << e.matrixType() << '(' << e.metaType().className << "): ";
        e.dumpTo(dbg);
        return dbg;
    }

#if Quotient_VERSION_MAJOR == 0 && Quotient_VERSION_MINOR <= 9
    [[deprecated("isStateEvent() has moved to RoomEvent")]] bool isStateEvent() const;
#endif

protected:
    friend class EventMetaType<Event>; // To access the below constructor

    explicit Event(const QJsonObject& json);

    QJsonObject& editJson() { return _json; }
    virtual void dumpTo(QDebug dbg) const;

private:
    QJsonObject _json;
};
using EventPtr = event_ptr_tt<Event>;

template <EventClass EventT>
using EventsArray = std::vector<event_ptr_tt<EventT>>;
using Events = EventsArray<Event>;

// === Facilities for event class definitions ===

//! \brief A template base class to derive your event type from
//!
//! This simple class template generates commonly used event constructor
//! signatures and the content() method with the appropriate return type.
//! The generic version here is only used with non-trivial \p ContentT (if you
//! don't need to create an event from its content structure, just go and derive
//! straight from the respective \p EventBaseT instead of using EventTemplate);
//! specialisations may override that and provide useful semantics even without
//! \p ContentT (see EventTemplate<CallEvent>, e.g.).
//!
//! The template uses CRTP to pick the event type id from the actual class;
//! it will fail to compile if \p EventT doesn't provide TypeId. It also uses
//! the base event type's basicJson(); if you need extra keys to be inserted
//! you may want to bypass this template as writing the code to that effect in
//! your class will likely be clearer and more concise.
//! \sa https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern
//! \sa DEFINE_SIMPLE_EVENT
template <typename EventT, EventClass BaseEventT, typename ContentT = void>
class EventTemplate : public BaseEventT {
    // Above: can't constrain EventT to be EventClass because it's incomplete
    // by CRTP definition.
public:
    static_assert(
        !std::is_same_v<ContentT, void>,
        "If you see this, you tried to use EventTemplate with the default"
        " ContentT type, which is void. This default is only used with explicit"
        " specialisations (see CallEvent, e.g.). Otherwise, if you don't intend"
        " to use the content part of EventTemplate then you don't need"
        " EventTemplate; just use the base event class directly");
    using content_type = ContentT;

    explicit EventTemplate(const QJsonObject& json)
        : BaseEventT(json)
    {}
    explicit EventTemplate(const ContentT& c)
        : BaseEventT(EventT::basicJson(EventT::TypeId, toJson(c)))
    {}

    ContentT content() const { return fromJson<ContentT>(this->contentJson()); }
};

//! \brief Supply event metatype information in base event types
//!
//! Use this macro in a public section of your base event class to provide
//! type identity and enable dynamic loading of generic events of that type.
//! Do _not_ add this macro if your class is an intermediate wrapper and is not
//! supposed to be instantiated on its own. Provides BaseMetaType static field
//! initialised by parameters passed to the macro, and a metaType() override
//! pointing to that BaseMetaType.
//! \sa EventMetaType
#define QUO_BASE_EVENT(CppType_, BaseCppType_, ...)                       \
    friend class EventMetaType<CppType_>;                                 \
    static inline EventMetaType<CppType_> BaseMetaType{                   \
        #CppType_, &BaseCppType_::BaseMetaType __VA_OPT__(, ) __VA_ARGS__ \
    };                                                                    \
    static_assert(&CppType_::BaseMetaType == &BaseMetaType,               \
                  #CppType_ " is wrong here - check for copy-pasta");     \
    const AbstractEventMetaType& metaType() const override                \
    {                                                                     \
        return BaseMetaType;                                              \
    }                                                                     \
    // End of macro

//! \brief Supply event metatype information in (specific) event types
//!
//! Use this macro in a public section of your event class to provide type
//! identity and enable dynamic loading of generic events of that type.
//! Do _not_ use this macro if your class is an intermediate wrapper and is not
//! supposed to be instantiated on its own. Provides MetaType static field
//! initialised as described below; a metaType() override pointing to it; and
//! the TypeId static field that is equal to MetaType.matrixId.
//!
//! The first two macro parameters are used as the first two EventMetaType
//! constructor parameters; the third EventMetaType parameter is always
//! BaseMetaType; and additional base types can be passed in extra macro
//! parameters if you need to include the same event type in more than one
//! event factory hierarchy (e.g., EncryptedEvent).
//! \sa EventMetaType
#define QUO_EVENT(CppType_, MatrixType_)                                 \
    friend class EventMetaType<CppType_>;                                \
    static inline const EventMetaType<CppType_> MetaType{ #CppType_,     \
                                                          &BaseMetaType, \
                                                          MatrixType_ }; \
    static_assert(&CppType_::MetaType == &MetaType,                      \
                  #CppType_ " is wrong here - check for copy-pasta");    \
    static inline const auto& TypeId = MetaType.matrixId;                \
    const AbstractEventMetaType& metaType() const override               \
    {                                                                    \
        return MetaType;                                                 \
    }                                                                    \
    // End of macro

#define QUO_CONTENT_GETTER_X(PartType_, PartName_, JsonKey_) \
    PartType_ PartName_() const                              \
    {                                                        \
        static const auto PartName_##JsonKey = JsonKey_;     \
        return contentPart<PartType_>(PartName_##JsonKey);   \
    }

//! \brief Define an inline method obtaining a content part
//!
//! This macro adds a const method that extracts a JSON value at the key
//! <tt>toSnakeCase(PartName_)</tt> (sic) and converts it to the type
//! \p PartType_. Effectively, the generated method is an equivalent of
//! \code
//! contentPart<PartType_>(Quotient::toSnakeCase(#PartName_##_L1));
//! \endcode
#define QUO_CONTENT_GETTER(PartType_, PartName_) \
    QUO_CONTENT_GETTER_X(PartType_, PartName_, toSnakeCase(#PartName_##_L1))

/// \brief Define a new event class with a single key-value pair in the content
///
/// This macro defines a new event class \p Name_ derived from \p Base_,
/// with Matrix event type \p TypeId_, providing a getter named \p GetterName_
/// for a single value of type \p ValueType_ inside the event content.
/// To retrieve the value the getter uses a JSON key name that corresponds to
/// its own (getter's) name but written in snake_case. \p GetterName_ must be
/// in camelCase, no quotes (an identifier, not a literal).
#define DEFINE_SIMPLE_EVENT(Name_, Base_, TypeId_, ValueType_, GetterName_, JsonKey_)      \
    constexpr inline auto Name_##ContentKey = JsonKey_##_L1;                               \
    class QUOTIENT_API Name_                                                               \
        : public ::Quotient::EventTemplate<                                                \
              Name_, Base_, EventContent::SingleKeyValue<ValueType_, Name_##ContentKey>> { \
    public:                                                                                \
        QUO_EVENT(Name_, TypeId_)                                                          \
        using value_type = ValueType_;                                                     \
        using EventTemplate::EventTemplate;                                                \
        QUO_CONTENT_GETTER_X(ValueType_, GetterName_, Name_##ContentKey)                   \
    };                                                                                     \
    // End of macro

// === is<>(), eventCast<>() and switchOnType<>() ===

template <EventClass EventT>
inline bool is(const Event& e)
{
    // Protect against accidental putting QUO_*EVENT to a private section
    static_assert(requires { &EventT::metaType; },
                  "Event class doesn't have a public metaType() override - "
                  "did you misplace the QUO_*EVENT macro?");
    if constexpr (requires { EventT::MetaType; }) {
        return &e.metaType() == &EventT::MetaType;
    } else {
        const auto* p = &e.metaType();
        do {
            if (p == &EventT::BaseMetaType)
                return true;
        } while ((p = p->baseType) != nullptr);
        return false;
    }
}

//! \brief Cast the event pointer down in a type-safe way
//!
//! Checks that the event \p eptr points to actually is of the requested type
//! and returns a (plain) pointer to the event downcast to that type. \p eptr
//! can be either "dumb" (BaseEventT*) or "smart" (`event_ptr_tt<>`). This
//! overload doesn't affect the event ownership - if the original pointer owns
//! the event it must outlive the downcast pointer to keep it from dangling.
template <EventClass EventT, typename BasePtrT>
inline auto eventCast(const BasePtrT& eptr)
    -> decltype(static_cast<EventT*>(std::to_address(eptr)))
{
    return eptr && is<std::decay_t<EventT>>(*eptr)
               ? static_cast<EventT*>(std::to_address(eptr))
               : nullptr;
}

//! \brief Cast the event pointer down in a type-safe way, with moving
//!
//! Checks that the event \p eptr points to actually is of the requested type;
//! if (and only if) it is, releases the pointer, downcasts it to the requested
//! event type and returns a new smart pointer wrapping the downcast one.
//! Unlike the non-moving eventCast() overload, this one only accepts a smart
//! pointer, and that smart pointer should be an rvalue (either a temporary,
//! or as a result of std::move()). The ownership, respectively, is transferred
//! to the new pointer; the original smart pointer is reset to nullptr, as is
//! normal for `unique_ptr<>::release()`.
//! \note If \p eptr's event type does not match \p EventT it retains ownership
//!       after calling this overload; if it is a temporary, this normally
//!       leads to the event getting deleted along with the end of
//!       the temporary's lifetime.
template <EventClass EventT, typename BaseEventT>
inline auto eventCast(event_ptr_tt<BaseEventT>&& eptr)
{
    return eptr && is<std::decay_t<EventT>>(*eptr)
               ? event_ptr_tt<EventT>(static_cast<EventT*>(eptr.release()))
               : nullptr;
}

namespace _impl {
    template <typename FnT, typename BaseT>
    concept Invocable_With_Downcast = requires
    {
        requires EventClass<BaseT>;
        std::is_base_of_v<BaseT, std::remove_cvref_t<fn_arg_t<FnT>>>;
    };
}

template <EventClass BaseT, typename TailT>
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

template <typename FnT1, typename... FnTs>
inline auto switchOnType(const EventClass auto& event, FnT1&& fn1, FnTs&&... fns)
{
    using event_type1 = fn_arg_t<FnT1>;
    if (is<std::decay_t<event_type1>>(event))
        return fn1(static_cast<event_type1>(event));
    return switchOnType(event, std::forward<FnTs>(fns)...);
}

template <typename... VisitorTs>
inline auto Event::switchOnType(VisitorTs&&... visitors) const
{
    return Quotient::switchOnType(*this,
                                  std::forward<VisitorTs>(visitors)...);
}

} // namespace Quotient
Q_DECLARE_METATYPE(Quotient::Event*)
Q_DECLARE_METATYPE(const Quotient::Event*)
