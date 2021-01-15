/******************************************************************************
 * SPDX-FileCopyrightText: 2018 Kitsune Ral <kitsune-ral@users.sf.net>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "directchatevent.h"

#include <QtCore/QJsonArray>

using namespace Quotient;

QMultiHash<QString, QString> DirectChatEvent::usersToDirectChats() const
{
    QMultiHash<QString, QString> result;
    const auto& json = contentJson();
    for (auto it = json.begin(); it != json.end(); ++it) {
        // Beware of range-for's over temporary returned from temporary
        // (see the bottom of
        // http://en.cppreference.com/w/cpp/language/range-for#Explanation)
        const auto roomIds = it.value().toArray();
        for (const auto& roomIdValue : roomIds)
            result.insert(it.key(), roomIdValue.toString());
    }
    return result;
}
