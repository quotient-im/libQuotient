// SPDX-FileCopyrightText: 2018 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "stateevent.h"

using namespace Quotient;

StateEventBase::StateEventBase(const QJsonObject& json)
    : RoomEvent(json)
{
    Q_ASSERT_X(json.contains(StateKeyKeyL), __FUNCTION__,
               "Attempt to create a state event without state key");
}

StateEventBase::StateEventBase(Event::Type type, const QString& stateKey,
                               const QJsonObject& contentJson)
    : RoomEvent(basicJson(type, stateKey, contentJson))
{}

bool StateEventBase::repeatsState() const
{
    const auto prevContentJson = unsignedPart<QJsonObject>(PrevContentKeyL);
    return fullJson().value(ContentKeyL) == prevContentJson;
}

QString StateEventBase::replacedState() const
{
    return unsignedPart<QString>("replaces_state"_ls);
}

void StateEventBase::dumpTo(QDebug dbg) const
{
    if (!stateKey().isEmpty())
        dbg << '<' << stateKey() << "> ";
    if (const auto prevContentJson = unsignedPart<QJsonObject>(PrevContentKeyL);
        !prevContentJson.isEmpty())
        dbg << QJsonDocument(prevContentJson).toJson(QJsonDocument::Compact)
            << " -> ";
    RoomEvent::dumpTo(dbg);
}
