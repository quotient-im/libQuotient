// SPDX-FileCopyrightText: 2017 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "eventcontent.h"

#include "converters.h"
#include "logging.h"

#include <QtCore/QMimeDatabase>
#include <QtCore/QFileInfo>

using namespace Quotient::EventContent;
using std::move;

QJsonObject Base::toJson() const
{
    QJsonObject o;
    fillJson(o);
    return o;
}

FileInfo::FileInfo(const QFileInfo &fi)
    : mimeType(QMimeDatabase().mimeTypeForFile(fi))
    , url(QUrl::fromLocalFile(fi.filePath()))
    , payloadSize(fi.size())
    , originalName(fi.fileName())
{
    Q_ASSERT(fi.isFile());
}

FileInfo::FileInfo(QUrl u, qint64 payloadSize, const QMimeType& mimeType,
                   Omittable<EncryptedFile> encryptedFile,
                   QString originalFilename)
    : mimeType(mimeType)
    , url(move(u))
    , payloadSize(payloadSize)
    , originalName(move(originalFilename))
    , file(move(encryptedFile))
{
    if (!isValid())
        qCWarning(MESSAGES)
            << "To client developers: using FileInfo(QUrl, qint64, ...) "
               "constructor for non-mxc resources is deprecated since Quotient "
               "0.7; for local resources, use FileInfo(QFileInfo) instead";
}

FileInfo::FileInfo(QUrl mxcUrl, const QJsonObject& infoJson,
                   Omittable<EncryptedFile> encryptedFile,
                   QString originalFilename)
    : originalInfoJson(infoJson)
    , mimeType(
          QMimeDatabase().mimeTypeForName(infoJson["mimetype"_ls].toString()))
    , url(move(mxcUrl))
    , payloadSize(fromJson<qint64>(infoJson["size"_ls]))
    , originalName(move(originalFilename))
    , file(move(encryptedFile))
{
    if(url.isEmpty() && file.has_value()) {
        url = file->url;
    }
    if (!mimeType.isValid())
        mimeType = QMimeDatabase().mimeTypeForData(QByteArray());
}

bool FileInfo::isValid() const
{
    return url.scheme() == "mxc"
           && (url.authority() + url.path()).count('/') == 1;
}

QJsonObject Quotient::EventContent::toInfoJson(const FileInfo& info)
{
    QJsonObject infoJson;
    if (info.payloadSize != -1)
        infoJson.insert(QStringLiteral("size"), info.payloadSize);
    if (info.mimeType.isValid())
        infoJson.insert(QStringLiteral("mimetype"), info.mimeType.name());
    //TODO add encryptedfile
    return infoJson;
}

ImageInfo::ImageInfo(const QFileInfo& fi, QSize imageSize)
    : FileInfo(fi), imageSize(imageSize)
{}

ImageInfo::ImageInfo(const QUrl& mxcUrl, qint64 fileSize, const QMimeType& type,
                     QSize imageSize, Omittable<EncryptedFile> encryptedFile,
                     const QString& originalFilename)
    : FileInfo(mxcUrl, fileSize, type, move(encryptedFile), originalFilename)
    , imageSize(imageSize)
{}

ImageInfo::ImageInfo(const QUrl& mxcUrl, const QJsonObject& infoJson,
                     Omittable<EncryptedFile> encryptedFile,
                     const QString& originalFilename)
    : FileInfo(mxcUrl, infoJson, move(encryptedFile), originalFilename)
    , imageSize(infoJson["w"_ls].toInt(), infoJson["h"_ls].toInt())
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
                     Omittable<EncryptedFile> encryptedFile)
    : ImageInfo(QUrl(infoJson["thumbnail_url"_ls].toString()),
                infoJson["thumbnail_info"_ls].toObject(), move(encryptedFile))
{}

void Thumbnail::dumpTo(QJsonObject& infoJson) const
{
    if (url.isValid())
        infoJson.insert(QStringLiteral("thumbnail_url"), url.toString());
    if (!imageSize.isEmpty())
        infoJson.insert(QStringLiteral("thumbnail_info"),
                         toInfoJson(*this));
}
