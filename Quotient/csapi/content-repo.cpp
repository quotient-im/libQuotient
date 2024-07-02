// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "content-repo.h"

using namespace Quotient;

auto queryToUploadContent(const QString& filename)
{
    QUrlQuery _q;
    addParam<IfNotEmpty>(_q, QStringLiteral("filename"), filename);
    return _q;
}

UploadContentJob::UploadContentJob(QIODevice* content, const QString& filename,
                                   const QString& contentType)
    : BaseJob(HttpVerb::Post, QStringLiteral("UploadContentJob"),
              makePath("/_matrix", "/media/v3/upload"), queryToUploadContent(filename))
{
    setRequestHeader("Content-Type", contentType.toLatin1());
    setRequestData({ content });
    addExpectedKey("content_uri");
}

auto queryToUploadContentToMXC(const QString& filename)
{
    QUrlQuery _q;
    addParam<IfNotEmpty>(_q, QStringLiteral("filename"), filename);
    return _q;
}

UploadContentToMXCJob::UploadContentToMXCJob(const QString& serverName, const QString& mediaId,
                                             QIODevice* content, const QString& filename,
                                             const QString& contentType)
    : BaseJob(HttpVerb::Put, QStringLiteral("UploadContentToMXCJob"),
              makePath("/_matrix", "/media/v3/upload/", serverName, "/", mediaId),
              queryToUploadContentToMXC(filename))
{
    setRequestHeader("Content-Type", contentType.toLatin1());
    setRequestData({ content });
}

QUrl CreateContentJob::makeRequestUrl(QUrl baseUrl)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl), makePath("/_matrix", "/media/v1/create"));
}

CreateContentJob::CreateContentJob()
    : BaseJob(HttpVerb::Post, QStringLiteral("CreateContentJob"),
              makePath("/_matrix", "/media/v1/create"))
{
    addExpectedKey("content_uri");
}

auto queryToGetContent(bool allowRemote, qint64 timeoutMs, bool allowRedirect)
{
    QUrlQuery _q;
    addParam<IfNotEmpty>(_q, QStringLiteral("allow_remote"), allowRemote);
    addParam<IfNotEmpty>(_q, QStringLiteral("timeout_ms"), timeoutMs);
    addParam<IfNotEmpty>(_q, QStringLiteral("allow_redirect"), allowRedirect);
    return _q;
}

QUrl GetContentJob::makeRequestUrl(QUrl baseUrl, const QString& serverName, const QString& mediaId,
                                   bool allowRemote, qint64 timeoutMs, bool allowRedirect)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   makePath("/_matrix", "/media/v3/download/", serverName, "/",
                                            mediaId),
                                   queryToGetContent(allowRemote, timeoutMs, allowRedirect));
}

GetContentJob::GetContentJob(const QString& serverName, const QString& mediaId, bool allowRemote,
                             qint64 timeoutMs, bool allowRedirect)
    : BaseJob(HttpVerb::Get, QStringLiteral("GetContentJob"),
              makePath("/_matrix", "/media/v3/download/", serverName, "/", mediaId),
              queryToGetContent(allowRemote, timeoutMs, allowRedirect), {}, false)
{
    setExpectedContentTypes({ "application/octet-stream" });
}

auto queryToGetContentOverrideName(bool allowRemote, qint64 timeoutMs, bool allowRedirect)
{
    QUrlQuery _q;
    addParam<IfNotEmpty>(_q, QStringLiteral("allow_remote"), allowRemote);
    addParam<IfNotEmpty>(_q, QStringLiteral("timeout_ms"), timeoutMs);
    addParam<IfNotEmpty>(_q, QStringLiteral("allow_redirect"), allowRedirect);
    return _q;
}

QUrl GetContentOverrideNameJob::makeRequestUrl(QUrl baseUrl, const QString& serverName,
                                               const QString& mediaId, const QString& fileName,
                                               bool allowRemote, qint64 timeoutMs,
                                               bool allowRedirect)
{
    return BaseJob::makeRequestUrl(
        std::move(baseUrl),
        makePath("/_matrix", "/media/v3/download/", serverName, "/", mediaId, "/", fileName),
        queryToGetContentOverrideName(allowRemote, timeoutMs, allowRedirect));
}

GetContentOverrideNameJob::GetContentOverrideNameJob(const QString& serverName,
                                                     const QString& mediaId,
                                                     const QString& fileName, bool allowRemote,
                                                     qint64 timeoutMs, bool allowRedirect)
    : BaseJob(HttpVerb::Get, QStringLiteral("GetContentOverrideNameJob"),
              makePath("/_matrix", "/media/v3/download/", serverName, "/", mediaId, "/", fileName),
              queryToGetContentOverrideName(allowRemote, timeoutMs, allowRedirect), {}, false)
{
    setExpectedContentTypes({ "application/octet-stream" });
}

auto queryToGetContentThumbnail(int width, int height, const QString& method, bool allowRemote,
                                qint64 timeoutMs, bool allowRedirect, std::optional<bool> animated)
{
    QUrlQuery _q;
    addParam<>(_q, QStringLiteral("width"), width);
    addParam<>(_q, QStringLiteral("height"), height);
    addParam<IfNotEmpty>(_q, QStringLiteral("method"), method);
    addParam<IfNotEmpty>(_q, QStringLiteral("allow_remote"), allowRemote);
    addParam<IfNotEmpty>(_q, QStringLiteral("timeout_ms"), timeoutMs);
    addParam<IfNotEmpty>(_q, QStringLiteral("allow_redirect"), allowRedirect);
    addParam<IfNotEmpty>(_q, QStringLiteral("animated"), animated);
    return _q;
}

QUrl GetContentThumbnailJob::makeRequestUrl(QUrl baseUrl, const QString& serverName,
                                            const QString& mediaId, int width, int height,
                                            const QString& method, bool allowRemote,
                                            qint64 timeoutMs, bool allowRedirect,
                                            std::optional<bool> animated)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   makePath("/_matrix", "/media/v3/thumbnail/", serverName, "/",
                                            mediaId),
                                   queryToGetContentThumbnail(width, height, method, allowRemote,
                                                              timeoutMs, allowRedirect, animated));
}

GetContentThumbnailJob::GetContentThumbnailJob(const QString& serverName, const QString& mediaId,
                                               int width, int height, const QString& method,
                                               bool allowRemote, qint64 timeoutMs,
                                               bool allowRedirect, std::optional<bool> animated)
    : BaseJob(HttpVerb::Get, QStringLiteral("GetContentThumbnailJob"),
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
    addParam<>(_q, QStringLiteral("url"), url);
    addParam<IfNotEmpty>(_q, QStringLiteral("ts"), ts);
    return _q;
}

QUrl GetUrlPreviewJob::makeRequestUrl(QUrl baseUrl, const QUrl& url, std::optional<qint64> ts)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl), makePath("/_matrix", "/media/v3/preview_url"),
                                   queryToGetUrlPreview(url, ts));
}

GetUrlPreviewJob::GetUrlPreviewJob(const QUrl& url, std::optional<qint64> ts)
    : BaseJob(HttpVerb::Get, QStringLiteral("GetUrlPreviewJob"),
              makePath("/_matrix", "/media/v3/preview_url"), queryToGetUrlPreview(url, ts))
{}

QUrl GetConfigJob::makeRequestUrl(QUrl baseUrl)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl), makePath("/_matrix", "/media/v3/config"));
}

GetConfigJob::GetConfigJob()
    : BaseJob(HttpVerb::Get, QStringLiteral("GetConfigJob"),
              makePath("/_matrix", "/media/v3/config"))
{}
