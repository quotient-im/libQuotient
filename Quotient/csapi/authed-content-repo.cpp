// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "authed-content-repo.h"

using namespace Quotient;

auto queryToGetContentAuthed(qint64 timeoutMs)
{
    QUrlQuery _q;
    addParam<IfNotEmpty>(_q, u"timeout_ms"_s, timeoutMs);
    return _q;
}

QUrl GetContentAuthedJob::makeRequestUrl(const HomeserverData& hsData, const QString& serverName,
                                         const QString& mediaId, qint64 timeoutMs)
{
    return BaseJob::makeRequestUrl(hsData,
                                   makePath("/_matrix/client/v1", "/media/download/", serverName,
                                            "/", mediaId),
                                   queryToGetContentAuthed(timeoutMs));
}

GetContentAuthedJob::GetContentAuthedJob(const QString& serverName, const QString& mediaId,
                                         qint64 timeoutMs)
    : BaseJob(HttpVerb::Get, u"GetContentAuthedJob"_s,
              makePath("/_matrix/client/v1", "/media/download/", serverName, "/", mediaId),
              queryToGetContentAuthed(timeoutMs))
{
    setExpectedContentTypes({ "application/octet-stream" });
}

auto queryToGetContentOverrideNameAuthed(qint64 timeoutMs)
{
    QUrlQuery _q;
    addParam<IfNotEmpty>(_q, u"timeout_ms"_s, timeoutMs);
    return _q;
}

QUrl GetContentOverrideNameAuthedJob::makeRequestUrl(const HomeserverData& hsData,
                                                     const QString& serverName,
                                                     const QString& mediaId,
                                                     const QString& fileName, qint64 timeoutMs)
{
    return BaseJob::makeRequestUrl(hsData,
                                   makePath("/_matrix/client/v1", "/media/download/", serverName,
                                            "/", mediaId, "/", fileName),
                                   queryToGetContentOverrideNameAuthed(timeoutMs));
}

GetContentOverrideNameAuthedJob::GetContentOverrideNameAuthedJob(const QString& serverName,
                                                                 const QString& mediaId,
                                                                 const QString& fileName,
                                                                 qint64 timeoutMs)
    : BaseJob(HttpVerb::Get, u"GetContentOverrideNameAuthedJob"_s,
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
    addParam<>(_q, u"width"_s, width);
    addParam<>(_q, u"height"_s, height);
    addParam<IfNotEmpty>(_q, u"method"_s, method);
    addParam<IfNotEmpty>(_q, u"timeout_ms"_s, timeoutMs);
    addParam<IfNotEmpty>(_q, u"animated"_s, animated);
    return _q;
}

QUrl GetContentThumbnailAuthedJob::makeRequestUrl(const HomeserverData& hsData,
                                                  const QString& serverName, const QString& mediaId,
                                                  int width, int height, const QString& method,
                                                  qint64 timeoutMs, std::optional<bool> animated)
{
    return BaseJob::makeRequestUrl(
        hsData, makePath("/_matrix/client/v1", "/media/thumbnail/", serverName, "/", mediaId),
        queryToGetContentThumbnailAuthed(width, height, method, timeoutMs, animated));
}

GetContentThumbnailAuthedJob::GetContentThumbnailAuthedJob(const QString& serverName,
                                                           const QString& mediaId, int width,
                                                           int height, const QString& method,
                                                           qint64 timeoutMs,
                                                           std::optional<bool> animated)
    : BaseJob(HttpVerb::Get, u"GetContentThumbnailAuthedJob"_s,
              makePath("/_matrix/client/v1", "/media/thumbnail/", serverName, "/", mediaId),
              queryToGetContentThumbnailAuthed(width, height, method, timeoutMs, animated))
{
    setExpectedContentTypes({ "image/jpeg", "image/png", "image/apng", "image/gif", "image/webp" });
}

auto queryToGetUrlPreviewAuthed(const QUrl& url, std::optional<qint64> ts)
{
    QUrlQuery _q;
    addParam<>(_q, u"url"_s, url);
    addParam<IfNotEmpty>(_q, u"ts"_s, ts);
    return _q;
}

QUrl GetUrlPreviewAuthedJob::makeRequestUrl(const HomeserverData& hsData, const QUrl& url,
                                            std::optional<qint64> ts)
{
    return BaseJob::makeRequestUrl(hsData, makePath("/_matrix/client/v1", "/media/preview_url"),
                                   queryToGetUrlPreviewAuthed(url, ts));
}

GetUrlPreviewAuthedJob::GetUrlPreviewAuthedJob(const QUrl& url, std::optional<qint64> ts)
    : BaseJob(HttpVerb::Get, u"GetUrlPreviewAuthedJob"_s,
              makePath("/_matrix/client/v1", "/media/preview_url"),
              queryToGetUrlPreviewAuthed(url, ts))
{}

QUrl GetConfigAuthedJob::makeRequestUrl(const HomeserverData& hsData)
{
    return BaseJob::makeRequestUrl(hsData, makePath("/_matrix/client/v1", "/media/config"));
}

GetConfigAuthedJob::GetConfigAuthedJob()
    : BaseJob(HttpVerb::Get, u"GetConfigAuthedJob"_s,
              makePath("/_matrix/client/v1", "/media/config"))
{}
