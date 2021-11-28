// SPDX-FileCopyrightText: 2018 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "stateevent.h"

using namespace Quotient;

// Aside from the normal factory to instantiate StateEventBase inheritors
// StateEventBase itself can be instantiated if there's a state_key JSON key
// but the event type is unknown.
[[maybe_unused]] static auto stateEventTypeInitialised =
    RoomEvent::factory_t::addMethod(
        [](const QJsonObject& json, const QString& matrixType) -> StateEventPtr {
            if (!json.contains(StateKeyKeyL))
                return nullptr;

            if (auto e = StateEventBase::factory_t::make(json, matrixType))
                return e;

            return makeEvent<StateEventBase>(unknownEventTypeId(), json);
        });

StateEventBase::StateEventBase(Event::Type type, event_mtype_t matrixType,
                               const QString& stateKey,
                               const QJsonObject& contentJson)
    : RoomEvent(type, basicStateEventJson(matrixType, contentJson, stateKey))
{}

bool StateEventBase::repeatsState() const
{
    const auto prevContentJson = unsignedPart(PrevContentKeyL);
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
