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
    return contentPart<bool>("m.federate"_ls);
}

QString RoomCreateEvent::version() const
{
    return contentPart<QString>("room_version"_ls);
}

RoomCreateEvent::Predecessor RoomCreateEvent::predecessor() const
{
    const auto predJson = contentPart<QJsonObject>("predecessor"_ls);
    return { fromJson<QString>(predJson[RoomIdKeyL]),
             fromJson<QString>(predJson[EventIdKeyL]) };
}

bool RoomCreateEvent::isUpgrade() const
{
    return contentJson().contains("predecessor"_ls);
}

RoomType RoomCreateEvent::roomType() const
{
    return contentPart<RoomType>("type"_ls);
}
