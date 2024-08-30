// SPDX-FileCopyrightText: 2018 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "stateevent.h"

using namespace Quotient;

StateEvent::StateEvent(const QJsonObject& json)
    : RoomEvent(json)
{
    Q_ASSERT_X(json.contains(StateKeyKey), __FUNCTION__,
               "Attempt to create a state event without state key");
}

StateEvent::StateEvent(event_type_t type, const QString& stateKey,
                       const QJsonObject& contentJson)
    : RoomEvent(basicJson(type, stateKey, contentJson))
{}

bool StateEvent::repeatsState() const
{
    return contentJson() == unsignedPart<QJsonObject>(PrevContentKey);
}

QString StateEvent::replacedState() const
{
    return unsignedPart<QString>("replaces_state"_L1);
}

void StateEvent::dumpTo(QDebug dbg) const
{
    if (!stateKey().isEmpty())
        dbg << '<' << stateKey() << "> ";
    if (const auto prevContentJson = unsignedPart<QJsonObject>(PrevContentKey);
        !prevContentJson.isEmpty())
        dbg << QJsonDocument(prevContentJson).toJson(QJsonDocument::Compact)
            << " -> ";
    RoomEvent::dumpTo(dbg);
}
