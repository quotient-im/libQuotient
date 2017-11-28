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
using namespace EventContent;

using MsgType = RoomMessageEvent::MsgType;

template <typename ContentT>
TypedBase* make(const QJsonObject& json)
{
    return new ContentT(json);
}

struct MsgTypeDesc
{
    QString jsonType;
    MsgType enumType;
    TypedBase* (*maker)(const QJsonObject&);
};

const std::vector<MsgTypeDesc> msgTypes =
    { { QStringLiteral("m.text"), MsgType::Text, make<TextContent> }
    , { QStringLiteral("m.emote"), MsgType::Emote, make<TextContent> }
    , { QStringLiteral("m.notice"), MsgType::Notice, make<TextContent> }
    , { QStringLiteral("m.image"), MsgType::Image, make<ImageContent> }
    , { QStringLiteral("m.file"), MsgType::File, make<FileContent> }
    , { QStringLiteral("m.location"), MsgType::Location, make<LocationContent> }
    , { QStringLiteral("m.video"), MsgType::Video, make<VideoContent> }
    , { QStringLiteral("m.audio"), MsgType::Audio, make<AudioContent> }
    };

QString msgTypeToJson(MsgType enumType)
{
    auto it = std::find_if(msgTypes.begin(), msgTypes.end(),
        [=](const MsgTypeDesc& mtd) { return mtd.enumType == enumType; });
    if (it != msgTypes.end())
        return it->jsonType;

    qCCritical(EVENTS) << "Unknown msgtype:" << enumType;
    return {};
}

MsgType jsonToMsgType(const QString& jsonType)
{
    auto it = std::find_if(msgTypes.begin(), msgTypes.end(),
        [=](const MsgTypeDesc& mtd) { return mtd.jsonType == jsonType; });
    if (it != msgTypes.end())
        return it->enumType;

    qCCritical(EVENTS) << "Unknown msgtype:" << jsonType;
    return {};
}

RoomMessageEvent::RoomMessageEvent(const QString& plainBody,
                                   MsgType msgType, TypedBase* content)
    : RoomMessageEvent(plainBody, msgTypeToJson(msgType), content)
{ }

RoomMessageEvent::RoomMessageEvent(const QJsonObject& obj)
    : RoomEvent(Type::RoomMessage, obj), _content(nullptr)
{
    const QJsonObject content = contentJson();
    if ( content.contains("msgtype") && content.contains("body") )
    {
        _plainBody = content["body"].toString();

        _msgtype = content["msgtype"].toString();
        for (auto mt: msgTypes)
            if (mt.jsonType == _msgtype)
                _content.reset(mt.maker(content));

        if (!_content)
        {
            qCWarning(EVENTS) << "RoomMessageEvent: couldn't load content,"
                              << " full content dump follows";
            qCWarning(EVENTS) << formatJson << content;
        }
    }
    else
    {
        qCWarning(EVENTS) << "No body or msgtype in room message event";
        qCWarning(EVENTS) << formatJson << obj;
    }
}

RoomMessageEvent::MsgType RoomMessageEvent::msgtype() const
{
    return jsonToMsgType(_msgtype);
}

QMimeType RoomMessageEvent::mimeType() const
{
    return _content ? _content->type() :
                      QMimeDatabase().mimeTypeForName("text/plain");
}

QJsonObject RoomMessageEvent::toJson() const
{
    QJsonObject obj = _content ? _content->toJson() : QJsonObject();
    obj.insert("msgtype", msgTypeToJson(msgtype()));
    obj.insert("body", plainBody());
    return obj;
}

TextContent::TextContent(const QString& text, const QString& contentType)
    : mimeType(QMimeDatabase().mimeTypeForName(contentType)), body(text)
{ }

TextContent::TextContent(const QJsonObject& json)
{
    QMimeDatabase db;

    // Special-casing the custom matrix.org's (actually, Riot's) way
    // of sending HTML messages.
    if (json["format"].toString() == "org.matrix.custom.html")
    {
        mimeType = db.mimeTypeForName("text/html");
        body = json["formatted_body"].toString();
    } else {
        // Falling back to plain text, as there's no standard way to describe
        // rich text in messages.
        mimeType = db.mimeTypeForName("text/plain");
        body = json["body"].toString();
    }
}

void TextContent::fillJson(QJsonObject* json) const
{
    Q_ASSERT(json);
    json->insert("format", QStringLiteral("org.matrix.custom.html"));
    json->insert("formatted_body", body);
}

LocationContent::LocationContent(const QString& geoUri,
                                 const ImageInfo<>& thumbnail)
    : Thumbnailed<>(thumbnail), geoUri(geoUri)
{ }

LocationContent::LocationContent(const QJsonObject& json)
    : Thumbnailed<>(json["info"].toObject())
    , geoUri(json["geo_uri"].toString())
{ }

void LocationContent::fillJson(QJsonObject* o) const
{
    Q_ASSERT(o);
    o->insert("geo_uri", geoUri);
    o->insert("info", Thumbnailed::toInfoJson());
}

QMimeType LocationContent::type() const
{
    return QMimeDatabase().mimeTypeForData(geoUri.toLatin1());
}

PlayableInfo::PlayableInfo(const QUrl& u, int fileSize,
                           const QMimeType& mimeType, int duration,
                           const QString& originalFilename)
    : FileInfo(u, fileSize, mimeType, originalFilename)
    , duration(duration)
{ }

PlayableInfo::PlayableInfo(const QUrl& u, const QJsonObject& infoJson,
                           const QString& originalFilename)
    : FileInfo(u, infoJson, originalFilename)
    , duration(infoJson["duration"].toInt())
{ }

void PlayableInfo::fillInfoJson(QJsonObject* infoJson) const
{
    FileInfo::fillInfoJson(infoJson);
    infoJson->insert("duration", duration);
}
