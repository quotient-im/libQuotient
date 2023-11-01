// SPDX-FileCopyrightText: 2017 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "avatar.h"

#include "connection.h"
#include "logging_categories_p.h"

#include "jobs/mediathumbnailjob.h"

#include <QtCore/QDir>
#include <QtCore/QPointer>
#include <QtCore/QStandardPaths>
#include <QtCore/QStringBuilder>
#include <QtGui/QPainter>

using namespace Quotient;

class Q_DECL_HIDDEN Avatar::Private : public QObject {
public:
    explicit Private(QUrl url = {}) : _url(std::move(url)) {}
    ~Private() override
    {
        if (isJobPending(_thumbnailRequest))
            _thumbnailRequest->abandon();
        if (isJobPending(_uploadRequest))
            _uploadRequest->abandon();
    }
    Q_DISABLE_COPY_MOVE(Private)

    QImage get(Connection* connection, QSize size,
               get_callback_t callback) const;
    void thumbnailRequestFinished();
    bool upload(UploadContentJob* job, upload_callback_t&& callback);

    bool checkUrl(const QUrl& url) const;
    QString localFile() const;

    QUrl _url;

    // The below are related to image caching, hence mutable
    mutable QImage _originalImage;
    mutable std::vector<std::pair<QSize, QImage>> _scaledImages;
    mutable QSize _largestRequestedSize{};
    enum ImageSource : quint8 { Unknown, Cache, Network, Invalid };
    mutable ImageSource _imageSource = Unknown;
    mutable QPointer<MediaThumbnailJob> _thumbnailRequest = nullptr;
    mutable QPointer<BaseJob> _uploadRequest = nullptr;
    mutable std::vector<get_callback_t> callbacks{};
};

Avatar::Avatar() : d(makeImpl<Private>()) {}

Avatar::Avatar(QUrl url) : d(makeImpl<Private>(std::move(url))) {}

QImage Avatar::get(Connection* connection, int dimension,
                   get_callback_t callback) const
{
    return d->get(connection, { dimension, dimension }, std::move(callback));
}

QImage Avatar::get(Connection* connection, int width, int height,
                   get_callback_t callback) const
{
    return d->get(connection, { width, height }, std::move(callback));
}

bool Avatar::upload(Connection* connection, const QString& fileName,
                    upload_callback_t callback) const
{
    if (isJobPending(d->_uploadRequest))
        return false;
    return d->upload(connection->uploadFile(fileName), std::move(callback));
}

bool Avatar::upload(Connection* connection, QIODevice* source,
                    upload_callback_t callback) const
{
    if (isJobPending(d->_uploadRequest) || !source->isReadable())
        return false;
    return d->upload(connection->uploadContent(source), std::move(callback));
}

QString Avatar::mediaId() const { return d->_url.authority() + d->_url.path(); }

QImage Avatar::Private::get(Connection* connection, QSize size,
                            get_callback_t callback) const
{
    if (_imageSource == Unknown && _originalImage.load(localFile())) {
        _imageSource = Cache;
        _largestRequestedSize = _originalImage.size();
    }

    // Assuming that all thumbnails for this avatar have the same aspect ratio,
    // it's enough for the image requested before to be large enough in at least
    // one dimension to be suitable for scaling down to the requested size;
    // therefore the new size has to be larger in both dimensions to warrant a
    // new request to the server
    if (((_imageSource == Unknown && !_thumbnailRequest)
         || (size.width() > _largestRequestedSize.width()
             && size.height() > _largestRequestedSize.height()))
        && checkUrl(_url)) {
        qCDebug(MAIN) << "Getting avatar from" << _url.toString();
        _largestRequestedSize = size;
        if (isJobPending(_thumbnailRequest))
            _thumbnailRequest->abandon();
        if (callback)
            callbacks.emplace_back(std::move(callback));
        _thumbnailRequest = connection->getThumbnail(_url, size);
        connect(_thumbnailRequest, &MediaThumbnailJob::finished, this,
                &Private::thumbnailRequestFinished);
        // The result of this request will only be returned when get() is
        // called next time afterwards
    }
    if (_imageSource == Invalid || _originalImage.isNull())
        return {};

    // NB: because of KeepAspectRatio, scaledImage.size() might not be equal to
    // requestedSize - this is why requestedSize is stored separately
    for (const auto& [requestedSize, scaledImage] : _scaledImages)
        if (requestedSize == size)
            return scaledImage;

    const auto& result = _originalImage.scaled(size, Qt::KeepAspectRatio,
                                               Qt::SmoothTransformation);
    _scaledImages.emplace_back(size, result);
    return result;
}

void Avatar::Private::thumbnailRequestFinished()
{
    // NB: The following code preserves _originalImage in case of
    // most errors
    switch (_thumbnailRequest->error()) {
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
        if (_originalImage.isNull())
            _imageSource = Invalid; // Can't do much with the rest
        return;
    }
    auto&& img = _thumbnailRequest->thumbnail();
    if (img.format() == QImage::Format_Invalid) {
        qCWarning(MAIN) << "The request for" << _url
                        << "was successful but the received image "
                           "is invalid or unsupported";
        return;
    }
    _imageSource = Network;
    _originalImage = std::move(img);
    _originalImage.save(localFile());
    _scaledImages.clear();
    for (const auto& n : callbacks)
        n();
    callbacks.clear();
}

bool Avatar::Private::upload(UploadContentJob* job, upload_callback_t &&callback)
{
    _uploadRequest = job;
    if (!isJobPending(_uploadRequest))
        return false;
    _uploadRequest->connect(_uploadRequest, &BaseJob::success, _uploadRequest,
                            [job, callback] { callback(job->contentUri()); });
    return true;
}

bool Avatar::Private::checkUrl(const QUrl& url) const
{
    if (_imageSource == Invalid || url.isEmpty())
        return false;

    // FIXME: Make "mxc" a library-wide constant and maybe even make
    // the URL checker a Connection(?) method.
    if (!url.isValid() || url.scheme() != "mxc"_ls || url.path().count(u'/') != 1) {
        qCWarning(MAIN) << "Avatar URL is invalid or not mxc-based:"
                        << url.toDisplayString();
        _imageSource = Invalid;
    }
    return _imageSource != Invalid;
}

QString Avatar::Private::localFile() const
{
    static const auto cachePath = cacheLocation(QStringLiteral("avatars"));
    return cachePath % _url.authority() % u'_' % _url.fileName() % ".png"_ls;
}

QUrl Avatar::url() const { return d->_url; }

bool Avatar::updateUrl(const QUrl& newUrl)
{
    if (newUrl == d->_url)
        return false;

    d->_url = newUrl;
    d->_imageSource = Private::Unknown;
    d->_originalImage = {};
    d->_scaledImages.clear();
    if (isJobPending(d->_thumbnailRequest))
        d->_thumbnailRequest->abandon();
    return true;
}
