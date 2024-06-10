// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "authed-content-repo.h"

using namespace Quotient;

auto queryToGetContentAuthed(qint64 timeoutMs)
{
    QUrlQuery _q;
    addParam<IfNotEmpty>(_q, QStringLiteral("timeout_ms"), timeoutMs);
    return _q;
}

QUrl GetContentAuthedJob::makeRequestUrl(QUrl baseUrl, const QString& serverName,
                                         const QString& mediaId, qint64 timeoutMs)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   makePath("/_matrix/client/v1", "/media/download/", serverName,
                                            "/", mediaId),
                                   queryToGetContentAuthed(timeoutMs));
}

GetContentAuthedJob::GetContentAuthedJob(const QString& serverName, const QString& mediaId,
                                         qint64 timeoutMs)
    : BaseJob(HttpVerb::Get, QStringLiteral("GetContentAuthedJob"),
              makePath("/_matrix/client/v1", "/media/download/", serverName, "/", mediaId),
              queryToGetContentAuthed(timeoutMs))
{
    setExpectedContentTypes({ "application/octet-stream" });
}

auto queryToGetContentOverrideNameAuthed(qint64 timeoutMs)
{
    QUrlQuery _q;
    addParam<IfNotEmpty>(_q, QStringLiteral("timeout_ms"), timeoutMs);
    return _q;
}

QUrl GetContentOverrideNameAuthedJob::makeRequestUrl(QUrl baseUrl, const QString& serverName,
                                                     const QString& mediaId,
                                                     const QString& fileName, qint64 timeoutMs)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   makePath("/_matrix/client/v1", "/media/download/", serverName,
                                            "/", mediaId, "/", fileName),
                                   queryToGetContentOverrideNameAuthed(timeoutMs));
}

GetContentOverrideNameAuthedJob::GetContentOverrideNameAuthedJob(const QString& serverName,
                                                                 const QString& mediaId,
                                                                 const QString& fileName,
                                                                 qint64 timeoutMs)
    : BaseJob(HttpVerb::Get, QStringLiteral("GetContentOverrideNameAuthedJob"),
              makePath("/_matrix/client/v1", "/media/download/", serverName, "/", mediaId, "/",
                       fileName),
              queryToGetContentOverrideNameAuthed(timeoutMs))
{
    setExpectedContentTypes({ "application/octet-stream" });
}

auto queryToGetContentThumbnailAuthed(int width, int height, const QString& method,
                                      qint64 timeoutMs, std::optional<bool> animated)
{
    QUrlQuery _q;
    addParam<>(_q, QStringLiteral("width"), width);
    addParam<>(_q, QStringLiteral("height"), height);
    addParam<IfNotEmpty>(_q, QStringLiteral("method"), method);
    addParam<IfNotEmpty>(_q, QStringLiteral("timeout_ms"), timeoutMs);
    addParam<IfNotEmpty>(_q, QStringLiteral("animated"), animated);
    return _q;
}

QUrl GetContentThumbnailAuthedJob::makeRequestUrl(QUrl baseUrl, const QString& serverName,
                                                  const QString& mediaId, int width, int height,
                                                  const QString& method, qint64 timeoutMs,
                                                  std::optional<bool> animated)
{
    return BaseJob::makeRequestUrl(
        std::move(baseUrl),
        makePath("/_matrix/client/v1", "/media/thumbnail/", serverName, "/", mediaId),
        queryToGetContentThumbnailAuthed(width, height, method, timeoutMs, animated));
}

GetContentThumbnailAuthedJob::GetContentThumbnailAuthedJob(const QString& serverName,
                                                           const QString& mediaId, int width,
                                                           int height, const QString& method,
                                                           qint64 timeoutMs,
                                                           std::optional<bool> animated)
    : BaseJob(HttpVerb::Get, QStringLiteral("GetContentThumbnailAuthedJob"),
              makePath("/_matrix/client/v1", "/media/thumbnail/", serverName, "/", mediaId),
              queryToGetContentThumbnailAuthed(width, height, method, timeoutMs, animated))
{
    setExpectedContentTypes({ "image/jpeg", "image/png", "image/apng", "image/gif", "image/webp" });
}

auto queryToGetUrlPreviewAuthed(const QUrl& url, std::optional<qint64> ts)
{
    QUrlQuery _q;
    addParam<>(_q, QStringLiteral("url"), url);
    addParam<IfNotEmpty>(_q, QStringLiteral("ts"), ts);
    return _q;
}

QUrl GetUrlPreviewAuthedJob::makeRequestUrl(QUrl baseUrl, const QUrl& url, std::optional<qint64> ts)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   makePath("/_matrix/client/v1", "/media/preview_url"),
                                   queryToGetUrlPreviewAuthed(url, ts));
}

GetUrlPreviewAuthedJob::GetUrlPreviewAuthedJob(const QUrl& url, std::optional<qint64> ts)
    : BaseJob(HttpVerb::Get, QStringLiteral("GetUrlPreviewAuthedJob"),
              makePath("/_matrix/client/v1", "/media/preview_url"),
              queryToGetUrlPreviewAuthed(url, ts))
{}

QUrl GetConfigAuthedJob::makeRequestUrl(QUrl baseUrl)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   makePath("/_matrix/client/v1", "/media/config"));
}

GetConfigAuthedJob::GetConfigAuthedJob()
    : BaseJob(HttpVerb::Get, QStringLiteral("GetConfigAuthedJob"),
              makePath("/_matrix/client/v1", "/media/config"))
{}
