// SPDX-FileCopyrightText: 2018 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "basejob.h"

#include <QtGui/QPixmap>

namespace Quotient {
class QUOTIENT_API MediaThumbnailJob : public BaseJob {
public:
    static QUrl makeRequestUrl(const HomeserverData& hsData, const QUrl& mxcUri,
                               QSize requestedSize, std::optional<bool> animated = std::nullopt);
    static QUrl makeRequestUrl(const HomeserverData& hsData, const QString& serverName,
                               const QString& mediaId, QSize requestedSize,
                               std::optional<bool> animated = std::nullopt);

    MediaThumbnailJob(QString serverName, QString mediaId, QSize requestedSize,
                      std::optional<bool> animated = std::nullopt);
    MediaThumbnailJob(const QUrl& mxcUri, QSize requestedSize,
                      std::optional<bool> animated = std::nullopt);

    QImage thumbnail() const;
    [[deprecated("Use thumbnail().scaled() instead")]]
    QImage scaledThumbnail(QSize toSize) const;

private:
    QString serverName;
    QString mediaId;
    QSize requestedSize;
    std::optional<bool> animated;
    QImage _thumbnail;

    void doPrepare(const ConnectionData* connectionData) override;
    Status prepareResult() override;
};

inline auto collectResponse(const MediaThumbnailJob* j) { return j->thumbnail(); }

} // namespace Quotient
