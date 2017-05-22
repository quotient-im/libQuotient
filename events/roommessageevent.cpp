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

#include <QtCore/QMimeDatabase>

using namespace QMatrixClient;
using namespace MessageEventContent;

using ContentPair = std::pair<CType, Base*>;

template <CType EnumType, typename ContentT>
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
    return { CType::Video, c };
};

ContentPair makeUnknown(const QJsonObject& json)
{
    qCDebug(EVENTS) << "RoomMessageEvent: couldn't resolve msgtype, JSON follows:";
    qCDebug(EVENTS) << json;
    return { CType::Unknown, new Base() };
}

RoomMessageEvent::RoomMessageEvent(const QJsonObject& obj)
    : RoomEvent(Type::RoomMessage, obj), _msgtype(CType::Unknown)
    , _content(nullptr)
{
    const QJsonObject content = contentJson();
    if ( content.contains("msgtype") && content.contains("body") )
    {
        _plainBody = content["body"].toString();

        auto factory = lookup(content["msgtype"].toString(),
                            "m.text", make<CType::Text, TextContent>,
                            "m.emote", make<CType::Emote, TextContent>,
                            "m.notice", make<CType::Notice, TextContent>,
                            "m.image", make<CType::Image, ImageContent>,
                            "m.file", make<CType::File, FileContent>,
                            "m.location", make<CType::Location, LocationContent>,
                            "m.video", makeVideo,
                            "m.audio", make<CType::Audio, AudioContent>,
                            // Insert new message types before this line
                            makeUnknown
                        );
        std::tie(_msgtype, _content) = factory(content);
    }
    else
    {
        qCWarning(EVENTS) << "No body or msgtype in room message event";
        qCWarning(EVENTS) << formatJson << obj;
    }
}

RoomMessageEvent::~RoomMessageEvent()
{
    if (_content)
        delete _content;
}

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
