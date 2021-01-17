// SPDX-FileCopyrightText: 2016 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <QtCore/QFlags>

#include <array>

namespace Quotient {
enum class JoinState : unsigned int {
    Join = 0x1,
    Invite = 0x2,
    Leave = 0x4,
};

Q_DECLARE_FLAGS(JoinStates, JoinState)

// We cannot use Q_ENUM outside of a Q_OBJECT and besides, we want
// to use strings that match respective JSON keys.
static const std::array<const char*, 3> JoinStateStrings { { "join", "invite",
                                                             "leave" } };

inline const char* toCString(JoinState js)
{
    size_t state = size_t(js), index = 0;
    while (state >>= 1u)
        ++index;
    return JoinStateStrings[index];
}
} // namespace Quotient
Q_DECLARE_OPERATORS_FOR_FLAGS(Quotient::JoinStates)
