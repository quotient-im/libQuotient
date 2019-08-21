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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

#pragma once

#include <QtCore/QUrl>

#include <memory>
#include <chrono>

class QNetworkAccessManager;

namespace Quotient {
class BaseJob;

class ConnectionData {
public:
    explicit ConnectionData(QUrl baseUrl);
    virtual ~ConnectionData();

    void submit(BaseJob* job);
    void limitRate(std::chrono::milliseconds nextCallAfter);

    QByteArray accessToken() const;
    QUrl baseUrl() const;
    const QString& deviceId() const;
    const QString& userId() const;
    QNetworkAccessManager* nam() const;

    void setBaseUrl(QUrl baseUrl);
    void setToken(QByteArray accessToken);
    [[deprecated("Use setBaseUrl() instead")]]
    void setHost(QString host);
    [[deprecated("Use setBaseUrl() instead")]]
    void setPort(int port);
    void setDeviceId(const QString& deviceId);
    void setUserId(const QString& userId);

    QString lastEvent() const;
    void setLastEvent(QString identifier);

    QByteArray generateTxnId() const;

private:
    class Private;
    std::unique_ptr<Private> d;
};
} // namespace Quotient
