/******************************************************************************
 * SPDX-FileCopyrightText: 2019 QMatrixClient project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "roomcreateevent.h"

using namespace Quotient;

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
