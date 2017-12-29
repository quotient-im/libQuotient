/******************************************************************************
 * Copyright (C) 2015 Felix Rohrbach <kde@fxrh.de>
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

#include <QtCore/QUrl>

#include <functional>
#include <memory>

class QNetworkAccessManager;

namespace QMatrixClient
{
    class ConnectionData
    {
        public:
            explicit ConnectionData(QUrl baseUrl);
            virtual ~ConnectionData();

            QByteArray accessToken() const;
            QUrl baseUrl() const;
            const QString& deviceId() const;

            QNetworkAccessManager* nam() const;
            void setBaseUrl(QUrl baseUrl);
            void setToken(QByteArray accessToken);
            void setHost( QString host );
            void setPort( int port );
            void setDeviceId(const QString& deviceId);

            QString lastEvent() const;
            void setLastEvent( QString identifier );

            QByteArray generateTxnId() const;

            using nam_customizer_t =
                std::function<void(QNetworkAccessManager*)>;
            static void customizeNetworkAccess(nam_customizer_t customizer);

        private:
            struct Private;
            std::unique_ptr<Private> d;
    };
}  // namespace QMatrixClient
