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

#include <QtCore/QMimeType>
#include <QtCore/QUrl>
#include <QtCore/QSize>

#include "event.h"

namespace QMatrixClient
{

    namespace MessageEventContent
    {
        class Base { };
    }

    class RoomMessageEvent: public Event
    {
        public:
            enum MsgType
            {
                Text, Emote, Notice, Image, File, Location, Video, Audio, Unknown
            };

            RoomMessageEvent(const QJsonObject& obj);
            virtual ~RoomMessageEvent();
            
            QString userId() const;
            MsgType msgtype() const;

            /**
             * @brief Get the event content
             * @return the event content, or the first found representation
             * if there are several ones; nullptr if the event has no content.
             */
            MessageEventContent::Base* content() const;

            QString plainBody() const;

            /**
             * Same as plainBody() for now; might change for "best-looking body"
             * in the future. For richer contents, use content-specific data.
             *
             * @deprecated
             */
            QString body() const;

            static RoomMessageEvent* fromJson( const QJsonObject& obj );

        private:
            class Private;
            Private* d;
    };

    // For compatibility with the previous library interface
    using MessageEventType = RoomMessageEvent::MsgType;

    namespace MessageEventContent
    {
        // The below structures fairly follow CS spec 11.2.1.6. The overall
        // set of attributes for each content types is a superset of the spec
        // but specific aggregation structure is altered.

        class Text: public Base
        {
            public:
                Text(const QJsonObject& json);

                QMimeType mimeType;
                QString body;
        };

        class FileInfo: public Base
        {
            public:
                FileInfo(QUrl url, const QJsonObject& infoJson,
                         QString originalFilename = QString());

                QUrl url;
                QMimeType mimeType;
                int fileSize;
                QString originalName;
        };

        class ImageInfo: public FileInfo
        {
            public:
                ImageInfo(QUrl url, const QJsonObject& infoJson);

                QSize imageSize;
        };

        class VideoInfo: public FileInfo
        {
            public:
                VideoInfo(QUrl url, const QJsonObject& infoJson);

                int duration;
                QSize imageSize;
                ImageInfo thumbnail;
        };

        class AudioInfo: public FileInfo
        {
            public:
                AudioInfo(QUrl url, const QJsonObject& infoJson);

                int duration;
        };

        template <class ContentInfoT>
        class FileBased: public ContentInfoT
        {
            public:
                FileBased(const QJsonObject& json)
                    : ContentInfoT(json["url"].toString(), json["info"].toObject())
                { }
        };

        template <class ContentInfoT>
        class ThumbnailedFileBased: public FileBased<ContentInfoT>
        {
            public:
                ThumbnailedFileBased(const QJsonObject& json)
                    : FileBased<ContentInfoT>(json)
                    , thumbnail(json["thumbnail_url"].toString(),
                                json["thumbnail_info"].toObject())
                { }

                ImageInfo thumbnail;
        };

        using Image = ThumbnailedFileBased<ImageInfo>;
        using File = ThumbnailedFileBased<FileInfo>;

        class Location: public Base
        {
            public:
                Location(const QJsonObject& json);

                QUrl geoUri;
                ImageInfo thumbnail;
        };

        using Video = FileBased<VideoInfo>;
        using Audio = FileBased<AudioInfo>;
    }
}

#endif // QMATRIXCLIENT_ROOMMESSAGEEVENT_H
