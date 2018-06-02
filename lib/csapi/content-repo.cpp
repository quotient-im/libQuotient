/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "content-repo.h"

#include "converters.h"

#include <QtNetwork/QNetworkReply>
#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/_matrix/media/r0");

class UploadContentJob::Private
{
    public:
        QString contentUri;
};

BaseJob::Query queryToUploadContent(const QString& filename)
{
    BaseJob::Query _q;
    if (!filename.isEmpty())
        _q.addQueryItem("filename", filename);
    return _q;
}

UploadContentJob::UploadContentJob(QIODevice* content, const QString& filename, const QString& contentType)
    : BaseJob(HttpVerb::Post, "UploadContentJob",
        basePath % "/upload",
        queryToUploadContent(filename))
    , d(new Private)
{
    setRequestHeader("Content-Type", contentType.toLatin1());

    setRequestData(Data(content));
}

UploadContentJob::~UploadContentJob() = default;

const QString& UploadContentJob::contentUri() const
{
    return d->contentUri;
}

BaseJob::Status UploadContentJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    if (!json.contains("content_uri"))
        return { JsonParseError,
            "The key 'content_uri' not found in the response" };
    d->contentUri = fromJson<QString>(json.value("content_uri"));
    return Success;
}

class GetContentJob::Private
{
    public:
        QString contentType;
        QString contentDisposition;
        QIODevice* content;
};

QUrl GetContentJob::makeRequestUrl(QUrl baseUrl, const QString& serverName, const QString& mediaId)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
            basePath % "/download/" % serverName % "/" % mediaId);
}

GetContentJob::GetContentJob(const QString& serverName, const QString& mediaId)
    : BaseJob(HttpVerb::Get, "GetContentJob",
        basePath % "/download/" % serverName % "/" % mediaId, false)
    , d(new Private)
{
    setExpectedContentTypes({ "*/*" });
}

GetContentJob::~GetContentJob() = default;

const QString& GetContentJob::contentType() const
{
    return d->contentType;
}

const QString& GetContentJob::contentDisposition() const
{
    return d->contentDisposition;
}

QIODevice* GetContentJob::content() const
{
    return d->content;
}

BaseJob::Status GetContentJob::parseReply(QNetworkReply* reply)
{
    d->contentType = reply->rawHeader("Content-Type"); 
    d->contentDisposition = reply->rawHeader("Content-Disposition"); 
    d->content = reply;
    return Success;
}

class GetContentOverrideNameJob::Private
{
    public:
        QString contentType;
        QString contentDisposition;
        QIODevice* content;
};

QUrl GetContentOverrideNameJob::makeRequestUrl(QUrl baseUrl, const QString& serverName, const QString& mediaId, const QString& fileName)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
            basePath % "/download/" % serverName % "/" % mediaId % "/" % fileName);
}

GetContentOverrideNameJob::GetContentOverrideNameJob(const QString& serverName, const QString& mediaId, const QString& fileName)
    : BaseJob(HttpVerb::Get, "GetContentOverrideNameJob",
        basePath % "/download/" % serverName % "/" % mediaId % "/" % fileName, false)
    , d(new Private)
{
    setExpectedContentTypes({ "*/*" });
}

GetContentOverrideNameJob::~GetContentOverrideNameJob() = default;

const QString& GetContentOverrideNameJob::contentType() const
{
    return d->contentType;
}

const QString& GetContentOverrideNameJob::contentDisposition() const
{
    return d->contentDisposition;
}

QIODevice* GetContentOverrideNameJob::content() const
{
    return d->content;
}

BaseJob::Status GetContentOverrideNameJob::parseReply(QNetworkReply* reply)
{
    d->contentType = reply->rawHeader("Content-Type"); 
    d->contentDisposition = reply->rawHeader("Content-Disposition"); 
    d->content = reply;
    return Success;
}

class GetContentThumbnailJob::Private
{
    public:
        QString contentType;
        QIODevice* content;
};

BaseJob::Query queryToGetContentThumbnail(Omittable<int> width, Omittable<int> height, const QString& method)
{
    BaseJob::Query _q;
    if (width)
        _q.addQueryItem("width", QString("%1").arg(width.value()));
    if (height)
        _q.addQueryItem("height", QString("%1").arg(height.value()));
    if (!method.isEmpty())
        _q.addQueryItem("method", method);
    return _q;
}

QUrl GetContentThumbnailJob::makeRequestUrl(QUrl baseUrl, const QString& serverName, const QString& mediaId, Omittable<int> width, Omittable<int> height, const QString& method)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
            basePath % "/thumbnail/" % serverName % "/" % mediaId,
            queryToGetContentThumbnail(width, height, method));
}

GetContentThumbnailJob::GetContentThumbnailJob(const QString& serverName, const QString& mediaId, Omittable<int> width, Omittable<int> height, const QString& method)
    : BaseJob(HttpVerb::Get, "GetContentThumbnailJob",
        basePath % "/thumbnail/" % serverName % "/" % mediaId,
        queryToGetContentThumbnail(width, height, method),
        {}, false)
    , d(new Private)
{
    setExpectedContentTypes({ "image/jpeg", "image/png" });
}

GetContentThumbnailJob::~GetContentThumbnailJob() = default;

const QString& GetContentThumbnailJob::contentType() const
{
    return d->contentType;
}

QIODevice* GetContentThumbnailJob::content() const
{
    return d->content;
}

BaseJob::Status GetContentThumbnailJob::parseReply(QNetworkReply* reply)
{
    d->contentType = reply->rawHeader("Content-Type"); 
    d->content = reply;
    return Success;
}

class GetUrlPreviewJob::Private
{
    public:
        Omittable<qint64> matrixImageSize;
        QString ogImage;
};

BaseJob::Query queryToGetUrlPreview(const QString& url, Omittable<qint64> ts)
{
    BaseJob::Query _q;
    _q.addQueryItem("url", url);
    if (ts)
        _q.addQueryItem("ts", QString("%1").arg(ts.value()));
    return _q;
}

QUrl GetUrlPreviewJob::makeRequestUrl(QUrl baseUrl, const QString& url, Omittable<qint64> ts)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
            basePath % "/preview_url",
            queryToGetUrlPreview(url, ts));
}

GetUrlPreviewJob::GetUrlPreviewJob(const QString& url, Omittable<qint64> ts)
    : BaseJob(HttpVerb::Get, "GetUrlPreviewJob",
        basePath % "/preview_url",
        queryToGetUrlPreview(url, ts))
    , d(new Private)
{
}

GetUrlPreviewJob::~GetUrlPreviewJob() = default;

Omittable<qint64> GetUrlPreviewJob::matrixImageSize() const
{
    return d->matrixImageSize;
}

const QString& GetUrlPreviewJob::ogImage() const
{
    return d->ogImage;
}

BaseJob::Status GetUrlPreviewJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    d->matrixImageSize = fromJson<qint64>(json.value("matrix:image:size"));
    d->ogImage = fromJson<QString>(json.value("og:image"));
    return Success;
}

