/******************************************************************************
 * Copyright (C) 2016 Felix Rohrbach <kde@fxrh.de>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include "mediathumbnailjob.h"

using namespace Quotient;

QUrl MediaThumbnailJob::makeRequestUrl(QUrl baseUrl, const QUrl& mxcUri,
                                       QSize requestedSize)
{
    return makeRequestUrl(std::move(baseUrl), mxcUri.authority(),
                          mxcUri.path().mid(1), requestedSize.width(),
                          requestedSize.height());
}

MediaThumbnailJob::MediaThumbnailJob(const QString& serverName,
                                     const QString& mediaId, QSize requestedSize)
    : GetContentThumbnailJob(serverName, mediaId, requestedSize.width(),
                             requestedSize.height())
{}

MediaThumbnailJob::MediaThumbnailJob(const QUrl& mxcUri, QSize requestedSize)
    : MediaThumbnailJob(mxcUri.authority(),
                        mxcUri.path().mid(1), // sans leading '/'
                        requestedSize)
{}

QImage MediaThumbnailJob::thumbnail() const { return _thumbnail; }

QImage MediaThumbnailJob::scaledThumbnail(QSize toSize) const
{
    return _thumbnail.scaled(toSize, Qt::KeepAspectRatio,
                             Qt::SmoothTransformation);
}

BaseJob::Status MediaThumbnailJob::parseReply(QNetworkReply* reply)
{
    auto result = GetContentThumbnailJob::parseReply(reply);
    if (!result.good())
        return result;

    if (_thumbnail.loadFromData(data()->readAll()))
        return Success;

    return { IncorrectResponseError,
             QStringLiteral("Could not read image data") };
}
