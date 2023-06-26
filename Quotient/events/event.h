// SPDX-FileCopyrightText: 2016 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "../converters.h"
#include "../function_traits.h"
#include "../metatype.h"
#include "single_key_value.h"

namespace Quotient {

// === Standard Matrix key names ===

constexpr inline auto TypeKey = "type"_ls;
constexpr inline auto BodyKey = "body"_ls;
constexpr inline auto ContentKey = "content"_ls;
constexpr inline auto EventIdKey = "event_id"_ls;
constexpr inline auto SenderKey = "sender"_ls;
constexpr inline auto RoomIdKey = "room_id"_ls;
constexpr inline auto UnsignedKey = "unsigned"_ls;
constexpr inline auto RedactedCauseKey = "redacted_because"_ls;
constexpr inline auto PrevContentKey = "prev_content"_ls;
constexpr inline auto StateKeyKey = "state_key"_ls;

// === Event creation facilities ===

class Event;

template <typename EventT, typename BaseEventT = Event>
concept EventClass = Loadable_Class<EventT, BaseEventT>;

template <typename EventT>
using event_ptr_tt = std::unique_ptr<EventT>;

//! \brief Create an event of arbitrary type from its arguments
//!
//! This should not be used to load events from JSON - use loadEvent() for that.
//! \sa loadEvent
template <EventClass EventT, typename... ArgTs>
inline event_ptr_tt<EventT> makeEvent(ArgTs&&... args)
{
    return std::make_unique<EventT>(std::forward<ArgTs>(args)...);
}

//! \brief Create an event with proper type from a JSON object
//!
//! Use this factory template to detect the type from the JSON object
//! contents (the detected event type should derive from the template
//! parameter type) and create an event object of that type.
template <EventClass EventT>
inline event_ptr_tt<EventT> loadEvent(const QJsonObject& fullJson)
{
    return mostSpecificMetaObject<EventT>().loadFrom(
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
    return mostSpecificMetaObject<EventT>().loadFrom(
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

class QUOTIENT_API Event : public LoadableBase {
public:
    QUO_BASE_LOADABLE(Event, LoadableBase)

    Q_DISABLE_COPY(Event)
    Event(Event&&) noexcept = default;
    Event& operator=(Event&&) = delete;
    ~Event() override = default;

    [[deprecated("Use metaObject() instead")]] const auto& metaType()
    {
        return metaObject();
    }

    /// Make a minimal correct Matrix event JSON
    static QJsonObject basicJson(const QString& matrixType,
                                 const QJsonObject& content)
    {
        return { { TypeKey, matrixType }, { ContentKey, content } };
    }

    //! \brief Exact Matrix type stored in JSON
    QString matrixType() const;

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
            << e.matrixType() << '(' << e.metaObject().className << "): ";
        e.dumpTo(dbg);
        return dbg;
    }

    // State events are quite special in Matrix; so isStateEvent() is here,
    // as an exception. For other base events, Event::is<>() and
    // Quotient::is<>() should be used; don't add is* methods here
    bool isStateEvent() const;

protected:
    friend class MetaType<Event>; // To access the below constructor

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

#define QUO_BASE_EVENT QUO_BASE_LOADABLE
#define QUO_EVENT QUO_LOADABLE

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
//! contentPart<PartType_>(Quotient::toSnakeCase(#PartName_##_ls));
//! \endcode
#define QUO_CONTENT_GETTER(PartType_, PartName_) \
    QUO_CONTENT_GETTER_X(PartType_, PartName_, toSnakeCase(#PartName_##_ls))

/// \brief Define a new event class with a single key-value pair in the content
///
/// This macro defines a new event class \p Name_ derived from \p Base_,
/// with Matrix event type \p TypeId_, providing a getter named \p GetterName_
/// for a single value of type \p ValueType_ inside the event content.
/// To retrieve the value the getter uses a JSON key name that corresponds to
/// its own (getter's) name but written in snake_case. \p GetterName_ must be
/// in camelCase, no quotes (an identifier, not a literal).
#define DEFINE_SIMPLE_EVENT(Name_, Base_, TypeId_, ValueType_, GetterName_,  \
                            JsonKey_)                                        \
    constexpr inline auto Name_##ContentKey = JsonKey_##_ls;                 \
    class QUOTIENT_API Name_                                                 \
        : public EventTemplate<                                              \
              Name_, Base_,                                                  \
              EventContent::SingleKeyValue<ValueType_, Name_##ContentKey>> { \
    public:                                                                  \
        QUO_EVENT(Name_, TypeId_)                                            \
        using value_type = ValueType_;                                       \
        using EventTemplate::EventTemplate;                                  \
        QUO_CONTENT_GETTER_X(ValueType_, GetterName_, Name_##ContentKey)     \
    };                                                                       \
    // End of macro

template <EventClass EventT, typename BasePtrT>
[[deprecated("Use downcast() instead")]]
inline auto eventCast(BasePtrT&& eptr)
{
    return downcast<EventT>(std::forward<BasePtrT>(eptr));
}

// A facility overload that calls void-returning switchOnType() on each event
// over a range of event pointers
// TODO: remove after 0.9
template <typename RangeT, typename... FnTs>
[[deprecated("Make a range-for and call switchOnType() from it directly")]]
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
