// SDPX-FileCopyrightText: 2020 Carl Schwan <carlschwan@kde.org>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "roomevent.h"
#include "eventcontent.h"

namespace Quotient {

/// Sticker messages are specialised image messages that are displayed without
/// controls (e.g. no "download" link, or light-box view on click, as would be
/// displayed for for m.image events).
class QUOTIENT_API StickerEvent : public RoomEvent
{
public:
    DEFINE_EVENT_TYPEID("m.sticker", StickerEvent)

    explicit StickerEvent(const QJsonObject& obj)
        : RoomEvent(TypeId, obj)
        , m_imageContent(
              EventContent::ImageContent(obj["content"_ls].toObject()))
    {}

    /// \brief A textual representation or associated description of the
    /// sticker image.
    ///
    /// This could be the alt text of the original image, or a message to
    /// accompany and further describe the sticker.
    QUO_CONTENT_GETTER(QString, body)

    /// \brief Metadata about the image referred to in url including a
    /// thumbnail representation.
    const EventContent::ImageContent& image() const
    {
        return m_imageContent;
    }

    /// \brief The URL to the sticker image. This must be a valid mxc:// URI.
    QUrl url() const
    {
        return m_imageContent.url();
    }

private:
    EventContent::ImageContent m_imageContent;
};
REGISTER_EVENT_TYPE(StickerEvent)
} // namespace Quotient
