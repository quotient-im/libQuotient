// SPDX-FileCopyrightText: 2021 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "events/stateevent.h"

#include <QtCore/QHash>

namespace Quotient {

class Room;

class RoomStateView : private QHash<StateEventKey, const StateEventBase*> {
    Q_GADGET
public:
    const QHash<StateEventKey, const StateEventBase*>& events() const
    {
        return *this;
    }

    //! \brief Get a state event with the given event type and state key
    //! \return A state event corresponding to the pair of event type
    //!         \p evtType and state key \p stateKey, or nullptr if there's
    //!         no such \p evtType / \p stateKey combination in the current
    //!         state.
    //! \warning In libQuotient 0.7 the return type changed to an OmittableCref
    //!          which is effectively a nullable const reference wrapper. You
    //!          have to check that it has_value() before using. Alternatively
    //!          you can now use queryCurrentState() to access state safely.
    //! \sa getCurrentStateContentJson
    const StateEventBase* get(const QString& evtType,
                              const QString& stateKey = {}) const;

    //! \brief Get a state event with the given event type and state key
    //!
    //! This is a typesafe overload that accepts a C++ event type instead of
    //! its Matrix name.
    //! \warning In libQuotient 0.7 the return type changed to an Omittable with
    //!          a reference wrapper inside - you have to check that it
    //!          has_value() before using. Alternatively you can now use
    //!          queryCurrentState() to access state safely.
    template <typename EvT>
    const EvT* get(const QString& stateKey = {}) const
    {
        static_assert(std::is_base_of_v<StateEventBase, EvT>);
        if (const auto* evt = get(EvT::matrixTypeId(), stateKey)) {
            Q_ASSERT(evt->matrixType() == EvT::matrixTypeId()
                     && evt->stateKey() == stateKey);
            return eventCast<const EvT>(evt);
        }
        return nullptr;
    }

    using QHash::contains;

    bool contains(const QString& evtType, const QString& stateKey = {}) const;

    template <typename EvT>
    bool contains(const QString& stateKey = {}) const
    {
        return contains(EvT::matrixTypeId(), stateKey);
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
    const QVector<const StateEventBase*>
    eventsOfType(const QString& evtType) const;

    template <typename FnT>
    auto query(const QString& evtType, const QString& stateKey, FnT&& fn) const
    {
        return lift(std::forward<FnT>(fn), get(evtType, stateKey));
    }

    template <typename FnT>
    auto query(const QString& stateKey, FnT&& fn) const
    {
        using EventT = std::decay_t<fn_arg_t<FnT>>;
        static_assert(std::is_base_of_v<StateEventBase, EventT>);
        return lift(std::forward<FnT>(fn), get<EventT>(stateKey));
    }

    template <typename FnT, typename FallbackT>
    auto queryOr(const QString& evtType, const QString& stateKey, FnT&& fn,
                 FallbackT&& fallback) const
    {
        return lift(std::forward<FnT>(fn), get(evtType, stateKey))
            .value_or(std::forward<FallbackT>(fallback));
    }

    template <typename FnT>
    auto query(FnT&& fn) const
    {
        return query({}, std::forward<FnT>(fn));
    }

    template <typename FnT, typename FallbackT>
    auto queryOr(const QString& stateKey, FnT&& fn, FallbackT&& fallback) const
    {
        using EventT = std::decay_t<fn_arg_t<FnT>>;
        static_assert(std::is_base_of_v<StateEventBase, EventT>);
        return lift(std::forward<FnT>(fn), get<EventT>(stateKey))
            .value_or(std::forward<FallbackT>(fallback));
    }

    template <typename FnT, typename FallbackT>
    auto queryOr(FnT&& fn, FallbackT&& fallback) const
    {
        return queryOr({}, std::forward<FnT>(fn),
                       std::forward<FallbackT>(fallback));
    }

private:
    friend class Room;
};
} // namespace Quotient
