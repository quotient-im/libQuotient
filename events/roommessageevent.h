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

#ifndef QMATRIXCLIENT_ROOMMESSAGEEVENT_H
#define QMATRIXCLIENT_ROOMMESSAGEEVENT_H

#include <QtCore/QUrl>
#include <QtCore/QMimeType>
#include <QtCore/QSize>

#include "event.h"

namespace QMatrixClient
{
    enum class MessageEventType
    {
        Text, Emote, Notice, Image, File, Location, Video, Audio, Unknown
    };

    namespace MessageEventContent
    {
        class Base { };
    }

    class RoomMessageEvent: public Event
    {
        public:
            RoomMessageEvent();
            virtual ~RoomMessageEvent();
            
            QString userId() const;
            MessageEventType msgtype() const;

            QString plainBody() const;

            /**
             * Same as plainBody() for now; might change for "best-looking body"
             * in the future. For richer contents, use content-specific data.
             *
             * @deprecated
             */
            QString body() const;

            MessageEventContent::Base* content() const;
        
            static RoomMessageEvent* fromJson( const QJsonObject& obj );
            
        private:
            class Private;
            Private* d;
    };

    namespace MessageEventContent
    {
        // The below structures fairly follow CS spec 11.2.1.6. The overall
        // set of attributes for each content types is a superset of the spec
        // but specific aggregation structure is altered.

        class TextContent: public Base
        {
            public:
                TextContent(const QJsonObject& json);

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
                ThumbnailedContent(const QJsonObject& json)
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
                LocationContent(const QJsonObject& json);

                QString geoUri;
                ImageInfo thumbnail;
        };

        // The spec structures m.video messages differently for some reason -
        // instead of putting a thumbnail block on the top level, as with
        // file and image, it puts it inside "info" key. So instead of
        // using ThumbnailContent<> base, we add the thumbnail into VideoInfo explicitly.
        class VideoContent: public FileInfo
        {
            public:
                VideoContent(QUrl u, const QJsonObject& infoJson);

                int duration;
                QSize imageSize;
                ImageInfo thumbnail;
        };

        class AudioContent: public FileInfo
        {
            public:
                AudioContent(QUrl u, const QJsonObject& infoJson);

                int duration;
        };
    }
}

#endif // QMATRIXCLIENT_ROOMMESSAGEEVENT_H
