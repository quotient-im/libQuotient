/******************************************************************************
 * Copyright (C) 2017 Kitsune Ral <kitsune-ral@users.sf.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include "avatar.h"

#include "connection.h"

#include "events/eventcontent.h"
#include "jobs/mediathumbnailjob.h"

#include <QtCore/QDir>
#include <QtCore/QPointer>
#include <QtCore/QStandardPaths>
#include <QtCore/QStringBuilder>
#include <QtGui/QPainter>

using namespace QMatrixClient;
using std::move;

class Avatar::Private
{
public:
    explicit Private(QUrl url = {})
        : _url(move(url))
    {}
    ~Private()
    {
        if (isJobRunning(_thumbnailRequest))
            _thumbnailRequest->abandon();
        if (isJobRunning(_uploadRequest))
            _uploadRequest->abandon();
    }

    QImage get(Connection* connection, QSize size,
               get_callback_t callback) const;
    bool upload(UploadContentJob* job, upload_callback_t callback);

    bool checkUrl(const QUrl& url) const;
    QString localFile() const;

    QUrl _url;

    // The below are related to image caching, hence mutable
    mutable QImage _originalImage;
    mutable std::vector<QPair<QSize, QImage>> _scaledImages;
    mutable QSize _requestedSize;
    mutable enum { Unknown, Cache, Network, Banned } _imageSource = Unknown;
    mutable QPointer<MediaThumbnailJob> _thumbnailRequest = nullptr;
    mutable QPointer<BaseJob> _uploadRequest = nullptr;
    mutable std::vector<get_callback_t> callbacks;
};

Avatar::Avatar()
    : d(std::make_unique<Private>())
{}

Avatar::Avatar(QUrl url)
    : d(std::make_unique<Private>(std::move(url)))
{}

Avatar::Avatar(Avatar&&) = default;

Avatar::~Avatar() = default;

Avatar& Avatar::operator=(Avatar&&) = default;

QImage Avatar::get(Connection* connection, int dimension,
                   get_callback_t callback) const
{
    return d->get(connection, { dimension, dimension }, move(callback));
}

QImage Avatar::get(Connection* connection, int width, int height,
                   get_callback_t callback) const
{
    return d->get(connection, { width, height }, move(callback));
}

bool Avatar::upload(Connection* connection, const QString& fileName,
                    upload_callback_t callback) const
{
    if (isJobRunning(d->_uploadRequest))
        return false;
    return d->upload(connection->uploadFile(fileName), move(callback));
}

bool Avatar::upload(Connection* connection, QIODevice* source,
                    upload_callback_t callback) const
{
    if (isJobRunning(d->_uploadRequest) || !source->isReadable())
        return false;
    return d->upload(connection->uploadContent(source), move(callback));
}

QString Avatar::mediaId() const { return d->_url.authority() + d->_url.path(); }

QImage Avatar::Private::get(Connection* connection, QSize size,
                            get_callback_t callback) const
{
    if (!callback) {
        qCCritical(MAIN) << "Null callbacks are not allowed in Avatar::get";
        Q_ASSERT(false);
    }

    if (_imageSource == Unknown && _originalImage.load(localFile())) {
        _imageSource = Cache;
        _requestedSize = _originalImage.size();
    }

    // Alternating between longer-width and longer-height requests is a sure way
    // to trick the below code into constantly getting another image from
    // the server because the existing one is alleged unsatisfactory.
    // Client authors can only blame themselves if they do so.
    if (((_imageSource == Unknown && !_thumbnailRequest)
         || size.width() > _requestedSize.width()
         || size.height() > _requestedSize.height())
        && checkUrl(_url)) {
        qCDebug(MAIN) << "Getting avatar from" << _url.toString();
        _requestedSize = size;
        if (isJobRunning(_thumbnailRequest))
            _thumbnailRequest->abandon();
        if (callback)
            callbacks.emplace_back(move(callback));
        _thumbnailRequest = connection->getThumbnail(_url, size);
        QObject::connect(_thumbnailRequest, &MediaThumbnailJob::success,
                         _thumbnailRequest, [this] {
                             _imageSource = Network;
                             _originalImage = _thumbnailRequest->scaledThumbnail(
                                 _requestedSize);
                             _originalImage.save(localFile());
                             _scaledImages.clear();
                             for (const auto& n : callbacks)
                                 n();
                             callbacks.clear();
                         });
    }

    for (const auto& p : _scaledImages)
        if (p.first == size)
            return p.second;
    auto result = _originalImage.isNull()
                      ? QImage()
                      : _originalImage.scaled(size, Qt::KeepAspectRatio,
                                              Qt::SmoothTransformation);
    _scaledImages.emplace_back(size, result);
    return result;
}

bool Avatar::Private::upload(UploadContentJob* job, upload_callback_t callback)
{
    _uploadRequest = job;
    if (!isJobRunning(_uploadRequest))
        return false;
    _uploadRequest->connect(_uploadRequest, &BaseJob::success, _uploadRequest,
                            [job, callback] { callback(job->contentUri()); });
    return true;
}

bool Avatar::Private::checkUrl(const QUrl& url) const
{
    if (_imageSource == Banned || url.isEmpty())
        return false;

    // FIXME: Make "mxc" a library-wide constant and maybe even make
    // the URL checker a Connection(?) method.
    if (!url.isValid() || url.scheme() != "mxc" || url.path().count('/') != 1) {
        qCWarning(MAIN) << "Avatar URL is invalid or not mxc-based:"
                        << url.toDisplayString();
        _imageSource = Banned;
    }
    return _imageSource != Banned;
}

QString Avatar::Private::localFile() const
{
    static const auto cachePath = cacheLocation(QStringLiteral("avatars"));
    return cachePath % _url.authority() % '_' % _url.fileName() % ".png";
}

QUrl Avatar::url() const { return d->_url; }

bool Avatar::updateUrl(const QUrl& newUrl)
{
    if (newUrl == d->_url)
        return false;

    d->_url = newUrl;
    d->_imageSource = Private::Unknown;
    if (isJobRunning(d->_thumbnailRequest))
        d->_thumbnailRequest->abandon();
    return true;
}
