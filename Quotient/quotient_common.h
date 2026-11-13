// SPDX-FileCopyrightText: 2019 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "quotient_export.h"
#include "util.h" // IWYU pragma: keep - for Quotient::Literals::operator""_L1

#include <qobjectdefs.h>

#include <array>
#include <span>
#include <QtQmlIntegration/qqmlintegration.h>

//! \brief Quotient replacement for the Q_FLAG/Q_DECLARE_FLAGS combination
//!
//! Although the comment in QTBUG-82295 says that Q_FLAG[_NS] "should" be
//! applied to the enum type only, Qt then doesn't allow to wrap the
//! corresponding flag type (defined with Q_DECLARE_FLAGS) into a QVariant.
//! This macro defines Q_FLAG and on top of that adds Q_ENUM_IMPL which is
//! a part of Q_ENUM() macro that enables the metatype data but goes under
//! the moc radar to avoid double registration of the same data in the map
//! defined in moc_*.cpp.
//!
//! Simply put, instead of using Q_FLAG/Q_DECLARE_FLAGS combo (and struggling
//! to figure out what you should pass to Q_FLAG if you want to make it
//! wrappable in a QVariant) use the macro below, and things will just work.
//!
//! \sa https://bugreports.qt.io/browse/QTBUG-82295
#define QUO_DECLARE_FLAGS(Flags, Enum) \
    Q_DECLARE_FLAGS(Flags, Enum)       \
    Q_ENUM_IMPL(Enum)                  \
    Q_FLAG(Flags)

//! \brief Quotient replacement for the Q_FLAG_NS/Q_DECLARE_FLAGS combination
//!
//! This is the equivalent of QUO_DECLARE_FLAGS for enums declared at the
//! namespace level (be sure to provide Q_NAMESPACE _in the same file_
//! as the enum definition and this macro).
//! \sa QUO_DECLARE_FLAGS
#define QUO_DECLARE_FLAGS_NS(Flags, Enum) \
    Q_DECLARE_FLAGS(Flags, Enum)          \
    Q_ENUM_NS_IMPL(Enum)                  \
    Q_FLAG_NS(Flags)

namespace Quotient {
Q_NAMESPACE_EXPORT(QUOTIENT_API)
QML_ELEMENT
Q_CLASSINFO("RegisterEnumClassesUnscoped", "false") // Prevent unscoped enums from overwriting each other

//! \brief Enabling structure for conversion between a given enum and JSON
//!
//! Providing this structure in a way that satisfies either Serializable_Enum or Serializable_Flag
//! enables de/serialisation using fromJson() and toJson() for the respective enum. See commented
//! out code in the definition for an idea what should be there to satisfy either concept. `strings`
//! has the list of JSON representations for respective enumerators. `defaultValue` should have
//! the enumerator used when the string coming from JSON cannot be found in `strings`, or when
//! the JSON value is not even a string. `isFlag` defines whether the enumeration should be treated
//! as a collection of flag values (usually, such enum also has a QFlags counterpart type defined
//! with Q_FLAG, Q_FLAG_NS, QUO_DECLARE_FLAGS or QUO_DECLARE_FLAGS_NS). `isFlag` can either be
//! `false` or completely omitted from the definition for non-flag enumerations.
//!
//! \note This mechanism doesn't support using numeric values of enumerators in JSON; if you need
//!       to store and load the numeric value, use fromJson() and toJson() with the underlying
//!       integral type instead of the enumeration type.
//!
//! The rules to define enumerations and their string representations are different for non-flag and
//! flag enumerations - see the respective note blocks below. These cannot be checked at
//! compile-time, breaking them will lead to either assertion failures or incorrect conversion.
//!
//! \note For non-flag enumerations, \p EnumT must not have gaps in enumerators, or \p strings has
//!       to match those gaps (i.e., if \p EnumT is defined as
//!       <tt>{ Value1 = 1, Value2 = 3, Value3 = 5 }</tt> then \p strings should be defined as
//!       <tt>{ "", "Value1", "", "Value2", "", "Value3" }</tt> (mind the gap at value 0,
//!       in particular). Although it is allowed to have enumerations based
//! \note For flag enumerations, enumerators of \p EnumT must follow the power-of-two sequence
//!       starting from 1, so exactly 1,2,4,8,16 and so on. Having gaps is allowed but the same rule
//!       as for non-flag enumerations applies: \p strings has to reflect this gap, skipping an
//!       array item. As of now, there's no way to encode and decode the value of zero (no flags
//!       set) or a combination of flags (i.e. Flag4|Flag16).
//!
//! In most cases you can use facility macros, QUO_META_ENUM and QUO_META_FLAG, instead of writing
//! all the boilerplate yourself. The enumeration can be defined in any namespace but the macros
//! are only valid when put into `namespace Quotient`. Clients are free to write
//! `namespace Quotient { QUO_META_ENUM(...) }` for that matter.

template <typename EnumT>
struct QUOTIENT_API MetaEnum
{
    // static constexpr std::array strings{"one"_L1, "two"_L1, "three"_L1};
    // static constexpr auto defaultValue = EnumT::Undefined;
    // static constexpr bool isFlag = true; // If the enum should be treated as a flags group
};

template <typename EnumT>
concept Serializable_Enum = requires {
    typename MetaEnum<EnumT>;
    { MetaEnum<EnumT>::strings } -> std::ranges::range;
    { MetaEnum<EnumT>::defaultValue } -> std::convertible_to<EnumT>;
};

template <typename FlagT>
concept Serializable_Flag = std::is_unsigned_v<std::underlying_type_t<FlagT>>
                            && Serializable_Enum<FlagT> && MetaEnum<FlagT>::isFlag == true;

#define QUO_META_ENUM_IMPL(EnumType_, IsFlag_, DefaultValue_, ...) \
    template <>                                                    \
    struct QUOTIENT_API MetaEnum<EnumType_>                        \
    {                                                              \
        static constexpr std::array strings{__VA_ARGS__};          \
        static constexpr auto defaultValue = DefaultValue_;        \
        static constexpr bool isFlag = IsFlag_;                    \
    }; // End of macro

#define QUO_META_ENUM(EnumType_, DefaultValue_, ...) \
    QUO_META_ENUM_IMPL(EnumType_, false, DefaultValue_, __VA_ARGS__)

#define QUO_META_FLAG(EnumType_, DefaultValue_, ...) \
    QUO_META_ENUM_IMPL(EnumType_, true, DefaultValue_, __VA_ARGS__)

// TODO: code like below should be generated from the CS API definition

//! \brief Membership states
//!
//! These are used for member events. The names here are case-insensitively
//! equal to state names used on the wire.
//! \sa MemberEventContent, RoomMemberEvent
enum class Membership : uint16_t {
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
QUO_META_FLAG(Membership, Membership::Invalid, "join"_L1, "leave"_L1, "invite"_L1, "knock"_L1,
              "ban"_L1)

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

template <>
struct QUOTIENT_API MetaEnum<JoinState> : MetaEnum<Membership>
{
    // Same as Membership, except "ban"
    static constexpr auto strings = std::span(MetaEnum<Membership>::strings).subspan<0, 4>();
    // Protect against Membership gaining new values without JoinState being revisited
    static_assert(std::size(strings) + 1 == std::size(MetaEnum<Membership>::strings));
};

constexpr inline const auto &JoinStateStrings = MetaEnum<JoinState>::strings;

//! \brief Network job running policy flags
//!
//! So far only background/foreground flags are available.
//! \sa Connection::callApi, Connection::run
enum RunningPolicy { ForegroundRequest = 0x0, BackgroundRequest = 0x1 };
Q_ENUM_NS(RunningPolicy)

//! \brief The result of URI resolution using UriResolver
//! \sa UriResolver
enum UriResolveResult : int8_t {
    StillResolving = -1,
    UriResolved = 0,
    CouldNotResolve,
    IncorrectAction,
    InvalidUri,
    NoAccount
};
Q_ENUM_NS(UriResolveResult)

enum class RoomType : uint8_t {
    Space = 0,
    Undefined = 0xFF,
};
Q_ENUM_NS(RoomType)
QUO_META_ENUM(RoomType, RoomType::Undefined, "m.space"_L1)

enum class EncryptionType : uint8_t {
    MegolmV1AesSha2 = 0,
    Undefined = 0xFF,
};
Q_ENUM_NS(EncryptionType)

constexpr inline auto MegolmV1AesSha2AlgoKey = "m.megolm.v1.aes-sha2"_L1;

QUO_META_ENUM(EncryptionType, EncryptionType::Undefined, MegolmV1AesSha2AlgoKey)

//! Enum representing the available room join rules
enum class JoinRule : uint32_t {
    Public,
    Knock,
    Invite,
    Private,
    Restricted,
    KnockRestricted,
};
Q_ENUM_NS(JoinRule)
QUO_META_ENUM(JoinRule, JoinRule::Public, "public"_L1, "knock"_L1, "invite"_L1, "private"_L1,
              "restricted"_L1, "knock_restricted"_L1)

//! Why an encrypted room event could not be decrypted
enum class DecryptionFailure : uint8_t {
    None, //!< Not known to have failed: decrypted, not attempted yet, or not encrypted
    MissingKey, //!< The key has not been received (yet); it may still arrive
    //! The key was shared with this device only from a later message of the session on, e.g.
    //! because the user joined the room or this device was logged in after this message
    UnknownMessageIndex,
    WithheldUnverified, //!< The sender doesn't share keys with unverified devices
    WithheldBlacklisted, //!< The sender has blocked this device
    WithheldUnauthorised, //!< This device or user is not allowed to have the key
    WithheldUnavailable, //!< The key was requested, but the requested device doesn't have it
    WithheldNoOlm, //!< The sender could not establish a secure channel with this device
    WithheldHistoryNotShared, //!< The sender doesn't share the room history
    WithheldOther, //!< The key was withheld for a reason not covered above
    Other, //!< Any other reason, e.g. a malformed event or unsupported algorithm
};
Q_ENUM_NS(DecryptionFailure)

} // namespace Quotient
Q_DECLARE_OPERATORS_FOR_FLAGS(Quotient::MembershipMask)
Q_DECLARE_OPERATORS_FOR_FLAGS(Quotient::JoinStates)
