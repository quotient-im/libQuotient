// SPDX-FileCopyrightText: 2015 Felix Rohrbach <kde@fxrh.de>
// SPDX-FileCopyrightText: 2016 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-FileCopyrightText: 2017 Roman Plášil <me@rplasil.name>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "roommessageevent.h"

#include "../logging_categories_p.h"
#include "eventrelation.h"

#include <QtCore/QFileInfo>
#include <QtCore/QMimeDatabase>
#include <QtGui/QImageReader>
#include <QtCore/QStringBuilder>

using namespace Quotient;
using namespace EventContent;

using MsgType = RoomMessageEvent::MsgType;

namespace { // Supporting internal definitions
constexpr auto RelatesToKey = "m.relates_to"_L1;
constexpr auto MsgTypeKey = "msgtype"_L1;
constexpr auto FormattedBodyKey = "formatted_body"_L1;
constexpr auto TextTypeKey = "m.text"_L1;
constexpr auto EmoteTypeKey = "m.emote"_L1;
constexpr auto NoticeTypeKey = "m.notice"_L1;
constexpr auto HtmlContentTypeId = "org.matrix.custom.html"_L1;

template <typename ContentT>
std::unique_ptr<TypedBase> make(const QJsonObject& json)
{
    return std::make_unique<ContentT>(json);
}

template <>
std::unique_ptr<TypedBase> make<TextContent>(const QJsonObject& json)
{
    return json.contains(FormattedBodyKey) || json.contains(RelatesToKey)
               ? std::make_unique<TextContent>(json)
               : nullptr;
}

struct MsgTypeDesc {
    QLatin1String matrixType;
    MsgType enumType;
    std::unique_ptr<TypedBase> (*maker)(const QJsonObject&);
};

constexpr auto msgTypes = std::to_array<MsgTypeDesc>({
    { TextTypeKey, MsgType::Text, make<TextContent> },
    { EmoteTypeKey, MsgType::Emote, make<TextContent> },
    { NoticeTypeKey, MsgType::Notice, make<TextContent> },
    { "m.image"_L1, MsgType::Image, make<ImageContent> },
    { "m.file"_L1, MsgType::File, make<FileContent> },
    { "m.location"_L1, MsgType::Location, make<LocationContent> },
    { "m.video"_L1, MsgType::Video, make<VideoContent> },
    { "m.audio"_L1, MsgType::Audio, make<AudioContent> },
    { "m.key.verification.request"_L1 , MsgType::Text, make<TextContent> },
});

QString msgTypeToJson(MsgType enumType)
{
    if (auto it = std::ranges::find(msgTypes, enumType, &MsgTypeDesc::enumType);
        it != msgTypes.end())
        return it->matrixType;

    return {};
}

MsgType jsonToMsgType(const QString& matrixType)
{
    if (auto it = std::ranges::find(msgTypes, matrixType, &MsgTypeDesc::matrixType);
        it != msgTypes.end())
        return it->enumType;

    return MsgType::Unknown;
}

inline bool isReplacement(const std::optional<EventRelation>& rel)
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
            auto newContentJson = json.take("m.new_content"_L1).toObject();
            newContentJson.insert(BodyKey, plainBody);
            newContentJson.insert(MsgTypeKey, jsonMsgType);
            json.insert("m.new_content"_L1, newContentJson);
            json[MsgTypeKey] = jsonMsgType;
            json[BodyKey] = "* "_L1 + plainBody;
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
    : RoomEvent(
        basicJson(TypeId, assembleContentJson(plainBody, jsonMsgType, content)))
    , _content(content)
{}

RoomMessageEvent::RoomMessageEvent(const QString& plainBody, MsgType msgType,
                                   TypedBase* content)
    : RoomMessageEvent(plainBody, msgTypeToJson(msgType), content)
{}

RoomMessageEvent::RoomMessageEvent(const QJsonObject& obj)
    : RoomEvent(obj), _content(nullptr)
{
    if (isRedacted())
        return;
    const QJsonObject content = contentJson();
    if (content.contains(MsgTypeKey) && content.contains(BodyKey)) {
        auto msgtype = content[MsgTypeKey].toString();
        bool msgTypeFound = false;
        for (const auto& mt : msgTypes)
            if (mt.matrixType == msgtype) {
                _content = mt.maker(content);
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
    return contentPart<QString>(BodyKey);
}

QMimeType RoomMessageEvent::mimeType() const
{
    static const auto PlainTextMimeType =
        QMimeDatabase().mimeTypeForName("text/plain"_L1);
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

std::optional<EventRelation> RoomMessageEvent::relatesTo() const
{
    return content() && hasTextContent() ? static_cast<const TextContent*>(content())->relatesTo : std::nullopt;
}

QString RoomMessageEvent::upstreamEventId() const
{
    const auto relation = relatesTo();
    return relation ? relation.value().eventId : QString();
}

QString RoomMessageEvent::replacedEvent() const
{
    if (!content() || !hasTextContent())
        return {};

    const auto& rel = static_cast<const TextContent*>(content())->relatesTo;
    return isReplacement(rel) ? rel->eventId : QString();
}

bool RoomMessageEvent::isReplaced() const
{
    return unsignedPart<QJsonObject>("m.relations"_L1).contains("m.replace"_L1);
}

QString RoomMessageEvent::replacedBy() const
{
    return unsignedPart<QJsonObject>("m.relations"_L1)["m.replace"_L1][EventIdKey].toString();
}

bool RoomMessageEvent::isReply(bool includeFallbacks) const
{
    const auto relation = relatesTo();
    return relation.has_value() &&
            (relation.value().type == EventRelation::ReplyType ||
            (relation.value().type == EventRelation::ThreadType &&
            (relation.value().isFallingBack == false || includeFallbacks)));
}

QString RoomMessageEvent::replyEventId(bool includeFallbacks) const
{
    const auto relation = relatesTo();
    if (relation.has_value()) {
        if (relation.value().type == EventRelation::ReplyType) {
            return relation.value().eventId;
        } else if (relation.value().type == EventRelation::ThreadType &&
                (relation.value().isFallingBack == false || includeFallbacks)) {
            return relation.value().inThreadReplyEventId;
        }
    }
    return {};
}

bool RoomMessageEvent::isThreaded() const
{
    const auto relation = relatesTo();
    return (relation && relation.value().type == EventRelation::ThreadType)
            || unsignedPart<QJsonObject>("m.relations"_ls).contains(EventRelation::ThreadType);
}

QString RoomMessageEvent::threadRootEventId() const
{
    const auto relation = relatesTo();
    if (relation && relation.value().type == EventRelation::ThreadType) {
        return relation.value().eventId;
    } else {
        return unsignedPart<QJsonObject>("m.relations"_ls)[EventRelation::ThreadType].toString();
    }
}

namespace {
QString safeFileName(QString rawName)
{
    static auto safeFileNameRegex = QRegularExpression(uR"([/\<>|"*?:])"_s);
    return rawName.replace(safeFileNameRegex, "_"_L1);
}
}

QString RoomMessageEvent::fileNameToDownload() const
{
    Q_ASSERT(hasFileContent());
    const auto* fileInfo = content()->fileInfo();
    QString fileName;
    if (!fileInfo->originalName.isEmpty())
        fileName = QFileInfo(safeFileName(fileInfo->originalName)).fileName();
    else if (QUrl u { plainBody() }; u.isValid()) {
        qDebug(MAIN) << id()
                     << "has no file name supplied but the event body "
                        "looks like a URL - using the file name from it";
        fileName = u.fileName();
    }
    if (fileName.isEmpty())
        return safeFileName(fileInfo->mediaId()).replace(u'.', u'-') % u'.'
               % fileInfo->mimeType.preferredSuffix();

    if (QSysInfo::productType() == "windows"_L1) {
        if (const auto& suffixes = fileInfo->mimeType.suffixes();
            !suffixes.isEmpty() && std::ranges::none_of(suffixes, [&fileName](const QString& s) {
                return fileName.endsWith(s);
            }))
            return fileName % u'.' % fileInfo->mimeType.preferredSuffix();
    }
    return fileName;
}

QString rawMsgTypeForMimeType(const QMimeType& mimeType)
{
    auto name = mimeType.name();
    return name.startsWith("image/"_L1)   ? u"m.image"_s
           : name.startsWith("video/"_L1) ? u"m.video"_s
           : name.startsWith("audio/"_L1) ? u"m.audio"_s
                                          : u"m.file"_s;
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
                         std::optional<EventRelation> relatesTo)
    : mimeType(QMimeDatabase().mimeTypeForName(contentType))
    , body(std::move(text))
    , relatesTo(std::move(relatesTo))
{
    if (contentType == HtmlContentTypeId)
        mimeType = QMimeDatabase().mimeTypeForName("text/html"_L1);
}

TextContent::TextContent(const QJsonObject& json)
    : relatesTo(fromJson<std::optional<EventRelation>>(json[RelatesToKey]))
{
    QMimeDatabase db;
    static const auto PlainTextMimeType = db.mimeTypeForName("text/plain"_L1);
    static const auto HtmlMimeType = db.mimeTypeForName("text/html"_L1);

    const auto actualJson = isReplacement(relatesTo)
                                ? json.value("m.new_content"_L1).toObject()
                                : json;
    // Special-casing the custom matrix.org's (actually, Element's) way
    // of sending HTML messages.
    if (actualJson["format"_L1].toString() == HtmlContentTypeId) {
        mimeType = HtmlMimeType;
        body = actualJson[FormattedBodyKey].toString();
    } else {
        // Falling back to plain text, as there's no standard way to describe
        // rich text in messages.
        mimeType = PlainTextMimeType;
        body = actualJson[BodyKey].toString();
    }
}

void TextContent::fillJson(QJsonObject &json) const
{
    static const auto FormatKey = "format"_L1;

    if (mimeType.inherits("text/html"_L1)) {
        json.insert(FormatKey, HtmlContentTypeId);
        json.insert(FormattedBodyKey, body);
    }
    if (relatesTo) {
        json.insert("m.relates_to"_L1,
                    relatesTo->type == EventRelation::ReplyType
                        ? QJsonObject{ { relatesTo->type,
                                         QJsonObject{ { EventIdKey, relatesTo->eventId } } } }
                        : QJsonObject{ { RelTypeKey, relatesTo->type },
                                       { EventIdKey, relatesTo->eventId } });
        if (relatesTo->type == EventRelation::ReplacementType) {
            QJsonObject newContentJson;
            if (mimeType.inherits("text/html"_L1)) {
                newContentJson.insert(FormatKey, HtmlContentTypeId);
                newContentJson.insert(FormattedBodyKey, body);
            }
            json.insert("m.new_content"_L1, newContentJson);
        }
    }
}

LocationContent::LocationContent(const QString& geoUri,
                                 const Thumbnail& thumbnail)
    : geoUri(geoUri), thumbnail(thumbnail)
{}

LocationContent::LocationContent(const QJsonObject& json)
    : TypedBase(json)
    , geoUri(json["geo_uri"_L1].toString())
    , thumbnail(json["info"_L1].toObject())
{}

QMimeType LocationContent::type() const
{
    return QMimeDatabase().mimeTypeForData(geoUri.toLatin1());
}

void LocationContent::fillJson(QJsonObject& o) const
{
    o.insert("geo_uri"_L1, geoUri);
    o.insert("info"_L1, toInfoJson(thumbnail));
}
