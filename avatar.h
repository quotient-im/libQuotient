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
            explicit Avatar(Connection* connection, QIcon defaultIcon = {});
            ~Avatar();

            using notifier_t = std::function<void()>;

            QImage get(int dimension, notifier_t notifier);
            QImage get(int w, int h, notifier_t notifier);

            QUrl url() const;
            bool updateUrl(const QUrl& newUrl);

        private:
            class Private;
            QScopedPointer<Private> d;
    };
}  // namespace QMatrixClient
