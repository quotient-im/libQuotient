// SPDX-FileCopyrightText: 2019 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "roomcreateevent.h"

using namespace Quotient;

template <>
struct Quotient::JsonConverter<RoomType> {
    static RoomType load(const QJsonValue& jv)
    {
        const auto& roomTypeString = jv.toString();
        for (auto it = RoomTypeStrings.begin(); it != RoomTypeStrings.end();
             ++it)
            if (roomTypeString == *it)
                return RoomType(it - RoomTypeStrings.begin());

        if (!roomTypeString.isEmpty())
            qCWarning(EVENTS) << "Unknown Room Type: " << roomTypeString;
        return RoomType::Undefined;
    }
};

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
