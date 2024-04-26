// SPDX-FileCopyrightText: 2021 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "events/stateevent.h"

#include <QtCore/QHash>

namespace Quotient {

class Room;

// NB: Both concepts below expect EvT::needsStateKey to exist so you can't
// express one via negation of the other (there's still an invalid case of
// a non-state event where needsStateKey is not even defined).

template <typename FnT, class EvT = std::decay_t<fn_arg_t<FnT>>>
concept Keyed_State_Fn = EvT::needsStateKey;

template <typename FnT, class EvT = std::decay_t<fn_arg_t<FnT>>>
concept Keyless_State_Fn = !EvT::needsStateKey;

class QUOTIENT_API RoomStateView
    : private QHash<StateEventKey, const StateEvent*> {
    Q_GADGET
public:
    const QHash<StateEventKey, const StateEvent*>& events() const
    {
        return *this;
    }

    //! \brief Get a state event with the given event type and state key
    //! \return A state event corresponding to the pair of event type
    //!         \p evtType and state key \p stateKey, or `nullptr` if there's
    //!         no such \p evtType / \p stateKey combination in the current
    //!         state.
    //! \warning The returned value is not guaranteed to be non-`nullptr`; you
    //!          MUST check it before using or use other methods of this class
    //!          such as query() and content() to access state safely.
    //! \sa content, contentJson, query
    const StateEvent* get(const QString& evtType,
                          const QString& stateKey = {}) const;

    //! \brief Get a state event with the given event type and state key
    //!
    //! This is a typesafe overload that accepts a C++ event type instead of
    //! its Matrix name. It is only defined for events with state key (i.e.,
    //! derived from KeyedStateEvent).
    template <Keyed_State_Event EvT>
    const EvT* get(const QString& stateKey = {}) const
    {
        if (const auto* evt = get(EvT::TypeId, stateKey)) {
            Q_ASSERT(evt->matrixType() == EvT::TypeId
                     && evt->stateKey() == stateKey);
            return eventCast<const EvT>(evt);
        }
        return nullptr;
    }

    //! \brief Get a state event with the given event type
    //!
    //! This is a typesafe overload that accepts a C++ event type instead of
    //! its Matrix name. This overload only defined for events that do not use
    //! state key (i.e., derived from KeylessStateEvent).
    template <Keyless_State_Event EvT>
    const EvT* get() const
    {
        if (const auto* evt = get(EvT::TypeId)) {
            Q_ASSERT(evt->matrixType() == EvT::TypeId);
            return eventCast<const EvT>(evt);
        }
        return nullptr;
    }

    using QHash::contains;

    bool contains(const QString& evtType, const QString& stateKey = {}) const;

    template <Keyed_State_Event EvT>
    bool contains(const QString& stateKey = {}) const
    {
        return contains(EvT::TypeId, stateKey);
    }

    template <Keyless_State_Event EvT>
    bool contains() const
    {
        return contains(EvT::TypeId);
    }

    template <Keyed_State_Event EvT>
    auto content(const QString& stateKey,
                 typename EvT::content_type defaultValue = {}) const
    {
        // EventBase<>::content is special in that it returns a const-ref,
        // and lift() inside queryOr() can't wrap that in a temporary optional.
        if (const auto evt = get<EvT>(stateKey))
            return evt->content();
        return std::move(defaultValue);
    }

    template <Keyless_State_Event EvT>
    auto content(typename EvT::content_type defaultValue = {}) const
    {
        // Same as above
        if (const auto evt = get<EvT>())
            return evt->content();
        return defaultValue;
    }

    //! \brief Get the content of the current state event with the given
    //!        event type and state key
    //! \return An empty object if there's no event in the current state with
    //!         this event type and state key; the contents of the event
    //!         <tt>'content'</tt> object otherwise
    Q_INVOKABLE QJsonObject contentJson(const QString& evtType,
                                        const QString& stateKey = {}) const;

    //! \brief Get all state events in the room of a certain type.
    //!
    //! This method returns all known state events that have occured in
    //! the room of the given type.
    const QVector<const StateEvent*> eventsOfType(const QString& evtType) const;

    //! \brief Run a function on a state event with the given type and key
    //!
    //! Use this overload when there's no predefined event type or the event
    //! type is unknown at compile time.
    //! \return an optional with the result of the function call, or std::nullopt if the event
    //!         is not found or \p fn fails
    template <typename FnT>
    auto query(const QString& evtType, const QString& stateKey, FnT&& fn) const
    {
        return lift(std::forward<FnT>(fn), get(evtType, stateKey));
    }

    //! \brief Run a function on a state event with the given type and key
    //!
    //! This is an overload for keyed state events (those that have
    //! `needsStateKey == true`) with type defined at compile time.
    //! \return an optional with the result of the function call, or std::nullopt if the event
    //!         is not found or \p fn fails
    template <Keyed_State_Fn FnT>
    auto query(const QString& stateKey, FnT&& fn) const
    {
        using EventT = std::decay_t<fn_arg_t<FnT>>;
        return lift(std::forward<FnT>(fn), get<EventT>(stateKey));
    }

    //! \brief Run a function on a keyless state event with the given type
    //!
    //! This is an overload for keyless state events (those having
    //! `needsStateKey == false`) with type defined at compile time.
    //! \return an optional with the result of the function call, or std::nullopt if the event
    //!         is not found or \p fn fails
    template <Keyless_State_Fn FnT>
    auto query(FnT&& fn) const
    {
        using EventT = std::decay_t<fn_arg_t<FnT>>;
        return lift(std::forward<FnT>(fn), get<EventT>());
    }

    //! \brief Same as query() but with a fallback value
    //!
    //! This is a shortcut for `query().value_or()`, passing respective
    //! arguments to the respective functions. This is an overload for the case
    //! when the event type cannot be fixed at compile time.
    //! \return the result of \p fn execution, or \p fallback if the requested
    //!         event doesn't exist or the function fails
    template <typename FnT, typename FallbackT>
    auto queryOr(const QString& evtType, const QString& stateKey, FnT&& fn,
                 FallbackT&& fallback) const
    {
        return query(evtType, stateKey, std::forward<FnT>(fn))
            .value_or(std::forward<FallbackT>(fallback));
    }

    //! \brief Same as query() but with a fallback value
    //!
    //! This is a shortcut for `query().value_or()`, passing respective
    //! arguments to the respective functions. This is an overload for the case
    //! when the event type cannot be fixed at compile time.
    //! \return the result of \p fn execution, or \p fallback if the requested
    //!         event doesn't exist or the function fails
    template <typename FnT, typename FallbackT>
    auto queryOr(const QString& stateKey, FnT&& fn, FallbackT&& fallback) const
    {
        return query(stateKey, std::forward<FnT>(fn))
            .value_or(std::forward<FallbackT>(fallback));
    }

    //! \brief Same as query() but with a fallback value
    //!
    //! This is a shortcut for `query().value_or()`, passing respective
    //! arguments to the respective functions. This is an overload for the case
    //! when the event type cannot be fixed at compile time.
    //! \return the result of \p fn execution, or \p fallback if the requested
    //!         event doesn't exist or the function fails
    template <typename FnT, typename FallbackT>
    auto queryOr(FnT&& fn, FallbackT&& fallback) const
    {
        return query(std::forward<FnT>(fn))
            .value_or(std::forward<FallbackT>(fallback));
    }

private:
    friend class Room;
};
} // namespace Quotient
