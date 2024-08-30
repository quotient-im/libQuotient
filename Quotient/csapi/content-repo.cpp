// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "content-repo.h"

using namespace Quotient;

auto queryToUploadContent(const QString& filename)
{
    QUrlQuery _q;
    addParam<IfNotEmpty>(_q, u"filename"_s, filename);
    return _q;
}

UploadContentJob::UploadContentJob(QIODevice* content, const QString& filename,
                                   const QString& contentType)
    : BaseJob(HttpVerb::Post, u"UploadContentJob"_s, makePath("/_matrix", "/media/v3/upload"),
              queryToUploadContent(filename))
{
    setRequestHeader("Content-Type", contentType.toLatin1());
    setRequestData({ content });
    addExpectedKey("content_uri");
}

auto queryToUploadContentToMXC(const QString& filename)
{
    QUrlQuery _q;
    addParam<IfNotEmpty>(_q, u"filename"_s, filename);
    return _q;
}

UploadContentToMXCJob::UploadContentToMXCJob(const QString& serverName, const QString& mediaId,
                                             QIODevice* content, const QString& filename,
                                             const QString& contentType)
    : BaseJob(HttpVerb::Put, u"UploadContentToMXCJob"_s,
              makePath("/_matrix", "/media/v3/upload/", serverName, "/", mediaId),
              queryToUploadContentToMXC(filename))
{
    setRequestHeader("Content-Type", contentType.toLatin1());
    setRequestData({ content });
}

QUrl CreateContentJob::makeRequestUrl(const HomeserverData& hsData)
{
    return BaseJob::makeRequestUrl(hsData, makePath("/_matrix", "/media/v1/create"));
}

CreateContentJob::CreateContentJob()
    : BaseJob(HttpVerb::Post, u"CreateContentJob"_s, makePath("/_matrix", "/media/v1/create"))
{
    addExpectedKey("content_uri");
}

auto queryToGetContent(bool allowRemote, qint64 timeoutMs, bool allowRedirect)
{
    QUrlQuery _q;
    addParam<IfNotEmpty>(_q, u"allow_remote"_s, allowRemote);
    addParam<IfNotEmpty>(_q, u"timeout_ms"_s, timeoutMs);
    addParam<IfNotEmpty>(_q, u"allow_redirect"_s, allowRedirect);
    return _q;
}

QUrl GetContentJob::makeRequestUrl(const HomeserverData& hsData, const QString& serverName,
                                   const QString& mediaId, bool allowRemote, qint64 timeoutMs,
                                   bool allowRedirect)
{
    return BaseJob::makeRequestUrl(hsData,
                                   makePath("/_matrix", "/media/v3/download/", serverName, "/",
                                            mediaId),
                                   queryToGetContent(allowRemote, timeoutMs, allowRedirect));
}

GetContentJob::GetContentJob(const QString& serverName, const QString& mediaId, bool allowRemote,
                             qint64 timeoutMs, bool allowRedirect)
    : BaseJob(HttpVerb::Get, u"GetContentJob"_s,
              makePath("/_matrix", "/media/v3/download/", serverName, "/", mediaId),
              queryToGetContent(allowRemote, timeoutMs, allowRedirect), {}, false)
{
    setExpectedContentTypes({ "application/octet-stream" });
}

auto queryToGetContentOverrideName(bool allowRemote, qint64 timeoutMs, bool allowRedirect)
{
    QUrlQuery _q;
    addParam<IfNotEmpty>(_q, u"allow_remote"_s, allowRemote);
    addParam<IfNotEmpty>(_q, u"timeout_ms"_s, timeoutMs);
    addParam<IfNotEmpty>(_q, u"allow_redirect"_s, allowRedirect);
    return _q;
}

QUrl GetContentOverrideNameJob::makeRequestUrl(const HomeserverData& hsData,
                                               const QString& serverName, const QString& mediaId,
                                               const QString& fileName, bool allowRemote,
                                               qint64 timeoutMs, bool allowRedirect)
{
    return BaseJob::makeRequestUrl(
        hsData, makePath("/_matrix", "/media/v3/download/", serverName, "/", mediaId, "/", fileName),
        queryToGetContentOverrideName(allowRemote, timeoutMs, allowRedirect));
}

GetContentOverrideNameJob::GetContentOverrideNameJob(const QString& serverName,
                                                     const QString& mediaId,
                                                     const QString& fileName, bool allowRemote,
                                                     qint64 timeoutMs, bool allowRedirect)
    : BaseJob(HttpVerb::Get, u"GetContentOverrideNameJob"_s,
              makePath("/_matrix", "/media/v3/download/", serverName, "/", mediaId, "/", fileName),
              queryToGetContentOverrideName(allowRemote, timeoutMs, allowRedirect), {}, false)
{
    setExpectedContentTypes({ "application/octet-stream" });
}

auto queryToGetContentThumbnail(int width, int height, const QString& method, bool allowRemote,
                                qint64 timeoutMs, bool allowRedirect, std::optional<bool> animated)
{
    QUrlQuery _q;
    addParam<>(_q, u"width"_s, width);
    addParam<>(_q, u"height"_s, height);
    addParam<IfNotEmpty>(_q, u"method"_s, method);
    addParam<IfNotEmpty>(_q, u"allow_remote"_s, allowRemote);
    addParam<IfNotEmpty>(_q, u"timeout_ms"_s, timeoutMs);
    addParam<IfNotEmpty>(_q, u"allow_redirect"_s, allowRedirect);
    addParam<IfNotEmpty>(_q, u"animated"_s, animated);
    return _q;
}

QUrl GetContentThumbnailJob::makeRequestUrl(const HomeserverData& hsData, const QString& serverName,
                                            const QString& mediaId, int width, int height,
                                            const QString& method, bool allowRemote,
                                            qint64 timeoutMs, bool allowRedirect,
                                            std::optional<bool> animated)
{
    return BaseJob::makeRequestUrl(hsData,
                                   makePath("/_matrix", "/media/v3/thumbnail/", serverName, "/",
                                            mediaId),
                                   queryToGetContentThumbnail(width, height, method, allowRemote,
                                                              timeoutMs, allowRedirect, animated));
}

GetContentThumbnailJob::GetContentThumbnailJob(const QString& serverName, const QString& mediaId,
                                               int width, int height, const QString& method,
                                               bool allowRemote, qint64 timeoutMs,
                                               bool allowRedirect, std::optional<bool> animated)
    : BaseJob(HttpVerb::Get, u"GetContentThumbnailJob"_s,
              makePath("/_matrix", "/media/v3/thumbnail/", serverName, "/", mediaId),
              queryToGetContentThumbnail(width, height, method, allowRemote, timeoutMs,
                                         allowRedirect, animated),
              {}, false)
{
    setExpectedContentTypes({ "image/jpeg", "image/png", "image/apng", "image/gif", "image/webp" });
}

auto queryToGetUrlPreview(const QUrl& url, std::optional<qint64> ts)
{
    QUrlQuery _q;
    addParam<>(_q, u"url"_s, url);
    addParam<IfNotEmpty>(_q, u"ts"_s, ts);
    return _q;
}

QUrl GetUrlPreviewJob::makeRequestUrl(const HomeserverData& hsData, const QUrl& url,
                                      std::optional<qint64> ts)
{
    return BaseJob::makeRequestUrl(hsData, makePath("/_matrix", "/media/v3/preview_url"),
                                   queryToGetUrlPreview(url, ts));
}

GetUrlPreviewJob::GetUrlPreviewJob(const QUrl& url, std::optional<qint64> ts)
    : BaseJob(HttpVerb::Get, u"GetUrlPreviewJob"_s, makePath("/_matrix", "/media/v3/preview_url"),
              queryToGetUrlPreview(url, ts))
{}

QUrl GetConfigJob::makeRequestUrl(const HomeserverData& hsData)
{
    return BaseJob::makeRequestUrl(hsData, makePath("/_matrix", "/media/v3/config"));
}

GetConfigJob::GetConfigJob()
    : BaseJob(HttpVerb::Get, u"GetConfigJob"_s, makePath("/_matrix", "/media/v3/config"))
{}
