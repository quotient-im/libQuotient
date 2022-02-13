// SPDX-FileCopyrightText: 2015 Felix Rohrbach <kde@fxrh.de>
// SPDX-FileCopyrightText: 2016 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-FileCopyrightText: 2017 Roman Plášil <me@rplasil.name>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "roommessageevent.h"

#include "logging.h"
#include "events/eventrelation.h"

#include <QtCore/QFileInfo>
#include <QtCore/QMimeDatabase>
#include <QtGui/QImageReader>
#if QT_VERSION_MAJOR < 6
#    include <QtMultimedia/QMediaResource>
#endif

using namespace Quotient;
using namespace EventContent;

using MsgType = RoomMessageEvent::MsgType;

namespace { // Supporting internal definitions
constexpr auto RelatesToKey = "m.relates_to"_ls;
constexpr auto MsgTypeKey = "msgtype"_ls;
constexpr auto FormattedBodyKey = "formatted_body"_ls;
constexpr auto TextTypeKey = "m.text"_ls;
constexpr auto EmoteTypeKey = "m.emote"_ls;
constexpr auto NoticeTypeKey = "m.notice"_ls;
constexpr auto HtmlContentTypeId = "org.matrix.custom.html"_ls;

template <typename ContentT>
TypedBase* make(const QJsonObject& json)
{
    return new ContentT(json);
}

template <>
TypedBase* make<TextContent>(const QJsonObject& json)
{
    return json.contains(FormattedBodyKey) || json.contains(RelatesToKey)
               ? new TextContent(json)
               : nullptr;
}

struct MsgTypeDesc {
    QLatin1String matrixType;
    MsgType enumType;
    TypedBase* (*maker)(const QJsonObject&);
};

const std::vector<MsgTypeDesc> msgTypes = {
    { TextTypeKey, MsgType::Text, make<TextContent> },
    { EmoteTypeKey, MsgType::Emote, make<TextContent> },
    { NoticeTypeKey, MsgType::Notice, make<TextContent> },
    { "m.image"_ls, MsgType::Image, make<ImageContent> },
    { "m.file"_ls, MsgType::File, make<FileContent> },
    { "m.location"_ls, MsgType::Location, make<LocationContent> },
    { "m.video"_ls, MsgType::Video, make<VideoContent> },
    { "m.audio"_ls, MsgType::Audio, make<AudioContent> }
};

QString msgTypeToJson(MsgType enumType)
{
    auto it = std::find_if(msgTypes.begin(), msgTypes.end(),
                           [=](const MsgTypeDesc& mtd) {
                               return mtd.enumType == enumType;
                           });
    if (it != msgTypes.end())
        return it->matrixType;

    return {};
}

MsgType jsonToMsgType(const QString& matrixType)
{
    auto it = std::find_if(msgTypes.begin(), msgTypes.end(),
                           [=](const MsgTypeDesc& mtd) {
                               return mtd.matrixType == matrixType;
                           });
    if (it != msgTypes.end())
        return it->enumType;

    return MsgType::Unknown;
}

inline bool isReplacement(const Omittable<EventRelation>& rel)
{
    return rel && rel->type == EventRelation::ReplacementType;
}

} // anonymous namespace

QJsonObject RoomMessageEvent::assembleContentJson(const QString& plainBody,
                                                  const QString& jsonMsgType,
                                                  TypedBase* content)
{
    QJsonObject json;
    if (content) {
        // TODO: replace with content->fillJson(json) when it starts working
        json = content->toJson();
        if (jsonMsgType != TextTypeKey && jsonMsgType != NoticeTypeKey
            && jsonMsgType != EmoteTypeKey) {
            if (json.contains(RelatesToKey)) {
                json.remove(RelatesToKey);
                qCWarning(EVENTS)
                    << RelatesToKey << "cannot be used in" << jsonMsgType
                    << "messages; the relation has been stripped off";
            }
        } else if (auto* textContent = static_cast<const TextContent*>(content);
                   textContent->relatesTo
                   && textContent->relatesTo->type
                          == EventRelation::ReplacementType) {
            auto newContentJson = json.take("m.new_content"_ls).toObject();
            newContentJson.insert(BodyKey, plainBody);
            newContentJson.insert(MsgTypeKey, jsonMsgType);
            json.insert(QStringLiteral("m.new_content"), newContentJson);
            json[MsgTypeKey] = jsonMsgType;
            json[BodyKeyL] = "* " + plainBody;
            return json;
        }
    }
    json.insert(MsgTypeKey, jsonMsgType);
    json.insert(BodyKey, plainBody);
    return json;
}

RoomMessageEvent::RoomMessageEvent(const QString& plainBody,
                                   const QString& jsonMsgType,
                                   TypedBase* content)
    : RoomEvent(typeId(), matrixTypeId(),
                assembleContentJson(plainBody, jsonMsgType, content))
    , _content(content)
{}

RoomMessageEvent::RoomMessageEvent(const QString& plainBody, MsgType msgType,
                                   TypedBase* content)
    : RoomMessageEvent(plainBody, msgTypeToJson(msgType), content)
{}

#if QT_VERSION_MAJOR < 6
TypedBase* contentFromFile(const QFileInfo& file, bool asGenericFile)
{
    auto filePath = file.absoluteFilePath();
    auto localUrl = QUrl::fromLocalFile(filePath);
    auto mimeType = QMimeDatabase().mimeTypeForFile(file);
    if (!asGenericFile) {
        auto mimeTypeName = mimeType.name();
        if (mimeTypeName.startsWith("image/"))
            return new ImageContent(localUrl, file.size(), mimeType,
                                    QImageReader(filePath).size(), none,
                                    file.fileName());

        // duration can only be obtained asynchronously and can only be reliably
        // done by starting to play the file. Left for a future implementation.
        if (mimeTypeName.startsWith("video/"))
            return new VideoContent(localUrl, file.size(), mimeType,
                                    QMediaResource(localUrl).resolution(), none,
                                    file.fileName());

        if (mimeTypeName.startsWith("audio/"))
            return new AudioContent(localUrl, file.size(), mimeType, none,
                                    file.fileName());
    }
    return new FileContent(localUrl, file.size(), mimeType, none, file.fileName());
}

RoomMessageEvent::RoomMessageEvent(const QString& plainBody,
                                   const QFileInfo& file, bool asGenericFile)
    : RoomMessageEvent(plainBody,
                       asGenericFile ? QStringLiteral("m.file")
                                     : rawMsgTypeForFile(file),
                       contentFromFile(file, asGenericFile))
{}
#endif

RoomMessageEvent::RoomMessageEvent(const QJsonObject& obj)
    : RoomEvent(typeId(), obj), _content(nullptr)
{
    if (isRedacted())
        return;
    const QJsonObject content = contentJson();
    if (content.contains(MsgTypeKey) && content.contains(BodyKeyL)) {
        auto msgtype = content[MsgTypeKey].toString();
        bool msgTypeFound = false;
        for (const auto& mt : msgTypes)
            if (mt.matrixType == msgtype) {
                _content.reset(mt.maker(content));
                msgTypeFound = true;
            }

        if (!msgTypeFound) {
            qCWarning(EVENTS) << "RoomMessageEvent: unknown msg_type,"
                              << " full content dump follows";
            qCWarning(EVENTS) << formatJson << content;
        }
    } else {
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
    return contentPart<QString>(MsgTypeKey);
}

QString RoomMessageEvent::plainBody() const
{
    return contentPart<QString>(BodyKeyL);
}

QMimeType RoomMessageEvent::mimeType() const
{
    static const auto PlainTextMimeType =
        QMimeDatabase().mimeTypeForName("text/plain");
    return _content ? _content->type() : PlainTextMimeType;
}

bool RoomMessageEvent::hasTextContent() const
{
    return !content()
           || (msgtype() == MsgType::Text || msgtype() == MsgType::Emote
               || msgtype() == MsgType::Notice);
}

bool RoomMessageEvent::hasFileContent() const
{
    return content() && content()->fileInfo();
}

bool RoomMessageEvent::hasThumbnail() const
{
    return content() && content()->thumbnailInfo();
}

QString RoomMessageEvent::replacedEvent() const
{
    if (!content() || !hasTextContent())
        return {};

    const auto& rel = static_cast<const TextContent*>(content())->relatesTo;
    return isReplacement(rel) ? rel->eventId : QString();
}

QString rawMsgTypeForMimeType(const QMimeType& mimeType)
{
    auto name = mimeType.name();
    return name.startsWith("image/")
               ? QStringLiteral("m.image")
               : name.startsWith("video/")
                     ? QStringLiteral("m.video")
                     : name.startsWith("audio/") ? QStringLiteral("m.audio")
                                                 : QStringLiteral("m.file");
}

QString RoomMessageEvent::rawMsgTypeForUrl(const QUrl& url)
{
    return rawMsgTypeForMimeType(QMimeDatabase().mimeTypeForUrl(url));
}

QString RoomMessageEvent::rawMsgTypeForFile(const QFileInfo& fi)
{
    return rawMsgTypeForMimeType(QMimeDatabase().mimeTypeForFile(fi));
}

TextContent::TextContent(QString text, const QString& contentType,
                         Omittable<EventRelation> relatesTo)
    : mimeType(QMimeDatabase().mimeTypeForName(contentType))
    , body(std::move(text))
    , relatesTo(std::move(relatesTo))
{
    if (contentType == HtmlContentTypeId)
        mimeType = QMimeDatabase().mimeTypeForName("text/html");
}

TextContent::TextContent(const QJsonObject& json)
    : relatesTo(fromJson<Omittable<EventRelation>>(json[RelatesToKey]))
{
    QMimeDatabase db;
    static const auto PlainTextMimeType = db.mimeTypeForName("text/plain");
    static const auto HtmlMimeType = db.mimeTypeForName("text/html");

    const auto actualJson = isReplacement(relatesTo)
                                ? json.value("m.new_content"_ls).toObject()
                                : json;
    // Special-casing the custom matrix.org's (actually, Element's) way
    // of sending HTML messages.
    if (actualJson["format"_ls].toString() == HtmlContentTypeId) {
        mimeType = HtmlMimeType;
        body = actualJson[FormattedBodyKey].toString();
    } else {
        // Falling back to plain text, as there's no standard way to describe
        // rich text in messages.
        mimeType = PlainTextMimeType;
        body = actualJson[BodyKeyL].toString();
    }
}

void TextContent::fillJson(QJsonObject* json) const
{
    static const auto FormatKey = QStringLiteral("format");

    Q_ASSERT(json);
    if (mimeType.inherits("text/html")) {
        json->insert(FormatKey, HtmlContentTypeId);
        json->insert(FormattedBodyKey, body);
    }
    if (relatesTo) {
        json->insert(
            QStringLiteral("m.relates_to"),
            relatesTo->type == EventRelation::ReplyType
                ? QJsonObject { { relatesTo->type,
                                  QJsonObject {
                                      { EventIdKey, relatesTo->eventId } } } }
                : QJsonObject { { RelTypeKey, relatesTo->type },
                                { EventIdKey, relatesTo->eventId } });
        if (relatesTo->type == EventRelation::ReplacementType) {
            QJsonObject newContentJson;
            if (mimeType.inherits("text/html")) {
                newContentJson.insert(FormatKey, HtmlContentTypeId);
                newContentJson.insert(FormattedBodyKey, body);
            }
            json->insert(QStringLiteral("m.new_content"), newContentJson);
        }
    }
}

LocationContent::LocationContent(const QString& geoUri,
                                 const Thumbnail& thumbnail)
    : geoUri(geoUri), thumbnail(thumbnail)
{}

LocationContent::LocationContent(const QJsonObject& json)
    : TypedBase(json)
    , geoUri(json["geo_uri"_ls].toString())
    , thumbnail(json["info"_ls].toObject())
{}

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
