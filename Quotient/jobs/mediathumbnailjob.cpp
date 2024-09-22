// SPDX-FileCopyrightText: 2018 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "mediathumbnailjob.h"

#include "../csapi/authed-content-repo.h"
#include "../csapi/content-repo.h"

#include "../connectiondata.h"
#include "../logging_categories_p.h"

using namespace Quotient;

QUrl MediaThumbnailJob::makeRequestUrl(const HomeserverData& hsData, const QUrl& mxcUri,
                                       QSize requestedSize, std::optional<bool> animated)
{
    return makeRequestUrl(hsData, mxcUri.authority(), mxcUri.path().mid(1), requestedSize, animated);
}

QUrl MediaThumbnailJob::makeRequestUrl(const HomeserverData& hsData, const QString& serverName,
                                       const QString& mediaId, QSize requestedSize,
                                       std::optional<bool> animated)
{
    QT_IGNORE_DEPRECATIONS( // For GetContentThumbnailJob
        return hsData.checkMatrixSpecVersion(u"1.11")
                   ? GetContentThumbnailAuthedJob::makeRequestUrl(hsData, serverName, mediaId,
                                                                  requestedSize.width(),
                                                                  requestedSize.height(),
                                                                  "scale"_L1, 20'000, animated)
                   : GetContentThumbnailJob::makeRequestUrl(hsData, serverName, mediaId,
                                                            requestedSize.width(),
                                                            requestedSize.height(), "scale"_L1,
                                                            true, 20'000, false, animated);)
}

MediaThumbnailJob::MediaThumbnailJob(QString serverName, QString mediaId, QSize requestedSize,
                                     std::optional<bool> animated)
    : BaseJob(HttpVerb::Get, u"MediaThumbnailJob"_s, {})
    , serverName(std::move(serverName))
    , mediaId(std::move(mediaId))
    , requestedSize(std::move(requestedSize))
    , animated(animated)
{
    setLoggingCategory(THUMBNAILJOB);
    setExpectedContentTypes({ "image/jpeg", "image/png", "image/apng", "image/gif", "image/webp" });
}

MediaThumbnailJob::MediaThumbnailJob(const QUrl& mxcUri, QSize requestedSize,
                                     std::optional<bool> animated)
    : MediaThumbnailJob(mxcUri.authority(), mxcUri.path().mid(1) /* sans leading '/' */,
                        requestedSize, animated)
{}

QImage MediaThumbnailJob::thumbnail() const { return _thumbnail; }

QImage MediaThumbnailJob::scaledThumbnail(QSize toSize) const
{
    return _thumbnail.scaled(toSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

void MediaThumbnailJob::doPrepare(const ConnectionData* connectionData)
{
    const auto url = makeRequestUrl(connectionData->homeserverData(), serverName, mediaId,
                                    requestedSize, animated);
    setApiEndpoint(url.toEncoded(QUrl::RemoveQuery | QUrl::RemoveFragment | QUrl::FullyEncoded));
    setRequestQuery(QUrlQuery{ url.query() });
}

BaseJob::Status MediaThumbnailJob::prepareResult()
{
    if (_thumbnail.loadFromData(reply()->readAll()))
        return Success;

    return { IncorrectResponse, u"Could not read image data"_s };
}
