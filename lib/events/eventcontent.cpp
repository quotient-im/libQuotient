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
    fillJson(&o);
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
                   Omittable<EncryptedFile> file, QString originalFilename)
    : mimeType(mimeType)
    , url(move(u))
    , payloadSize(payloadSize)
    , originalName(move(originalFilename))
    , file(file)
{
    if (!isValid())
        qCWarning(MESSAGES)
            << "To client developers: using FileInfo(QUrl, qint64, ...) "
               "constructor for non-mxc resources is deprecated since Quotient "
               "0.7; for local resources, use FileInfo(QFileInfo) instead";
}

FileInfo::FileInfo(QUrl mxcUrl, const QJsonObject& infoJson,
                   const Omittable<EncryptedFile> &file,
                   QString originalFilename)
    : originalInfoJson(infoJson)
    , mimeType(
          QMimeDatabase().mimeTypeForName(infoJson["mimetype"_ls].toString()))
    , url(move(mxcUrl))
    , payloadSize(fromJson<qint64>(infoJson["size"_ls]))
    , originalName(move(originalFilename))
    , file(file)
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

void FileInfo::fillInfoJson(QJsonObject* infoJson) const
{
    Q_ASSERT(infoJson);
    if (payloadSize != -1)
        infoJson->insert(QStringLiteral("size"), payloadSize);
    if (mimeType.isValid())
        infoJson->insert(QStringLiteral("mimetype"), mimeType.name());
    //TODO add encryptedfile
}

ImageInfo::ImageInfo(const QFileInfo& fi, QSize imageSize)
    : FileInfo(fi), imageSize(imageSize)
{}

ImageInfo::ImageInfo(const QUrl& mxcUrl, qint64 fileSize, const QMimeType& type,
                     QSize imageSize, const Omittable<EncryptedFile> &file, const QString& originalFilename)
    : FileInfo(mxcUrl, fileSize, type, file, originalFilename)
    , imageSize(imageSize)
{}

ImageInfo::ImageInfo(const QUrl& mxcUrl, const QJsonObject& infoJson,
                     const Omittable<EncryptedFile> &file,
                     const QString& originalFilename)
    : FileInfo(mxcUrl, infoJson, file, originalFilename)
    , imageSize(infoJson["w"_ls].toInt(), infoJson["h"_ls].toInt())
{}

void ImageInfo::fillInfoJson(QJsonObject* infoJson) const
{
    FileInfo::fillInfoJson(infoJson);
    if (imageSize.width() != -1)
        infoJson->insert(QStringLiteral("w"), imageSize.width());
    if (imageSize.height() != -1)
        infoJson->insert(QStringLiteral("h"), imageSize.height());
}

Thumbnail::Thumbnail(const QJsonObject& infoJson, const Omittable<EncryptedFile> &file)
    : ImageInfo(QUrl(infoJson["thumbnail_url"_ls].toString()),
                infoJson["thumbnail_info"_ls].toObject(),
                file)
{}

void Thumbnail::fillInfoJson(QJsonObject* infoJson) const
{
    if (url.isValid())
        infoJson->insert(QStringLiteral("thumbnail_url"), url.toString());
    if (!imageSize.isEmpty())
        infoJson->insert(QStringLiteral("thumbnail_info"),
                         toInfoJson<ImageInfo>(*this));
}
