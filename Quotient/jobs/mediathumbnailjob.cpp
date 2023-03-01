// SPDX-FileCopyrightText: 2018 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

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
                             requestedSize.height(), "scale"_ls)
{
    setLoggingCategory(THUMBNAILJOB);
}

MediaThumbnailJob::MediaThumbnailJob(const QUrl& mxcUri, QSize requestedSize)
    : MediaThumbnailJob(mxcUri.authority(),
                        mxcUri.path().mid(1), // sans leading '/'
                        requestedSize)
{
    setLoggingCategory(THUMBNAILJOB);
}

QImage MediaThumbnailJob::thumbnail() const { return _thumbnail; }

QImage MediaThumbnailJob::scaledThumbnail(QSize toSize) const
{
    return _thumbnail.scaled(toSize, Qt::KeepAspectRatio,
                             Qt::SmoothTransformation);
}

BaseJob::Status MediaThumbnailJob::prepareResult()
{
    if (_thumbnail.loadFromData(data()->readAll()))
        return Success;

    return { IncorrectResponse, QStringLiteral("Could not read image data") };
}
