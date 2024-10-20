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
constexpr auto FormatKey = "format"_L1;
constexpr auto TextTypeId = "m.text"_L1;
constexpr auto EmoteTypeId = "m.emote"_L1;
constexpr auto NoticeTypeId = "m.notice"_L1;
constexpr auto FileTypeId = "m.file"_L1;
constexpr auto ImageTypeId = "m.image"_L1;
constexpr auto AudioTypeId = "m.audio"_L1;
constexpr auto VideoTypeId = "m.video"_L1;
constexpr auto LocationTypeId = "m.location"_L1;
constexpr auto HtmlContentTypeId = "org.matrix.custom.html"_L1;

template <typename ContentT>
std::unique_ptr<Base> make(const QJsonObject& json)
{
    return !json.isEmpty() ? std::make_unique<ContentT>(json) : nullptr;
}

template <>
std::unique_ptr<Base> make<TextContent>(const QJsonObject& json)
{
    return json.contains(FormattedBodyKey) || json.contains(RelatesToKey)
               ? std::make_unique<TextContent>(json)
               : nullptr;
}

struct MsgTypeDesc {
    QLatin1String matrixType;
    MsgType enumType;
    bool fileBased;
    std::unique_ptr<Base> (*maker)(const QJsonObject&);
};

constexpr auto msgTypes = std::to_array<MsgTypeDesc>({
    { TextTypeId, MsgType::Text, false, make<TextContent> },
    { EmoteTypeId, MsgType::Emote, false, make<TextContent> },
    { NoticeTypeId, MsgType::Notice, false, make<TextContent> },
    { ImageTypeId, MsgType::Image, true, make<ImageContent> },
    { FileTypeId, MsgType::File, true, make<FileContent> },
    { LocationTypeId, MsgType::Location, false, make<LocationContent> },
    { VideoTypeId, MsgType::Video, true, make<VideoContent> },
    { AudioTypeId, MsgType::Audio, true, make<AudioContent> },
    { "m.key.verification.request"_L1, MsgType::Text, false, make<TextContent> },
});

QString msgTypeToJson(MsgType enumType)
{
    if (auto it = std::ranges::find(msgTypes, enumType, &MsgTypeDesc::enumType);
        it != msgTypes.end())
        return it->matrixType;

    return {};
}

MsgTypeDesc jsonToMsgTypeDesc(const QString& matrixType)
{
    if (auto it = std::ranges::find(msgTypes, matrixType, &MsgTypeDesc::matrixType);
        it != msgTypes.end())
        return *it;

    return { {}, MsgType::Unknown, false, nullptr };
}

inline bool isReplacement(const std::optional<EventRelation>& rel)
{
    return rel && rel->type == EventRelation::ReplacementType;
}

} // anonymous namespace

QJsonObject RoomMessageEvent::assembleContentJson(const QString& plainBody,
                                                  const QString& jsonMsgType,
                                                  std::unique_ptr<Base> content,
                                                  const std::optional<EventRelation>& relatesTo)
{
    QJsonObject json;
    if (content) {
        // TODO: replace with content->fillJson(json) when it starts working
        json = content->toJson();
    }
    json.insert(MsgTypeKey, jsonMsgType);
    json.insert(BodyKey, plainBody);
    if (relatesTo.has_value()) {
        json.insert(RelatesToKey, toJson(relatesTo.value()));
        if (relatesTo->type == EventRelation::ReplacementType) {
            QJsonObject newContentJson;
            if (auto* textContent = static_cast<const TextContent*>(content.get());
                    textContent && textContent->mimeType.inherits("text/html"_L1)) {
                newContentJson.insert(FormatKey, HtmlContentTypeId);
                newContentJson.insert(FormattedBodyKey, textContent->body);
            }
            newContentJson.insert(BodyKey, plainBody);
            newContentJson.insert(MsgTypeKey, jsonMsgType);
            json.insert("m.new_content"_L1, newContentJson);
            json.insert(BodyKey, "* "_L1 + plainBody);
        }
    }
    return json;
}

RoomMessageEvent::RoomMessageEvent(const QString& plainBody, const QString& jsonMsgType,
                                   std::unique_ptr<Base> content,
                                   const std::optional<EventRelation>& relatesTo)
    : RoomEvent(basicJson(TypeId, assembleContentJson(plainBody, jsonMsgType, std::move(content),
                                                      relatesTo)))
{}

RoomMessageEvent::RoomMessageEvent(const QString& plainBody, MsgType msgType,
                                   std::unique_ptr<Base> content,
                                   const std::optional<EventRelation>& relatesTo)
    : RoomMessageEvent(plainBody, msgTypeToJson(msgType), std::move(content), relatesTo)
{}

RoomMessageEvent::RoomMessageEvent(const QJsonObject& obj)
    : RoomEvent(obj)
{
    if (isRedacted())
        return;
    const QJsonObject content = contentJson();
    if (!content.contains(MsgTypeKey) || !content.contains(BodyKey)) {
        qCWarning(EVENTS) << "No body or msgtype in room message event";
        qCWarning(EVENTS) << formatJson << fullJson();
        return;
    }

    if (auto it = std::ranges::find(msgTypes, content[MsgTypeKey].toString(), &MsgTypeDesc::matrixType); it == msgTypes.cend()) {
        qCWarning(EVENTS) << "RoomMessageEvent: unknown msgtype, full content dump follows";
        qCWarning(EVENTS) << formatJson << content;
    }
}

RoomMessageEvent::MsgType RoomMessageEvent::msgtype() const
{
    return jsonToMsgTypeDesc(rawMsgtype()).enumType;
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
    return content() ? content()->type() : PlainTextMimeType;
}

std::unique_ptr<Base> RoomMessageEvent::content() const
{
    const auto content = contentJson();
    if (!content.contains(MsgTypeKey) || !content.contains(BodyKey)) {
        qCWarning(EVENTS) << "No body or msgtype in room message event";
        qCWarning(EVENTS) << formatJson << fullJson();
        return {};
    }

    if (const auto it =
            std::ranges::find(msgTypes, content[MsgTypeKey].toString(), &MsgTypeDesc::matrixType);
        it != msgTypes.cend())
        return it->maker(content);

    qCWarning(EVENTS) << "RoomMessageEvent: unknown msgtype, full content dump follows";
    qCWarning(EVENTS) << formatJson << content;
    return {};
}

void RoomMessageEvent::setContent(std::unique_ptr<Base> content)
{
    editJson()[ContentKey] =
        assembleContentJson(plainBody(), rawMsgtype(), std::move(content), relatesTo());
}

template <>
bool RoomMessageEvent::has<TextContent>() const
{
    const auto t = msgtype();
    return (t == MsgType::Text || t == MsgType::Emote || t == MsgType::Notice)
           && make<TextContent>(contentJson()) != nullptr;
}

template <>
bool RoomMessageEvent::has<FileContentBase>() const
{
    return jsonToMsgTypeDesc(rawMsgtype()).fileBased;
}

template <>
bool RoomMessageEvent::has<FileContent>() const
{
    return rawMsgtype() == FileTypeId;
}

template <>
bool RoomMessageEvent::has<ImageContent>() const
{
    return rawMsgtype() == ImageTypeId;
}

template <>
bool RoomMessageEvent::has<AudioContent>() const
{
    return rawMsgtype() == AudioTypeId;
}

template <>
bool RoomMessageEvent::has<VideoContent>() const
{
    return rawMsgtype() == VideoTypeId;
}

bool RoomMessageEvent::hasThumbnail() const
{
    return fromJson<QUrl>(contentJson()[InfoKey]["thumbnail_url"_L1]).isValid();
}

Thumbnail RoomMessageEvent::getThumbnail() const
{
    return contentPart<Thumbnail>(InfoKey);
}

template <>
bool RoomMessageEvent::has<LocationContent>() const
{
    return rawMsgtype() == LocationTypeId;
}

std::optional<EventRelation> RoomMessageEvent::relatesTo() const
{
    return contentPart<std::optional<EventRelation>>(RelatesToKey);
}

QString RoomMessageEvent::upstreamEventId() const
{
    const auto relation = relatesTo();
    return relation ? relation.value().eventId : QString();
}

QString RoomMessageEvent::replacedEvent() const
{
    if (!has<TextContent>())
        return {};

    const auto er = relatesTo();
    return isReplacement(er) ? er->eventId : QString();
}

bool RoomMessageEvent::isReplaced() const
{
    return unsignedPart<QJsonObject>("m.relations"_L1).contains(EventRelation::ReplacementType);
}

QString RoomMessageEvent::replacedBy() const
{
    return unsignedPart<QJsonObject>("m.relations"_L1)[EventRelation::ReplacementType][EventIdKey]
        .toString();
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
    if (const auto relation = relatesTo()) {
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
    const auto fileInfo = get<FileContent>();
    if (QUO_ALARM(fileInfo == nullptr))
        return {};

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
    return name.startsWith("image/"_L1)   ? ImageTypeId
           : name.startsWith("video/"_L1) ? VideoTypeId
           : name.startsWith("audio/"_L1) ? AudioTypeId
                                          : FileTypeId;
}

QString RoomMessageEvent::rawMsgTypeForUrl(const QUrl& url)
{
    return rawMsgTypeForMimeType(QMimeDatabase().mimeTypeForUrl(url));
}

QString RoomMessageEvent::rawMsgTypeForFile(const QFileInfo& fi)
{
    return rawMsgTypeForMimeType(QMimeDatabase().mimeTypeForFile(fi));
}

TextContent::TextContent(QString text, const QString& contentType)
    : mimeType(QMimeDatabase().mimeTypeForName(contentType)), body(std::move(text))
{
    if (contentType == HtmlContentTypeId)
        mimeType = QMimeDatabase().mimeTypeForName("text/html"_L1);
}

TextContent::TextContent(const QJsonObject& json)
{
    QMimeDatabase db;
    static const auto PlainTextMimeType = db.mimeTypeForName("text/plain"_L1);
    static const auto HtmlMimeType = db.mimeTypeForName("text/html"_L1);

    const auto relatesTo = fromJson<std::optional<EventRelation>>(json[RelatesToKey]);

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
    if (mimeType.inherits("text/html"_L1)) {
        json.insert(FormatKey, HtmlContentTypeId);
        json.insert(FormattedBodyKey, body);
    }
}

LocationContent::LocationContent(const QString& geoUri, const Thumbnail& thumbnail)
    : geoUri(geoUri), thumbnail(thumbnail)
{}

LocationContent::LocationContent(const QJsonObject& json)
    : Base(json)
    , geoUri(json["geo_uri"_L1].toString())
    , thumbnail(json[InfoKey].toObject())
{}

QMimeType LocationContent::type() const
{
    return QMimeDatabase().mimeTypeForData(geoUri.toLatin1());
}

void LocationContent::fillJson(QJsonObject& o) const
{
    o.insert("geo_uri"_L1, geoUri);
    o.insert(InfoKey, toInfoJson(thumbnail));
}
