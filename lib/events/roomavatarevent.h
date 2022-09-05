// SPDX-FileCopyrightText: 2017 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "eventcontent.h"
#include "stateevent.h"

namespace Quotient {
class QUOTIENT_API RoomAvatarEvent
    : public KeylessStateEventBase<RoomAvatarEvent,
                       EventContent::ImageContent> {
    // It's a bit of an overkill to use a full-fledged ImageContent
    // because in reality m.room.avatar usually only has a single URL,
    // without a thumbnail. But The Spec says there be thumbnails, and
    // we follow The Spec (and ImageContent is very convenient to reuse here).
public:
    QUO_EVENT(RoomAvatarEvent, "m.room.avatar")
    using KeylessStateEventBase::KeylessStateEventBase;

    QUrl url() const { return content().url(); }
};
} // namespace Quotient
