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

#include "roommessageevent.h"

#include <QtCore/QJsonObject>
#include <QtCore/QMimeDatabase>
#include <QtCore/QDebug>

using namespace QMatrixClient;

class RoomMessageEvent::Private
{
    public:
        Private() : msgtype(MessageEventType::Unknown), content(nullptr) {}
        
        QString userId;
        MessageEventType msgtype;
        QString plainBody;
        MessageEventContent::Base* content;
};

RoomMessageEvent::RoomMessageEvent()
    : Event(EventType::RoomMessage)
    , d(new Private)
{ }

RoomMessageEvent::~RoomMessageEvent()
{
    delete d;
}

QString RoomMessageEvent::userId() const
{
    return d->userId;
}

MessageEventType RoomMessageEvent::msgtype() const
{
    return d->msgtype;
}

QString RoomMessageEvent::plainBody() const
{
    return d->plainBody;
}

QString RoomMessageEvent::body() const
{
    return plainBody();
}

MessageEventContent::Base* RoomMessageEvent::content() const
{
    return d->content;
}

template <class ContentT>
MessageEventContent::Base* make(const QJsonObject& json)
{
    return new ContentT(json);
}

template <class ContentT>
MessageEventContent::Base* make2(const QJsonObject& json)
{
    return new ContentT(json["url"].toString(), json["info"].toObject());
};

RoomMessageEvent* RoomMessageEvent::fromJson(const QJsonObject& obj)
{
    RoomMessageEvent* e = new RoomMessageEvent();
    e->parseJson(obj);
    if( obj.contains("sender") )
    {
        e->d->userId = obj.value("sender").toString();
    } else {
        qDebug() << "RoomMessageEvent: user_id not found";
    }
    if( obj.contains("content") )
    {
        const QJsonObject content = obj["content"].toObject();
        if ( content.contains("msgtype") && content.contains("body") )
        {
            using namespace MessageEventContent;

            e->d->plainBody = content["body"].toString();

            struct Factory
            {
                QString jsonTag;
                MessageEventType enumTag;
                MessageEventContent::Base*(*make)(const QJsonObject& json);
            };

            const Factory factories[] {
                { "m.text", MessageEventType::Text, make<TextContent> },
                { "m.emote", MessageEventType::Emote, make<TextContent> },
                { "m.notice", MessageEventType::Notice, make<TextContent> },
                { "m.image", MessageEventType::Image, make<ImageContent> },
                { "m.file", MessageEventType::File, make<FileContent> },
                { "m.location", MessageEventType::Location, make<LocationContent> },
                { "m.video", MessageEventType::Video, make2<VideoContent> },
                { "m.audio", MessageEventType::Audio, make2<AudioContent> },
                // Insert new message types before this line
            };

            QString msgtype = content.value("msgtype").toString();
            for (auto f: factories)
            {
                if (msgtype == f.jsonTag)
                {
                    e->d->msgtype = f.enumTag;
                    e->d->content = f.make(content);
                    break;
                }
            }
            if (e->d->msgtype == MessageEventType::Unknown)
            {
                qDebug() << "RoomMessageEvent: unknown msgtype: " << msgtype;
                qDebug() << obj;
                e->d->content = new Base;
            }
        }
        else
        {
            qWarning() << "RoomMessageEvent(" << e->id() << "): no body or msgtype";
            qDebug() << obj;
        }
    }
    return e;
}

using namespace MessageEventContent;

TextContent::TextContent(const QJsonObject& json)
{
    QMimeDatabase db;

    // Special-casing the custom matrix.org's (actually, Vector's) way
    // of sending HTML messages.
    if (json["format"].toString() == "org.matrix.custom.html")
    {
        mimeType = db.mimeTypeForName("text/html");
        body = json["formatted_body"].toString();
    } else {
        // Best-guessing from the content
        body = json["body"].toString();
        mimeType = db.mimeTypeForData(body.toUtf8());
    }
}

FileInfo::FileInfo(QUrl u, const QJsonObject& infoJson, QString originalFilename)
    : url(u)
    , fileSize(infoJson["size"].toInt())
    , mimetype(QMimeDatabase().mimeTypeForName(infoJson["mimetype"].toString()))
    , originalName(originalFilename)
{
    if (!mimetype.isValid())
        mimetype = QMimeDatabase().mimeTypeForData(QByteArray());
}

ImageInfo::ImageInfo(QUrl u, const QJsonObject& infoJson)
    : FileInfo(u, infoJson)
    , imageSize(infoJson["w"].toInt(), infoJson["h"].toInt())
{ }

LocationContent::LocationContent(const QJsonObject& json)
    : geoUri(json["geo_uri"].toString())
    , thumbnail(json["thumbnail_url"].toString(),
                json["thumbnail_info"].toObject())
{ }

VideoContent::VideoContent(QUrl u, const QJsonObject& infoJson)
    : FileInfo(u, infoJson)
    , duration(infoJson["duration"].toInt())
    , imageSize(infoJson["w"].toInt(), infoJson["h"].toInt())
    , thumbnail(infoJson["thumbnail_url"].toString(),
                infoJson["thumbnail_info"].toObject())
{ }

AudioContent::AudioContent(QUrl u, const QJsonObject& infoJson)
    : FileInfo(u, infoJson)
    , duration(infoJson["duration"].toInt())
{ }
