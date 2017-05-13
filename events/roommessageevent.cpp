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

#include "logging.h"
#include "util.h"

#include <QtCore/QMimeDatabase>

using namespace QMatrixClient;

class RoomMessageEvent::Private
{
    public:
        Private() : msgtype(MessageEventType::Unknown), content(nullptr) {}
        ~Private() { if (content) delete content; }
        
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

using namespace MessageEventContent;

Base* RoomMessageEvent::content() const
{
    return d->content;
}

using ContentPair = std::pair<MessageEventType, MessageEventContent::Base*>;

template <MessageEventType EnumType, typename ContentT>
ContentPair make(const QJsonObject& json)
{
    return { EnumType, new ContentT(json) };
}

ContentPair makeVideo(const QJsonObject& json)
{
    auto c = new VideoContent(json);
    // Only for m.video, the spec puts a thumbnail inside "info" JSON key. Once
    // this is fixed, VideoContent creation will switch to make<>().
    const QJsonObject infoJson = json["info"].toObject();
    if (infoJson.contains("thumbnail_url"))
    {
        c->thumbnail = ImageInfo(infoJson["thumbnail_url"].toString(),
                                         infoJson["thumbnail_info"].toObject());
    }
    return { MessageEventType::Video, c };
};

ContentPair makeUnknown(const QJsonObject& json)
{
    qCDebug(EVENTS) << "RoomMessageEvent: couldn't resolve msgtype, JSON follows:";
    qCDebug(EVENTS) << json;
    return { MessageEventType::Unknown, new Base };
}

RoomMessageEvent* RoomMessageEvent::fromJson(const QJsonObject& obj)
{
    RoomMessageEvent* e = new RoomMessageEvent();
    e->parseJson(obj);
    if( obj.contains("sender") )
    {
        e->d->userId = obj.value("sender").toString();
    } else {
        qCDebug(EVENTS) << "RoomMessageEvent: user_id not found";
    }
    if( obj.contains("content") )
    {
        const QJsonObject content = obj["content"].toObject();
        if ( content.contains("msgtype") && content.contains("body") )
        {
            e->d->plainBody = content["body"].toString();

            auto delegate = lookup(content.value("msgtype").toString(),
                    "m.text", make<MessageEventType::Text, TextContent>,
                    "m.emote", make<MessageEventType::Emote, TextContent>,
                    "m.notice", make<MessageEventType::Notice, TextContent>,
                    "m.image", make<MessageEventType::Image, ImageContent>,
                    "m.file", make<MessageEventType::File, FileContent>,
                    "m.location", make<MessageEventType::Location, LocationContent>,
                    "m.video", makeVideo,
                    "m.audio", make<MessageEventType::Audio, AudioContent>,
                    // Insert new message types before this line
                    makeUnknown
                );
            std::tie(e->d->msgtype, e->d->content) = delegate(content);
        }
        else
        {
            qCWarning(EVENTS) << "RoomMessageEvent(" << e->id() << "): no body or msgtype";
            qCDebug(EVENTS) << obj;
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
        // Falling back to plain text, as there's no standard way to describe
        // rich text in messages.
        body = json["body"].toString();
        mimeType = db.mimeTypeForName("text/plain");
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

VideoInfo::VideoInfo(QUrl u, const QJsonObject& infoJson)
    : FileInfo(u, infoJson)
    , duration(infoJson["duration"].toInt())
    , imageSize(infoJson["w"].toInt(), infoJson["h"].toInt())
{ }

AudioInfo::AudioInfo(QUrl u, const QJsonObject& infoJson)
    : FileInfo(u, infoJson)
    , duration(infoJson["duration"].toInt())
{ }
