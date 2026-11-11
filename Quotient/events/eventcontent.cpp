// SPDX-FileCopyrightText: 2017 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "eventcontent.h"

#include "../logging_categories_p.h"

#include "../converters.h"
#include "eventrelation.h"

#include <QtCore/QFileInfo>
#include <QtCore/QMimeDatabase>

using namespace Quotient::EventContent;

QJsonObject Base::toJson() const
{
    QJsonObject o;
    fillJson(o);
    return o;
}

FileInfo::FileInfo(const QFileInfo& fi)
    : source(QUrl::fromLocalFile(fi.filePath())),
      mimeType(QMimeDatabase().mimeTypeForFile(fi)),
      payloadSize(fi.size()),
      originalName(fi.fileName())
{
    Q_ASSERT(fi.isFile());
}

FileInfo::FileInfo(FileSourceInfo sourceInfo, qint64 payloadSize,
                   const QMimeType& mimeType, QString originalFilename)
    : source(std::move(sourceInfo))
    , mimeType(mimeType)
    , payloadSize(payloadSize)
    , originalName(std::move(originalFilename))
{
    if (!isValid())
        qCWarning(MESSAGES)
            << "To client developers: using FileInfo(QUrl, qint64, ...) "
               "constructor for non-mxc resources is deprecated since Quotient "
               "0.7; for local resources, use FileInfo(QFileInfo) instead";
}

FileInfo::FileInfo(FileSourceInfo sourceInfo, const QJsonObject& infoJson,
                   QString originalFilename)
    : source(std::move(sourceInfo))
    , originalInfoJson(infoJson)
    , mimeType(
          QMimeDatabase().mimeTypeForName(infoJson["mimetype"_L1].toString()))
    , payloadSize(fromJson<qint64>(infoJson["size"_L1]))
    , originalName(std::move(originalFilename))
{
    if (!mimeType.isValid())
        mimeType = QMimeDatabase().mimeTypeForData(QByteArray());
}

bool FileInfo::isValid() const
{
    const auto& u = url();
    return u.scheme() == "mxc"_L1 && QString(u.authority() + u.path()).count(u'/') == 1;
}

QUrl FileInfo::url() const
{
    return getUrlFromSourceInfo(source);
}

QJsonObject Quotient::EventContent::toInfoJson(const FileInfo& info)
{
    QJsonObject infoJson;
    if (info.payloadSize != -1)
        infoJson.insert("size"_L1, info.payloadSize);
    if (info.mimeType.isValid())
        infoJson.insert("mimetype"_L1, info.mimeType.name());
    return infoJson;
}

ImageInfo::ImageInfo(const QFileInfo& fi, QSize imageSize)
    : FileInfo(fi), imageSize(imageSize)
{}

ImageInfo::ImageInfo(FileSourceInfo sourceInfo, qint64 fileSize,
                     const QMimeType& type, QSize imageSize,
                     const QString& originalFilename, const QString &imageBlurhash)
    : FileInfo(std::move(sourceInfo), fileSize, type, originalFilename)
    , imageSize(imageSize)
    , blurhash(imageBlurhash)
{}

ImageInfo::ImageInfo(FileSourceInfo sourceInfo, const QJsonObject& infoJson,
                     const QString& originalFilename)
    : FileInfo(std::move(sourceInfo), infoJson, originalFilename)
    , imageSize(infoJson["w"_L1].toInt(), infoJson["h"_L1].toInt())
{}

QJsonObject Quotient::EventContent::toInfoJson(const ImageInfo& info)
{
    auto infoJson = toInfoJson(static_cast<const FileInfo&>(info));
    if (info.imageSize.width() != -1)
        infoJson.insert("w"_L1, info.imageSize.width());
    if (info.imageSize.height() != -1)
        infoJson.insert("h"_L1, info.imageSize.height());
    if (!info.blurhash.isEmpty())
        infoJson.insert("xyz.amorgan.blurhash"_L1, info.blurhash);
    return infoJson;
}

Thumbnail::Thumbnail(const QJsonObject& infoJson)
    : ImageInfo(fileSourceInfoFromJson(infoJson, { "thumbnail_url"_L1, "thumbnail_file"_L1 }),
                infoJson["thumbnail_info"_L1].toObject())
{}

void Thumbnail::dumpTo(QJsonObject& infoJson) const
{
    if (url().isValid())
        fillJson(infoJson, { "thumbnail_url"_L1, "thumbnail_file"_L1 }, source);
    if (!imageSize.isEmpty())
        infoJson.insert("thumbnail_info"_L1, toInfoJson(*this));
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

    const auto actualJson = relatesTo && relatesTo->type == EventRelation::ReplacementType
                                ? json.value(NewContentKey).toObject()
                                : json;
    // Special-casing the custom matrix.org's (actually, Element's) way
    // of sending HTML messages.
    if (actualJson[FormatKey].toString() == HtmlContentTypeId) {
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
