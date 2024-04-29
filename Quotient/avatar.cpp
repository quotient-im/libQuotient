// SPDX-FileCopyrightText: 2017 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "avatar.h"

#include "connection.h"
#include "logging_categories_p.h"

#include "jobs/mediathumbnailjob.h"

#include <QtCore/QDir>
#include <QtCore/QStandardPaths>
#include <QtCore/QStringBuilder>
#include <QtGui/QPainter>

using namespace Quotient;

class Q_DECL_HIDDEN Avatar::Private {
public:
    explicit Private(Connection* c, QUrl url = {}) : connection(c), _url(std::move(url)) {}
    ~Private()
    {
        thumbnailRequest.abandon();
        uploadRequest.abandon();
    }
    Q_DISABLE_COPY_MOVE(Private)

    QImage get(Connection* connection, QSize size,
               get_callback_t callback) const;
    void thumbnailRequestFinished() const;

    bool checkUrl(const QUrl& url) const;
    QString localFile() const;

    Connection* connection;
    QUrl _url;

    // The below are related to image caching, hence mutable
    mutable QImage originalImage;
    mutable std::vector<std::pair<QSize, QImage>> scaledImages;
    mutable QSize largestRequestedSize{};
    enum ImageSource : quint8 { Unknown, Cache, Network, Invalid };
    mutable ImageSource imageSource = Unknown;
    mutable JobHandle<MediaThumbnailJob> thumbnailRequest = nullptr;
    mutable JobHandle<UploadContentJob> uploadRequest = nullptr;
    mutable std::vector<get_callback_t> callbacks{};
};

Avatar::Avatar(Connection* c, QUrl url) : d(makeImpl<Private>(c, std::move(url))) {}

QImage Avatar::get(int dimension, get_callback_t callback) const
{
    return d->get({ dimension, dimension }, std::move(callback));
}

QImage Avatar::get(int width, int height, get_callback_t callback) const
{
    return d->get({ width, height }, std::move(callback));
}

bool Avatar::upload(const QString& fileName, upload_callback_t callback) const
{
    if (isJobPending(d->uploadRequest))
        return false;
    upload(fileName).then(std::move(callback));
    return true;
}

bool Avatar::upload(QIODevice* source, upload_callback_t callback) const
{
    if (isJobPending(d->uploadRequest) || !source->isReadable())
        return false;
    upload(source).then(std::move(callback));
    return true;
}

QFuture<QUrl> Avatar::upload(const QString& fileName) const
{
    d->uploadRequest = d->connection->uploadFile(fileName);
    return d->uploadRequest.responseFuture();
}

QFuture<QUrl> Avatar::upload(QIODevice* source) const
{
    d->uploadRequest = d->connection->uploadContent(source);
    return d->uploadRequest.responseFuture();
}

QString Avatar::mediaId() const { return d->_url.authority() + d->_url.path(); }

QImage Avatar::Private::get(QSize size, get_callback_t callback) const
{
    if (imageSource == Unknown && originalImage.load(localFile())) {
        imageSource = Cache;
        largestRequestedSize = originalImage.size();
    }

    // Assuming that all thumbnails for this avatar have the same aspect ratio,
    // it's enough for the image requested before to be large enough in at least
    // one dimension to be suitable for scaling down to the requested size;
    // therefore the new size has to be larger in both dimensions to warrant a
    // new request to the server
    if (((imageSource == Unknown && !thumbnailRequest)
         || (size.width() > largestRequestedSize.width()
             && size.height() > largestRequestedSize.height()))
        && checkUrl(_url)) {
        qCDebug(MAIN) << "Getting avatar from" << _url.toString();
        largestRequestedSize = size;
        thumbnailRequest.abandon();
        if (callback)
            callbacks.emplace_back(std::move(callback));
        thumbnailRequest = connection->getThumbnail(_url, size);
        thumbnailRequest.onResult([this] { thumbnailRequestFinished(); });
        // The result of this request will only be returned when get() is
        // called next time afterwards
    }
    if (imageSource == Invalid || originalImage.isNull())
        return {};

    // NB: because of KeepAspectRatio, scaledImage.size() might not be equal to
    // requestedSize - this is why requestedSize is stored separately
    for (const auto& [requestedSize, scaledImage] : scaledImages)
        if (requestedSize == size)
            return scaledImage;

    const auto& result = originalImage.scaled(size, Qt::KeepAspectRatio,
                                               Qt::SmoothTransformation);
    scaledImages.emplace_back(size, result);
    return result;
}

void Avatar::Private::thumbnailRequestFinished() const
{
    // NB: The following code preserves _originalImage in case of
    // most errors
    switch (thumbnailRequest->error()) {
    case BaseJob::NoError: break;
    case BaseJob::NetworkError:
    case BaseJob::NetworkAuthRequired:
    case BaseJob::TooManyRequests: // Shouldn't reach here but just in case
    case BaseJob::Timeout:
        return; // Make another attempt when requested again
    default:
        // Other errors are likely unrecoverable but just in case,
        // check if there's a previous image to fall back to; if
        // there is, assume that the error is temporary
        if (originalImage.isNull())
            imageSource = Invalid; // Can't do much with the rest
        return;
    }
    auto&& img = thumbnailRequest->thumbnail();
    if (img.format() == QImage::Format_Invalid) {
        qCWarning(MAIN) << "The request for" << _url
                        << "was successful but the received image "
                           "is invalid or unsupported";
        return;
    }
    imageSource = Network;
    originalImage = std::move(img);
    originalImage.save(localFile());
    scaledImages.clear();
    for (auto&& n : callbacks)
        n();
    callbacks.clear();
}

bool Avatar::Private::checkUrl(const QUrl& url) const
{
    if (imageSource == Invalid || url.isEmpty())
        return false;

    // FIXME: Make "mxc" a library-wide constant and maybe even make
    // the URL checker a Connection(?) method.
    if (!url.isValid() || url.scheme() != "mxc"_L1 || url.path().count(u'/') != 1) {
        qCWarning(MAIN) << "Avatar URL is invalid or not mxc-based:"
                        << url.toDisplayString();
        imageSource = Invalid;
    }
    return imageSource != Invalid;
}

QString Avatar::Private::localFile() const
{
    static const auto cachePath = cacheLocation(u"avatars");
    return cachePath % _url.authority() % u'_' % _url.fileName() % ".png"_L1;
}

QUrl Avatar::url() const { return d->_url; }

bool Avatar::updateUrl(const QUrl& newUrl)
{
    if (newUrl == d->_url)
        return false;

    d->_url = newUrl;
    d->imageSource = Private::Unknown;
    d->originalImage = {};
    d->scaledImages.clear();
    d->thumbnailRequest.abandon();
    return true;
}
