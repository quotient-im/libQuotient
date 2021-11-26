// SPDX-FileCopyrightText: 2019 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "roomcreateevent.h"
#include "logging.h"

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
    return fromJson<bool>(contentJson()["m.federate"_ls]);
}

QString RoomCreateEvent::version() const
{
    return fromJson<QString>(contentJson()["room_version"_ls]);
}

RoomCreateEvent::Predecessor RoomCreateEvent::predecessor() const
{
    const auto predJson = contentJson()["predecessor"_ls].toObject();
    return { fromJson<QString>(predJson[RoomIdKeyL]),
             fromJson<QString>(predJson[EventIdKeyL]) };
}

bool RoomCreateEvent::isUpgrade() const
{
    return contentJson().contains("predecessor"_ls);
}

RoomType RoomCreateEvent::roomType() const
{
    return fromJson<RoomType>(contentJson()["type"_ls]);
}
