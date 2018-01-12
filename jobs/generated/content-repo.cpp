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

UploadContentJob::UploadContentJob(QIODevice* content, const QString& filename, const QString& contentType)
    : BaseJob(HttpVerb::Post, "UploadContentJob",
        basePath % "/upload")
    , d(new Private)
{
    setRequestHeader("Content-Type", contentType.toLatin1());

    QUrlQuery _q;
    if (!filename.isEmpty())
        _q.addQueryItem("filename", filename);
    setRequestQuery(_q);
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

GetContentThumbnailJob::GetContentThumbnailJob(const QString& serverName, const QString& mediaId, int width, int height, const QString& method)
    : BaseJob(HttpVerb::Get, "GetContentThumbnailJob",
        basePath % "/thumbnail/" % serverName % "/" % mediaId, false)
    , d(new Private)
{
    QUrlQuery _q;
    _q.addQueryItem("width", QString("%1").arg(width));
    _q.addQueryItem("height", QString("%1").arg(height));
    if (!method.isEmpty())
        _q.addQueryItem("method", method);
    setRequestQuery(_q);
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
        double matrixImageSize;
        QString ogImage;
};

GetUrlPreviewJob::GetUrlPreviewJob(const QString& url, double ts)
    : BaseJob(HttpVerb::Get, "GetUrlPreviewJob",
        basePath % "/preview_url")
    , d(new Private)
{
    QUrlQuery _q;
    _q.addQueryItem("url", url);
    _q.addQueryItem("ts", QString("%1").arg(ts));
    setRequestQuery(_q);
}

GetUrlPreviewJob::~GetUrlPreviewJob() = default;

double GetUrlPreviewJob::matrixImageSize() const
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
    d->matrixImageSize = fromJson<double>(json.value("matrix:image:size"));
    d->ogImage = fromJson<QString>(json.value("og:image"));
    return Success;
}

