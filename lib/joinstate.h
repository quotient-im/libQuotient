/******************************************************************************
 * Copyright (C) 2015 Felix Rohrbach <kde@fxrh.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#pragma once

#include <QtCore/QFlags>

#include <array>

namespace QMatrixClient
{
    enum class JoinState : unsigned int
    {
        Join = 0x1,
        Invite = 0x2,
        Leave = 0x4,
    };

    Q_DECLARE_FLAGS(JoinStates, JoinState)

    // We cannot use REGISTER_ENUM outside of a Q_OBJECT and besides, we want
    // to use strings that match respective JSON keys.
    static const std::array<const char*, 3> JoinStateStrings
        { { "join", "invite", "leave" } };

    inline const char* toCString(JoinState js)
    {
        size_t state = size_t(js), index = 0;
        while (state >>= 1) ++index;
        return JoinStateStrings[index];
    }
}  // namespace QMatrixClient
Q_DECLARE_OPERATORS_FOR_FLAGS(QMatrixClient::JoinStates)
