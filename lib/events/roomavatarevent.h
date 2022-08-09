// SPDX-FileCopyrightText: 2017 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "eventcontent.h"
#include "stateevent.h"

namespace Quotient {
class QUOTIENT_API RoomAvatarEvent
    : public StateEvent<EventContent::ImageContent> {
    // It's a bit of an overkill to use a full-fledged ImageContent
    // because in reality m.room.avatar usually only has a single URL,
    // without a thumbnail. But The Spec says there be thumbnails, and
    // we follow The Spec.
public:
    QUO_EVENT(RoomAvatarEvent, "m.room.avatar")
    explicit RoomAvatarEvent(const QJsonObject& obj) : StateEvent(typeId(), obj)
    {}
    explicit RoomAvatarEvent(const EventContent::ImageContent& avatar)
        : StateEvent(typeId(), matrixTypeId(), QString(), avatar)
    {}
    // A replica of EventContent::ImageInfo constructor
    explicit RoomAvatarEvent(const QUrl& mxcUrl, qint64 fileSize = -1,
                             QMimeType mimeType = {},
                             const QSize& imageSize = {},
                             const QString& originalFilename = {})
        : RoomAvatarEvent(EventContent::ImageContent {
            mxcUrl, fileSize, mimeType, imageSize, originalFilename })
    {}

    QUrl url() const { return content().url(); }
};
} // namespace Quotient
