// SPDX-FileCopyrightText: 2015 Felix Rohrbach <kde@fxrh.de>
// SPDX-FileCopyrightText: 2016 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

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
    bool needsToken(const QString& requestName) const;
    QNetworkAccessManager* nam() const;

    void setBaseUrl(QUrl baseUrl);
    void setToken(QByteArray accessToken);
    void setDeviceId(const QString& deviceId);
    void setUserId(const QString& userId);
    void setNeedsToken(const QString& requestName);

    QString lastEvent() const;
    void setLastEvent(QString identifier);

    QByteArray generateTxnId() const;

private:
    class Private;
    std::unique_ptr<Private> d;
};
} // namespace Quotient
