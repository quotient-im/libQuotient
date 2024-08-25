// SPDX-FileCopyrightText: 2017 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "eventcontent.h"

#include "../logging_categories_p.h"

#include "../converters.h"

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
        infoJson.insert(QStringLiteral("size"), info.payloadSize);
    if (info.mimeType.isValid())
        infoJson.insert(QStringLiteral("mimetype"), info.mimeType.name());
    return infoJson;
}

ImageInfo::ImageInfo(const QFileInfo& fi, QSize imageSize)
    : FileInfo(fi), imageSize(imageSize)
{}

ImageInfo::ImageInfo(FileSourceInfo sourceInfo, qint64 fileSize,
                     const QMimeType& type, QSize imageSize,
                     const QString& originalFilename)
    : FileInfo(std::move(sourceInfo), fileSize, type, originalFilename)
    , imageSize(imageSize)
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
        infoJson.insert(QStringLiteral("w"), info.imageSize.width());
    if (info.imageSize.height() != -1)
        infoJson.insert(QStringLiteral("h"), info.imageSize.height());
    return infoJson;
}

Thumbnail::Thumbnail(const QJsonObject& infoJson,
                     const std::optional<EncryptedFileMetadata> &efm)
    : ImageInfo(QUrl(infoJson["thumbnail_url"_L1].toString()),
                infoJson["thumbnail_info"_L1].toObject())
{
    if (efm)
        source = *efm;
}

void Thumbnail::dumpTo(QJsonObject& infoJson) const
{
    if (url().isValid())
        fillJson(infoJson, { "thumbnail_url"_L1, "thumbnail_file"_L1 }, source);
    if (!imageSize.isEmpty())
        infoJson.insert(QStringLiteral("thumbnail_info"),
                         toInfoJson(*this));
}
