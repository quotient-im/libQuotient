#include <utility>

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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "avatar.h"

#include "jobs/mediathumbnailjob.h"
#include "events/eventcontent.h"
#include "connection.h"

#include <QtGui/QPainter>
#include <QtCore/QPointer>

using namespace QMatrixClient;
using std::move;

class Avatar::Private
{
    public:
        explicit Private(QIcon di, QUrl url = {})
            : _defaultIcon(move(di)), _url(move(url))
        { }
        QImage get(Connection* connection, QSize size,
                   get_callback_t callback) const;
        bool upload(UploadContentJob* job, upload_callback_t callback);

        bool checkUrl(QUrl url) const;

        const QIcon _defaultIcon;
        QUrl _url;

        // The below are related to image caching, hence mutable
        mutable QImage _originalImage;
        mutable std::vector<QPair<QSize, QImage>> _scaledImages;
        mutable QSize _requestedSize;
        mutable bool _bannedUrl = false;
        mutable bool _fetched = false;
        mutable QPointer<MediaThumbnailJob> _thumbnailRequest = nullptr;
        mutable QPointer<BaseJob> _uploadRequest = nullptr;
        mutable std::vector<get_callback_t> callbacks;
};

Avatar::Avatar(QIcon defaultIcon)
    : d(std::make_unique<Private>(std::move(defaultIcon)))
{ }

Avatar::Avatar(QUrl url, QIcon defaultIcon)
    : d(std::make_unique<Private>(std::move(defaultIcon), std::move(url)))
{ }

Avatar::Avatar(Avatar&&) = default;

Avatar::~Avatar() = default;

Avatar& Avatar::operator=(Avatar&&) = default;

QImage Avatar::get(Connection* connection, int dimension,
                   get_callback_t callback) const
{
    return d->get(connection, {dimension, dimension}, move(callback));
}

QImage Avatar::get(Connection* connection, int width, int height,
                   get_callback_t callback) const
{
    return d->get(connection, {width, height}, move(callback));
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

QString Avatar::mediaId() const
{
    return d->_url.authority() + d->_url.path();
}

QImage Avatar::Private::get(Connection* connection, QSize size,
                            get_callback_t callback) const
{
    if (!callback)
    {
        qCCritical(MAIN) << "Null callbacks are not allowed in Avatar::get";
        Q_ASSERT(false);
    }
    // FIXME: Alternating between longer-width and longer-height requests
    // is a sure way to trick the below code into constantly getting another
    // image from the server because the existing one is alleged unsatisfactory.
    // This is plain abuse by the client, though; so not critical for now.
    if( ( !(_fetched || _thumbnailRequest)
          || size.width() > _requestedSize.width()
          || size.height() > _requestedSize.height() ) && checkUrl(_url) )
    {
        qCDebug(MAIN) << "Getting avatar from" << _url.toString();
        _requestedSize = size;
        if (isJobRunning(_thumbnailRequest))
            _thumbnailRequest->abandon();
        if (callback)
            callbacks.emplace_back(move(callback));
        _thumbnailRequest = connection->getThumbnail(_url, size);
        if (_originalImage.isNull() && !_defaultIcon.isNull())
        {
            _originalImage = QImage(_defaultIcon.actualSize(size),
                                    QImage::Format_ARGB32_Premultiplied);
            _originalImage.fill(Qt::transparent);
            QPainter p { &_originalImage };
            _defaultIcon.paint(&p, { QPoint(), _defaultIcon.actualSize(size) });
        }
        QObject::connect( _thumbnailRequest, &MediaThumbnailJob::success,
            _thumbnailRequest, [this] {
                _fetched = true;
                _originalImage =
                        _thumbnailRequest->scaledThumbnail(_requestedSize);
                _scaledImages.clear();
                for (const auto& n: callbacks)
                    n();
                callbacks.clear();
            });
    }

    for (const auto& p: _scaledImages)
        if (p.first == size)
            return p.second;
    auto result = _originalImage.scaled(size,
                    Qt::KeepAspectRatio, Qt::SmoothTransformation);
    _scaledImages.emplace_back(size, result);
    return result;
}

bool Avatar::Private::upload(UploadContentJob* job, upload_callback_t callback)
{
    _uploadRequest = job;
    if (!isJobRunning(_uploadRequest))
        return false;
    _uploadRequest->connect(_uploadRequest, &BaseJob::success, _uploadRequest,
                            [job,callback] { callback(job->contentUri()); });
    return true;
}

bool Avatar::Private::checkUrl(QUrl url) const
{
    if (_bannedUrl || url.isEmpty())
        return false;

    // FIXME: Make "mxc" a library-wide constant and maybe even make
    // the URL checker a Connection(?) method.
    _bannedUrl = !(url.isValid() &&
            url.scheme() == "mxc" && url.path().count('/') == 1);
    if (_bannedUrl)
        qCWarning(MAIN) << "Avatar URL is invalid or not mxc-based:"
                        << url.toDisplayString();
    return !_bannedUrl;
}

QUrl Avatar::url() const { return d->_url; }

bool Avatar::updateUrl(const QUrl& newUrl)
{
    if (newUrl == d->_url)
        return false;

    d->_url = newUrl;
    d->_fetched = false;
    if (isJobRunning(d->_thumbnailRequest))
        d->_thumbnailRequest->abandon();
    return true;
}

