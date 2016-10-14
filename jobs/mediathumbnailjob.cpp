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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "mediathumbnailjob.h"

#include <QtCore/QDebug>

using namespace QMatrixClient;

class MediaThumbnailJob::Private
{
    public:
        QUrl url;
        QPixmap thumbnail;
        QSize requestedSize;
        ThumbnailType thumbnailType;
};

MediaThumbnailJob::MediaThumbnailJob(ConnectionData* data, QUrl url, QSize requestedSize,
                                     ThumbnailType thumbnailType)
    : BaseJob(data, JobHttpType::GetJob, "MediaThumbnailJob")
    , d(new Private)
{
    d->url = url;
    d->requestedSize = requestedSize;
    d->thumbnailType = thumbnailType;
}

MediaThumbnailJob::~MediaThumbnailJob()
{
    delete d;
}

QPixmap MediaThumbnailJob::thumbnail()
{
    return d->thumbnail;
}

QString MediaThumbnailJob::apiPath() const
{
    return QString("/_matrix/media/v1/thumbnail/%1%2").arg(d->url.host()).arg(d->url.path());
}

QUrlQuery MediaThumbnailJob::query() const
{
    QUrlQuery query;
    query.addQueryItem("width", QString::number(d->requestedSize.width()));
    query.addQueryItem("height", QString::number(d->requestedSize.height()));
    if( d->thumbnailType == ThumbnailType::Scale )
        query.addQueryItem("method", "scale");
    else
        query.addQueryItem("method", "crop");
    return query;
}

BaseJob::Status MediaThumbnailJob::parseReply(QByteArray data)
{
    if( !d->thumbnail.loadFromData(data) )
    {
        qDebug() << "MediaThumbnailJob: could not read image data";
    }
    return Success;
}
