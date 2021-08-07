/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "content-repo.h"

#include <QtCore/QStringBuilder>

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
              QStringLiteral("/_matrix/media/r0") % "/upload",
              queryToUploadContent(filename))
{
    setRequestHeader("Content-Type", contentType.toLatin1());
    setRequestData(Data(content));
    addExpectedKey("content_uri");
}

auto queryToGetContent(bool allowRemote)
{
    QUrlQuery _q;
    addParam<IfNotEmpty>(_q, QStringLiteral("allow_remote"), allowRemote);
    return _q;
}

QUrl GetContentJob::makeRequestUrl(QUrl baseUrl, const QString& serverName,
                                   const QString& mediaId, bool allowRemote)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   QStringLiteral("/_matrix/media/r0")
                                       % "/download/" % serverName % "/"
                                       % mediaId,
                                   queryToGetContent(allowRemote));
}

GetContentJob::GetContentJob(const QString& serverName, const QString& mediaId,
                             bool allowRemote)
    : BaseJob(HttpVerb::Get, QStringLiteral("GetContentJob"),
              QStringLiteral("/_matrix/media/r0") % "/download/" % serverName
                  % "/" % mediaId,
              queryToGetContent(allowRemote), {}, false)
{
    setExpectedContentTypes({ "*/*" });
}

auto queryToGetContentOverrideName(bool allowRemote)
{
    QUrlQuery _q;
    addParam<IfNotEmpty>(_q, QStringLiteral("allow_remote"), allowRemote);
    return _q;
}

QUrl GetContentOverrideNameJob::makeRequestUrl(QUrl baseUrl,
                                               const QString& serverName,
                                               const QString& mediaId,
                                               const QString& fileName,
                                               bool allowRemote)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   QStringLiteral("/_matrix/media/r0")
                                       % "/download/" % serverName % "/"
                                       % mediaId % "/" % fileName,
                                   queryToGetContentOverrideName(allowRemote));
}

GetContentOverrideNameJob::GetContentOverrideNameJob(const QString& serverName,
                                                     const QString& mediaId,
                                                     const QString& fileName,
                                                     bool allowRemote)
    : BaseJob(HttpVerb::Get, QStringLiteral("GetContentOverrideNameJob"),
              QStringLiteral("/_matrix/media/r0") % "/download/" % serverName
                  % "/" % mediaId % "/" % fileName,
              queryToGetContentOverrideName(allowRemote), {}, false)
{
    setExpectedContentTypes({ "*/*" });
}

auto queryToGetContentThumbnail(int width, int height, const QString& method,
                                bool allowRemote)
{
    QUrlQuery _q;
    addParam<>(_q, QStringLiteral("width"), width);
    addParam<>(_q, QStringLiteral("height"), height);
    addParam<IfNotEmpty>(_q, QStringLiteral("method"), method);
    addParam<IfNotEmpty>(_q, QStringLiteral("allow_remote"), allowRemote);
    return _q;
}

QUrl GetContentThumbnailJob::makeRequestUrl(QUrl baseUrl,
                                            const QString& serverName,
                                            const QString& mediaId, int width,
                                            int height, const QString& method,
                                            bool allowRemote)
{
    return BaseJob::makeRequestUrl(
        std::move(baseUrl),
        QStringLiteral("/_matrix/media/r0") % "/thumbnail/" % serverName % "/"
            % mediaId,
        queryToGetContentThumbnail(width, height, method, allowRemote));
}

GetContentThumbnailJob::GetContentThumbnailJob(const QString& serverName,
                                               const QString& mediaId, int width,
                                               int height, const QString& method,
                                               bool allowRemote)
    : BaseJob(HttpVerb::Get, QStringLiteral("GetContentThumbnailJob"),
              QStringLiteral("/_matrix/media/r0") % "/thumbnail/" % serverName
                  % "/" % mediaId,
              queryToGetContentThumbnail(width, height, method, allowRemote),
              {}, false)
{
    setExpectedContentTypes({ "image/jpeg", "image/png" });
}

auto queryToGetUrlPreview(const QUrl& url, Omittable<qint64> ts)
{
    QUrlQuery _q;
    addParam<>(_q, QStringLiteral("url"), url);
    addParam<IfNotEmpty>(_q, QStringLiteral("ts"), ts);
    return _q;
}

QUrl GetUrlPreviewJob::makeRequestUrl(QUrl baseUrl, const QUrl& url,
                                      Omittable<qint64> ts)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   QStringLiteral("/_matrix/media/r0")
                                       % "/preview_url",
                                   queryToGetUrlPreview(url, ts));
}

GetUrlPreviewJob::GetUrlPreviewJob(const QUrl& url, Omittable<qint64> ts)
    : BaseJob(HttpVerb::Get, QStringLiteral("GetUrlPreviewJob"),
              QStringLiteral("/_matrix/media/r0") % "/preview_url",
              queryToGetUrlPreview(url, ts))
{}

QUrl GetConfigJob::makeRequestUrl(QUrl baseUrl)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   QStringLiteral("/_matrix/media/r0")
                                       % "/config");
}

GetConfigJob::GetConfigJob()
    : BaseJob(HttpVerb::Get, QStringLiteral("GetConfigJob"),
              QStringLiteral("/_matrix/media/r0") % "/config")
{}
