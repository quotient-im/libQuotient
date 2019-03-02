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

#pragma once

#include <QtCore/QUrl>
#include <QtGui/QIcon>

#include <functional>
#include <memory>

namespace QMatrixClient {
    class Connection;

    class Avatar
    {
        public:
        explicit Avatar();
        explicit Avatar(QUrl url);
        Avatar(Avatar&&);
        ~Avatar();
        Avatar& operator=(Avatar&&);

        using get_callback_t = std::function<void()>;
        using upload_callback_t = std::function<void(QString)>;

        QImage get(Connection* connection, int dimension,
                   get_callback_t callback) const;
        QImage get(Connection* connection, int w, int h,
                   get_callback_t callback) const;

        bool upload(Connection* connection, const QString& fileName,
                    upload_callback_t callback) const;
        bool upload(Connection* connection, QIODevice* source,
                    upload_callback_t callback) const;

        QString mediaId() const;
        QUrl url() const;
        bool updateUrl(const QUrl& newUrl);

        private:
        class Private;
        std::unique_ptr<Private> d;
    };
} // namespace QMatrixClient
