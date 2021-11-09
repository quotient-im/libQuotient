// SPDX-FileCopyrightText: 2019 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <qobjectdefs.h>

#include <array>

// See https://bugreports.qt.io/browse/QTBUG-82295 - despite the comment that
// Q_FLAG[_NS] "should" be applied to the enum only, Qt doesn't allow to wrap
// a flag type into a QVariant then. The macros below define Q_FLAG_NS and on
// top of that add a part of Q_ENUM() that enables the metatype data but goes
// under the moc radar to avoid double registration of the same data in the map
// defined in moc_*.cpp
#define QUO_DECLARE_FLAGS(Flags, Enum) \
    Q_DECLARE_FLAGS(Flags, Enum)       \
    Q_ENUM_IMPL(Enum)                  \
    Q_FLAG(Flags)

#define QUO_DECLARE_FLAGS_NS(Flags, Enum) \
    Q_DECLARE_FLAGS(Flags, Enum)          \
    Q_ENUM_NS_IMPL(Enum)                  \
    Q_FLAG_NS(Flags)

#define DECL_DEPRECATED_ENUMERATOR(Deprecated, Recommended) \
    Deprecated Q_DECL_ENUMERATOR_DEPRECATED_X("Use " #Recommended) = Recommended

namespace Quotient {
Q_NAMESPACE

// std::array {} needs explicit template parameters on macOS because
// Apple stdlib doesn't have deduction guides for std::array. C++20 has
// to_array() but that can't be borrowed, this time because of MSVC:
// https://developercommunity.visualstudio.com/t/vc-ice-p1-initc-line-3652-from-stdto-array/1464038
// Therefore a simpler (but also slightly more wobbly - it resolves the element
// type using std::common_type<>) make_array facility is implemented here.
template <typename... Ts>
constexpr auto make_array(Ts&&... items)
{
    return std::array<std::common_type_t<Ts...>, sizeof...(items)>(
        { std::forward<Ts>(items)... });
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
QUO_DECLARE_FLAGS_NS(MembershipMask, Membership)

constexpr inline auto MembershipStrings = make_array(
    // The order MUST be the same as the order in the original enum
    "join", "leave", "invite", "knock", "ban");

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
QUO_DECLARE_FLAGS_NS(JoinStates, JoinState)

constexpr inline auto JoinStateStrings = make_array(
    MembershipStrings[0], MembershipStrings[1], MembershipStrings[2],
    MembershipStrings[3] /* same as MembershipStrings, sans "ban" */
);

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

enum RoomType {
    Space,
    Undefined,
};
Q_ENUM_NS(RoomType)

constexpr inline auto RoomTypeStrings = make_array(
    "m.space"
);

} // namespace Quotient
Q_DECLARE_OPERATORS_FOR_FLAGS(Quotient::MembershipMask)
Q_DECLARE_OPERATORS_FOR_FLAGS(Quotient::JoinStates)

class QDebug;
QDebug operator<<(QDebug dbg, Quotient::Membership m);
QDebug operator<<(QDebug dbg, Quotient::MembershipMask m);
QDebug operator<<(QDebug dbg, Quotient::JoinState js);
QDebug operator<<(QDebug dbg, Quotient::JoinStates js);
