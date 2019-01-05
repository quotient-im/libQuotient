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
#include <QtCore/QFileInfo>
#include <QtGui/QImageReader>
#include <QtMultimedia/QMediaResource>

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
    QString matrixType;
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
        return it->matrixType;

    return {};
}

MsgType jsonToMsgType(const QString& matrixType)
{
    auto it = std::find_if(msgTypes.begin(), msgTypes.end(),
        [=](const MsgTypeDesc& mtd) { return mtd.matrixType == matrixType; });
    if (it != msgTypes.end())
        return it->enumType;

    return MsgType::Unknown;
}

QJsonObject RoomMessageEvent::assembleContentJson(const QString& plainBody,
                const QString& jsonMsgType, TypedBase* content)
{
    auto json = content ? content->toJson() : QJsonObject();
    json.insert(QStringLiteral("msgtype"), jsonMsgType);
    json.insert(QStringLiteral("body"), plainBody);
    return json;
}

static const auto MsgTypeKey = "msgtype"_ls;
static const auto BodyKey = "body"_ls;

RoomMessageEvent::RoomMessageEvent(const QString& plainBody,
        const QString& jsonMsgType, TypedBase* content)
    : RoomEvent(typeId(), matrixTypeId(),
                assembleContentJson(plainBody, jsonMsgType, content))
    , _content(content)
{ }

RoomMessageEvent::RoomMessageEvent(const QString& plainBody,
                                   MsgType msgType, TypedBase* content)
    : RoomMessageEvent(plainBody, msgTypeToJson(msgType), content)
{ }

TypedBase* contentFromFile(const QFileInfo& file, bool asGenericFile)
{
    auto filePath = file.absoluteFilePath();
    auto localUrl = QUrl::fromLocalFile(filePath);
    auto mimeType = QMimeDatabase().mimeTypeForFile(file);
    if (!asGenericFile)
    {
        auto mimeTypeName = mimeType.name();
        if (mimeTypeName.startsWith("image/"))
            return new ImageContent(localUrl, file.size(), mimeType,
                                    QImageReader(filePath).size(),
                                    file.fileName());

        // duration can only be obtained asynchronously and can only be reliably
        // done by starting to play the file. Left for a future implementation.
        if (mimeTypeName.startsWith("video/"))
            return new VideoContent(localUrl, file.size(), mimeType,
                                    QMediaResource(localUrl).resolution(),
                                    file.fileName());

        if (mimeTypeName.startsWith("audio/"))
            return new AudioContent(localUrl, file.size(), mimeType,
                                    file.fileName());
    }
    return new FileContent(localUrl, file.size(), mimeType, file.fileName());
}

RoomMessageEvent::RoomMessageEvent(const QString& plainBody,
        const QFileInfo& file, bool asGenericFile)
    : RoomMessageEvent(plainBody,
        asGenericFile ? QStringLiteral("m.file") : rawMsgTypeForFile(file),
        contentFromFile(file, asGenericFile))
{ }

RoomMessageEvent::RoomMessageEvent(const QJsonObject& obj)
    : RoomEvent(typeId(), obj), _content(nullptr)
{
    if (isRedacted())
        return;
    const QJsonObject content = contentJson();
    if ( content.contains(MsgTypeKey) && content.contains(BodyKey) )
    {
        auto msgtype = content[MsgTypeKey].toString();
        for (const auto& mt: msgTypes)
            if (mt.matrixType == msgtype)
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
    return jsonToMsgType(rawMsgtype());
}

QString RoomMessageEvent::rawMsgtype() const
{
    return contentJson()[MsgTypeKey].toString();
}

QString RoomMessageEvent::plainBody() const
{
    return contentJson()[BodyKey].toString();
}

QMimeType RoomMessageEvent::mimeType() const
{
    static const auto PlainTextMimeType =
            QMimeDatabase().mimeTypeForName("text/plain");
    return _content ? _content->type() : PlainTextMimeType;
                      ;
}

bool RoomMessageEvent::hasTextContent() const
{
    return content() &&
        (msgtype() == MsgType::Text || msgtype() == MsgType::Emote ||
         msgtype() == MsgType::Notice); // FIXME: Unbind from specific msgtypes
}

bool RoomMessageEvent::hasFileContent() const
{
    return content() && content()->fileInfo();
}

bool RoomMessageEvent::hasThumbnail() const
{
    return content() && content()->thumbnailInfo();
}

QString rawMsgTypeForMimeType(const QMimeType& mimeType)
{
    auto name = mimeType.name();
    return name.startsWith("image/") ? QStringLiteral("m.image") :
            name.startsWith("video/") ? QStringLiteral("m.video") :
            name.startsWith("audio/") ? QStringLiteral("m.audio") :
            QStringLiteral("m.file");
}

QString RoomMessageEvent::rawMsgTypeForUrl(const QUrl& url)
{
    return rawMsgTypeForMimeType(QMimeDatabase().mimeTypeForUrl(url));
}

QString RoomMessageEvent::rawMsgTypeForFile(const QFileInfo& fi)
{
    return rawMsgTypeForMimeType(QMimeDatabase().mimeTypeForFile(fi));
}

TextContent::TextContent(const QString& text, const QString& contentType)
    : mimeType(QMimeDatabase().mimeTypeForName(contentType)), body(text)
{
    if (contentType == "org.matrix.custom.html")
        mimeType = QMimeDatabase().mimeTypeForName("text/html");
}

TextContent::TextContent(const QJsonObject& json)
{
    QMimeDatabase db;
    static const auto PlainTextMimeType = db.mimeTypeForName("text/plain");
    static const auto HtmlMimeType = db.mimeTypeForName("text/html");

    // Special-casing the custom matrix.org's (actually, Riot's) way
    // of sending HTML messages.
    if (json["format"_ls].toString() == "org.matrix.custom.html")
    {
        mimeType = HtmlMimeType;
        body = json["formatted_body"_ls].toString();
    } else {
        // Falling back to plain text, as there's no standard way to describe
        // rich text in messages.
        mimeType = PlainTextMimeType;
        body = json[BodyKey].toString();
    }
}

void TextContent::fillJson(QJsonObject* json) const
{
    Q_ASSERT(json);
    if (mimeType.inherits("text/html"))
    {
        json->insert(QStringLiteral("format"),
                     QStringLiteral("org.matrix.custom.html"));
        json->insert(QStringLiteral("formatted_body"), body);
    }
}

LocationContent::LocationContent(const QString& geoUri,
                                 const Thumbnail& thumbnail)
    : geoUri(geoUri), thumbnail(thumbnail)
{ }

LocationContent::LocationContent(const QJsonObject& json)
    : TypedBase(json)
    , geoUri(json["geo_uri"_ls].toString())
    , thumbnail(json["info"_ls].toObject())
{ }

QMimeType LocationContent::type() const
{
    return QMimeDatabase().mimeTypeForData(geoUri.toLatin1());
}

void LocationContent::fillJson(QJsonObject* o) const
{
    Q_ASSERT(o);
    o->insert(QStringLiteral("geo_uri"), geoUri);
    o->insert(QStringLiteral("info"), toInfoJson(thumbnail));
}
