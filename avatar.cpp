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

using namespace QMatrixClient;

QPixmap Avatar::get(int width, int height, Avatar::notifier_t notifier)
{
    QSize size(width, height);

    // FIXME: Alternating between longer-width and longer-height requests
    // is a sure way to trick the below code into constantly getting another
    // image from the server because the existing one is alleged unsatisfactory.
    // This is plain abuse by the client, though; so not critical for now.
    if( ( !(_valid || _ongoingRequest)
          || width > _requestedSize.width()
          || height > _requestedSize.height() ) && _url.isValid() )
    {
        qCDebug(MAIN) << "Getting avatar from" << _url.toString();
        _requestedSize = size;
        if (_ongoingRequest)
            _ongoingRequest->abandon();
        notifiers.emplace_back(std::move(notifier));
        _ongoingRequest = _connection->callApi<MediaThumbnailJob>(_url, size);
        _ongoingRequest->connect( _ongoingRequest, &MediaThumbnailJob::finished,
                                 _connection, [=]() {
            if (_ongoingRequest->status().good())
            {
                _valid = true;
                _originalPixmap = _ongoingRequest->scaledThumbnail(_requestedSize);
                _scaledPixmaps.clear();
                for (auto n: notifiers)
                    n();
            }
            _ongoingRequest = nullptr;
        });
    }

    if( _originalPixmap.isNull() )
    {
        if (_defaultIcon.isNull())
            return _originalPixmap;

        _originalPixmap = _defaultIcon.pixmap(size);
    }

    for (auto p: _scaledPixmaps)
        if (p.first == size)
            return p.second;
    auto pixmap = _originalPixmap.scaled(size,
                    Qt::KeepAspectRatio, Qt::SmoothTransformation);
    _scaledPixmaps.emplace_back(size, pixmap);
    return pixmap;
}

bool Avatar::updateUrl(const QUrl& newUrl)
{
    if (newUrl == _url)
        return false;

    _url = newUrl;
    _valid = false;
    return true;
}
