/******************************************************************************
 * Copyright (C) 2017 Kitsune Ral <kitsune-ral@users.sf.net>
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

#include "eventcontent.h"

#include "converters.h"
#include "util.h"

#include <QtCore/QMimeDatabase>

using namespace QMatrixClient::EventContent;

QJsonObject Base::toJson() const
{
    QJsonObject o;
    fillJson(&o);
    return o;
}

FileInfo::FileInfo(const QUrl& u, qint64 payloadSize, const QMimeType& mimeType,
                   const QString& originalFilename)
    : mimeType(mimeType), url(u), payloadSize(payloadSize)
    , originalName(originalFilename)
{ }

FileInfo::FileInfo(const QUrl& u, const QJsonObject& infoJson,
                   const QString& originalFilename)
    : originalInfoJson(infoJson)
    , mimeType(QMimeDatabase().mimeTypeForName(infoJson["mimetype"_ls].toString()))
    , url(u)
    , payloadSize(fromJson<qint64>(infoJson["size"_ls]))
    , originalName(originalFilename)
{
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
}

ImageInfo::ImageInfo(const QUrl& u, qint64 fileSize, QMimeType mimeType,
                     const QSize& imageSize, const QString& originalFilename)
    : FileInfo(u, fileSize, mimeType, originalFilename), imageSize(imageSize)
{ }

ImageInfo::ImageInfo(const QUrl& u, const QJsonObject& infoJson,
                     const QString& originalFilename)
    : FileInfo(u, infoJson, originalFilename)
    , imageSize(infoJson["w"_ls].toInt(), infoJson["h"_ls].toInt())
{ }

void ImageInfo::fillInfoJson(QJsonObject* infoJson) const
{
    FileInfo::fillInfoJson(infoJson);
    if (imageSize.width() != -1)
        infoJson->insert(QStringLiteral("w"), imageSize.width());
    if (imageSize.height() != -1)
        infoJson->insert(QStringLiteral("h"), imageSize.height());
}

Thumbnail::Thumbnail(const QJsonObject& infoJson)
    : ImageInfo(infoJson["thumbnail_url"_ls].toString(),
                infoJson["thumbnail_info"_ls].toObject())
{ }

void Thumbnail::fillInfoJson(QJsonObject* infoJson) const
{
    if (url.isValid())
        infoJson->insert(QStringLiteral("thumbnail_url"), url.toString());
    if (!imageSize.isEmpty())
        infoJson->insert(QStringLiteral("thumbnail_info"),
                         toInfoJson<ImageInfo>(*this));
}
