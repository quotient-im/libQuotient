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
        class TextContent: public Base
        {
            public:
                TextContent(const QJsonObject& json);

                QMimeType mimeType;
                QString body;
        };

        class ImageContent: public Base
        {
            public:
                QUrl url;
                int height;
                int width;
                int size;
                QString mimetype;
        };

        class FileContent: public Base
        {
            public:
                QString filename;
                QString mimetype;
                int size;
                QUrl url;
        };

        class LocationContent: public Base
        {
            public:
                QString geoUri;
                int thumbnailHeight;
                int thumbnailWidth;
                QString thumbnailMimetype;
                int thumbnailSize;
                QUrl thumbnailUrl;
        };

        class VideoContent: public Base
        {
            public:
                QUrl url;
                int duration;
                int width;
                int height;
                int size;
                QString mimetype;
                int thumbnailWidth;
                int thumbnailHeight;
                int thumbnailSize;
                QString thumbnailMimetype;
                QUrl thumbnailUrl;
        };

        class AudioContent: public Base
        {
            public:
                QUrl url;
                int size;
                int duration;
                QString mimetype;
        };
    }
}

#endif // QMATRIXCLIENT_ROOMMESSAGEEVENT_H
