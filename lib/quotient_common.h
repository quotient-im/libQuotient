// SPDX-FileCopyrightText: 2019 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <qobjectdefs.h>

#include <array>

namespace Quotient {
Q_NAMESPACE

namespace impl {
    template <class T, std::size_t N, std::size_t... I>
    constexpr std::array<std::remove_cv_t<T>, N>
        to_array_impl(T (&&a)[N], std::index_sequence<I...>)
    {
        return { {std::move(a[I])...} };
    }
}
// std::array {} needs explicit template parameters on macOS because
// Apple stdlib doesn't have deduction guides for std::array; to alleviate that,
// to_array() is borrowed from C++20 (thanks to cppreference for the possible
// implementation: https://en.cppreference.com/w/cpp/container/array/to_array)
template <typename T, size_t N>
constexpr auto to_array(T (&& items)[N])
{
    return impl::to_array_impl(std::move(items), std::make_index_sequence<N>{});
}

// TODO: code like this should be generated from the CS API definition

//! \brief Membership states
//!
//! These are used for member events. The names here are case-insensitively
//! equal to state names used on the wire.
//! \sa MemberEventContent, RoomMemberEvent
enum class Membership : unsigned int {
    // Specific power-of-2 values (1,2,4,...) are important here as syncdata.cpp
    // depends on that, as well as Join being the first in line
    Invalid = 0x0,
    Join = 0x1,
    Leave = 0x2,
    Invite = 0x4,
    Knock = 0x8,
    Ban = 0x10,
    Undefined = Invalid
};
Q_DECLARE_FLAGS(MembershipMask, Membership)
Q_FLAG_NS(MembershipMask)

constexpr inline auto MembershipStrings = to_array(
    // The order MUST be the same as the order in the original enum
    { "join", "leave", "invite", "knock", "ban" });

//! \brief Local user join-state names
//!
//! This represents a subset of Membership values that may arrive as the local
//! user's state grouping for the sync response.
//! \sa SyncData
enum class JoinState : std::underlying_type_t<Membership> {
    Invalid = std::underlying_type_t<Membership>(Membership::Invalid),
    Join = std::underlying_type_t<Membership>(Membership::Join),
    Leave = std::underlying_type_t<Membership>(Membership::Leave),
    Invite = std::underlying_type_t<Membership>(Membership::Invite),
    Knock = std::underlying_type_t<Membership>(Membership::Knock),
};
Q_DECLARE_FLAGS(JoinStates, JoinState)
Q_FLAG_NS(JoinStates)

constexpr inline auto JoinStateStrings = to_array({
    MembershipStrings[0], MembershipStrings[1], MembershipStrings[2],
    MembershipStrings[3] /* same as MembershipStrings, sans "ban" */
});

//! \brief Network job running policy flags
//!
//! So far only background/foreground flags are available.
//! \sa Connection::callApi, Connection::run
enum RunningPolicy { ForegroundRequest = 0x0, BackgroundRequest = 0x1 };

Q_ENUM_NS(RunningPolicy)

//! \brief The result of URI resolution using UriResolver
//! \sa UriResolver
enum UriResolveResult : short {
    StillResolving = -1,
    UriResolved = 0,
    CouldNotResolve,
    IncorrectAction,
    InvalidUri,
    NoAccount
};
Q_ENUM_NS(UriResolveResult)

} // namespace Quotient
Q_DECLARE_OPERATORS_FOR_FLAGS(Quotient::MembershipMask)
Q_DECLARE_OPERATORS_FOR_FLAGS(Quotient::JoinStates)

class QDebug;
QDebug operator<<(QDebug dbg, Quotient::Membership m);
QDebug operator<<(QDebug dbg, Quotient::MembershipMask m);
QDebug operator<<(QDebug dbg, Quotient::JoinState js);
QDebug operator<<(QDebug dbg, Quotient::JoinStates js);
