// SPDX-FileCopyrightText: 2019 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "roomcreateevent.h"

using namespace Quotient;

template <>
RoomType Quotient::fromJson(const QJsonValue& jv)
{
    return enumFromJsonString(jv.toString(), RoomTypeStrings,
                              RoomType::Undefined);
}

bool RoomCreateEvent::isFederated() const
{
    return contentPart<bool>("m.federate"_L1);
}

QString RoomCreateEvent::version() const
{
    return contentPart<QString>("room_version"_L1);
}

RoomCreateEvent::Predecessor RoomCreateEvent::predecessor() const
{
    const auto predJson = contentPart<QJsonObject>("predecessor"_L1);
    return { fromJson<QString>(predJson[RoomIdKey]),
             fromJson<QString>(predJson[EventIdKey]) };
}

bool RoomCreateEvent::isUpgrade() const
{
    return contentJson().contains("predecessor"_L1);
}

RoomType RoomCreateEvent::roomType() const
{
    return contentPart<RoomType>("type"_L1);
}
