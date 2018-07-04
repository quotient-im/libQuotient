/******************************************************************************
 * Copyright (C) 2018 Kitsune Ral <kitsune-ral@users.sf.net>
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

#include "directchatevent.h"

#include <QtCore/QJsonArray>

using namespace QMatrixClient;

QMultiHash<QString, QString> DirectChatEvent::usersToDirectChats() const
{
    QMultiHash<QString, QString> result;
    const auto& json = contentJson();
    for (auto it = json.begin(); it != json.end(); ++it)
    {
        // Beware of range-for's over temporary returned from temporary
        // (see the bottom of
        // http://en.cppreference.com/w/cpp/language/range-for#Explanation)
        const auto roomIds = it.value().toArray();
        for (const auto& roomIdValue: roomIds)
            result.insert(it.key(), roomIdValue.toString());
    }
    return result;
}
