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

//#include <QtCore/QDateTime>
#include <QtCore/QJsonObject>
#include <QtCore/QMimeDatabase>
#include <QtCore/QDebug>

using namespace QMatrixClient;

class RoomMessageEvent::Private
{
    public:
        Private() : msgtype(Unknown) {}
        
        QString userId;
        MessageEventType msgtype;
        QString plainBody;

        MessageEventContent::Base* content;
//        QDateTime hsob_ts;
};

template <class ContentInfoT>
MessageEventContent::Base* make(const QJsonObject& json)
{
    return new ContentInfoT(json);
}

RoomMessageEvent::RoomMessageEvent(const QJsonObject& obj)
    : Event(EventType::RoomMessage)
    , d(new Private)
{
    parseJson(obj);

    if( obj.contains("sender") )
    {
        d->userId = obj.value("sender").toString();
    } else {
        qDebug() << "RoomMessageEvent: sender id not found";
    }

    if( obj.contains("content") )
    {
        const QJsonObject content = obj.value("content").toObject();
        if ( content.contains("msgtype") && content.contains("body") )
        {
            d->plainBody = content.value("body").toString();

            namespace MEC = MessageEventContent;
            struct Factory
            {
                QString jsonTag;
                MsgType enumTag;
                MEC::Base*(*make)(const QJsonObject& json);
            };

            const Factory factories[] {
                { "m.text", Text, make<MEC::Text> },
                { "m.emote", Emote, make<MEC::Text> },
                { "m.notice", Notice, make<MEC::Text> },
                { "m.image", Image, make<MEC::Image> },
                { "m.file", File, make<MEC::File> },
                { "m.location", Location, make<MEC::Location> },
                { "m.video", Video, make<MEC::Video> },
                { "m.audio", Audio, make<MEC::Audio> },
                // Insert new message types before this line
            };

            QString msgtype = content.value("msgtype").toString();
            for (auto f: factories)
            {
                if (msgtype == f.jsonTag)
                {
                    d->msgtype = f.enumTag;
                    d->content = f.make(content);
                    break;
                }
            }
            if (d->msgtype == Unknown)
            {
                qDebug() << "Unknown msgtype: " << msgtype;
                qDebug() << obj;
                d->content = new MEC::Base();
            }
        }
        else
        {
            qWarning() << "Message event" << id() << ": no body or msgtype";
            qDebug() << obj;
        }
    }
}

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

//QDateTime RoomMessageEvent::hsob_ts() const
//{
//    return d->hsob_ts;
//}

MessageEventContent::Base* RoomMessageEvent::content() const
{
    return d->content;
}

RoomMessageEvent* RoomMessageEvent::fromJson(const QJsonObject& obj)
{
    return new RoomMessageEvent(obj);
}

using namespace MessageEventContent;

Text::Text(const QJsonObject& json)
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

FileInfo::FileInfo(QUrl url, const QJsonObject& infoJson, QString originalFilename)
    : url(url)
    , mimeType(QMimeDatabase().mimeTypeForName(infoJson["mimetype"].toString()))
    , fileSize(infoJson["size"].toInt())
    , originalName(originalFilename)
{
    if (!mimeType.isValid())
        mimeType = QMimeDatabase().mimeTypeForData(QByteArray());
}

ImageInfo::ImageInfo(QUrl url, const QJsonObject& infoJson)
    : FileInfo(url, infoJson)
    , imageSize(infoJson["w"].toInt(), infoJson["h"].toInt())
{  }

VideoInfo::VideoInfo(QUrl url, const QJsonObject& infoJson)
    : FileInfo(url, infoJson)
    , duration(infoJson["duration"].toInt())
    , imageSize(infoJson["w"].toInt(), infoJson["h"].toInt())
    , thumbnail(infoJson["thumbnail_url"].toString(),
                infoJson["thumbnail_info"].toObject())
{ }

AudioInfo::AudioInfo(QUrl url, const QJsonObject& infoJson)
    : FileInfo(url, infoJson)
    , duration(infoJson["duration"].toInt())
{ }

Location::Location(const QJsonObject& json)
    : geoUri(json["geo_uri"].toString())
    , thumbnail(json["thumbnail_url"].toString(),
                json["thumbnail_info"].toObject())
{ }
