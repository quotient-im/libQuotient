// SPDX-FileCopyrightText: 2019 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "roomtombstoneevent.h"

using namespace Quotient;

QString RoomTombstoneEvent::serverMessage() const
{
    return fromJson<QString>(contentJson()["body"_ls]);
}

QString RoomTombstoneEvent::successorRoomId() const
{
    return fromJson<QString>(contentJson()["replacement_room"_ls]);
}
