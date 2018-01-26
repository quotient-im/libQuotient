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

#include <QtCore/QUrl>
#include <QtCore/QMimeDatabase>

using namespace QMatrixClient::EventContent;

QJsonObject Base::toJson() const
{
    QJsonObject o;
    fillJson(&o);
    return o;
}

FileInfo::FileInfo(const QUrl& u, int payloadSize, const QMimeType& mimeType,
                   const QString& originalFilename)
    : mimeType(mimeType), url(u), payloadSize(payloadSize)
    , originalName(originalFilename)
{ }

FileInfo::FileInfo(const QUrl& u, const QJsonObject& infoJson,
                   const QString& originalFilename)
    : originalInfoJson(infoJson)
    , mimeType(QMimeDatabase().mimeTypeForName(infoJson["mimetype"].toString()))
    , url(u)
    , payloadSize(infoJson["size"].toInt())
    , originalName(originalFilename)
{
    if (!mimeType.isValid())
        mimeType = QMimeDatabase().mimeTypeForData(QByteArray());
}

void FileInfo::fillInfoJson(QJsonObject* infoJson) const
{
    Q_ASSERT(infoJson);
    infoJson->insert("size", payloadSize);
    infoJson->insert("mimetype", mimeType.name());
}

ImageInfo::ImageInfo(const QUrl& u, int fileSize, QMimeType mimeType,
                     const QSize& imageSize)
    : FileInfo(u, fileSize, mimeType), imageSize(imageSize)
{ }

ImageInfo::ImageInfo(const QUrl& u, const QJsonObject& infoJson,
                     const QString& originalFilename)
    : FileInfo(u, infoJson, originalFilename)
    , imageSize(infoJson["w"].toInt(), infoJson["h"].toInt())
{ }

void ImageInfo::fillInfoJson(QJsonObject* infoJson) const
{
    FileInfo::fillInfoJson(infoJson);
    infoJson->insert("w", imageSize.width());
    infoJson->insert("h", imageSize.height());
}

Thumbnail::Thumbnail(const QJsonObject& infoJson)
    : ImageInfo(infoJson["thumbnail_url"].toString(),
                infoJson["thumbnail_info"].toObject())
{ }

void Thumbnail::fillInfoJson(QJsonObject* infoJson) const
{
    infoJson->insert("thumbnail_url", url.toString());
    infoJson->insert("thumbnail_info", toInfoJson<ImageInfo>(*this));
}
