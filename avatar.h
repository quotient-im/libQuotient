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

#pragma once

#include <QtGui/QIcon>
#include <QtCore/QUrl>

#include <functional>

namespace QMatrixClient
{
    class MediaThumbnailJob;
    class Connection;

    class Avatar
    {
        public:
            explicit Avatar(Connection* connection, QIcon defaultIcon = {})
                : _defaultIcon(std::move(defaultIcon)), _connection(connection)
            { }

            QPixmap get(int w, int h, std::function<void()> continuation);

            QUrl url() const { return _url; }
            bool updateUrl(const QUrl& newUrl);

        private:
            QUrl _url;
            QPixmap _originalPixmap;
            QIcon _defaultIcon;

            /// Map of requested size to the actual pixmap used for it
            /// (it's a shame that QSize has no predefined qHash()).
            QHash<QPair<int,int>, QPixmap> _scaledPixmaps;

            QSize _requestedSize;
            bool _valid = false;
            Connection* _connection;
            MediaThumbnailJob* _ongoingRequest = nullptr;
    };
}  // namespace QMatrixClient
