// SPDX-FileCopyrightText: 2018 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "stateevent.h"

using namespace Quotient;

StateEventBase::StateEventBase(Type type, const QJsonObject& json)
    : RoomEvent(json.contains(StateKeyKeyL) ? type : UnknownEventTypeId, json)
{
    if (Event::type() == UnknownEventTypeId && !json.contains(StateKeyKeyL))
        qWarning(EVENTS) << "Attempt to create a state event with no stateKey -"
                            "forcing the event type to unknown to avoid damage";
}

StateEventBase::StateEventBase(Event::Type type, event_mtype_t matrixType,
                               const QString& stateKey,
                               const QJsonObject& contentJson)
    : RoomEvent(type, basicJson(matrixType, contentJson, stateKey))
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
