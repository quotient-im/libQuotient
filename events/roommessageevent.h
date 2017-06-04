/******************************************************************************
 * Copyright (C) 2015 Felix Rohrbach <kde@fxrh.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#pragma once

#include "event.h"

#include <QtCore/QUrl>
#include <QtCore/QMimeType>
#include <QtCore/QSize>

#include <memory>

namespace QMatrixClient
{
    namespace MessageEventContent
    {
        class Base
        {
            Q_GADGET
            public:
                enum class Type
                {
                    Text, Emote, Notice, Image, File, Location, Video, Audio, Unknown
                };

                virtual ~Base() = default;

                REGISTER_ENUM(Type)
        };
        using CType = Base::Type;
    }  // namespace MessageEventContent
    using MessageEventType = MessageEventContent::CType;

    class RoomMessageEvent: public RoomEvent
    {
        public:
            explicit RoomMessageEvent(const QJsonObject& obj);
            ~RoomMessageEvent();

            MessageEventType msgtype() const                 { return _msgtype; }
            const QString& plainBody() const                 { return _plainBody; }
            const MessageEventContent::Base* content() const { return _content; }

        private:
            MessageEventType _msgtype;
            QString _plainBody;
            MessageEventContent::Base* _content;
    };

    namespace MessageEventContent
    {
        // The below structures fairly follow CS spec 11.2.1.6. The overall
        // set of attributes for each content types is a superset of the spec
        // but specific aggregation structure is altered.

        class TextContent: public Base
        {
            public:
                explicit TextContent(const QJsonObject& json);

                QMimeType mimeType;
                QString body;
        };

        class FileInfo: public Base
        {
            public:
                FileInfo(QUrl u, const QJsonObject& infoJson,
                         QString originalFilename = QString());

                QUrl url;
                int fileSize;
                QMimeType mimetype;
                QString originalName;
        };

        class ImageInfo: public FileInfo
        {
            public:
                ImageInfo(QUrl u, const QJsonObject& infoJson);

                QSize imageSize;
        };

        template <class ContentInfoT>
        class ThumbnailedContent: public ContentInfoT
        {
            public:
                explicit ThumbnailedContent(const QJsonObject& json)
                    : ContentInfoT(json["url"].toString(), json["info"].toObject())
                    , thumbnail(json["thumbnail_url"].toString(),
                                json["thumbnail_info"].toObject())
                { }

                ImageInfo thumbnail;
        };

        using ImageContent = ThumbnailedContent<ImageInfo>;
        using FileContent = ThumbnailedContent<FileInfo>;

        class LocationContent: public Base
        {
            public:
                explicit LocationContent(const QJsonObject& json);

                QString geoUri;
                ImageInfo thumbnail;
        };

        class VideoInfo: public FileInfo
        {
            public:
                VideoInfo(QUrl u, const QJsonObject& infoJson);

                int duration;
                QSize imageSize;
        };
        using VideoContent = ThumbnailedContent<VideoInfo>;

        class AudioInfo: public FileInfo
        {
            public:
                AudioInfo(QUrl u, const QJsonObject& infoJson);

                int duration;
        };
        using AudioContent = ThumbnailedContent<AudioInfo>;
    }  // namespace MessageEventContent
}  // namespace QMatrixClient
