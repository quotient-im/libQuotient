/******************************************************************************
 * SPDX-FileCopyrightText: 2016 Felix Rohrbach <kde@fxrh.de>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include "csapi/content-repo.h"

#include <QtGui/QPixmap>

namespace Quotient {
class MediaThumbnailJob : public GetContentThumbnailJob {
public:
    using GetContentThumbnailJob::makeRequestUrl;
    static QUrl makeRequestUrl(QUrl baseUrl, const QUrl& mxcUri,
                               QSize requestedSize);

    MediaThumbnailJob(const QString& serverName, const QString& mediaId,
                      QSize requestedSize);
    MediaThumbnailJob(const QUrl& mxcUri, QSize requestedSize);

    QImage thumbnail() const;
    QImage scaledThumbnail(QSize toSize) const;

protected:
    Status prepareResult() override;

private:
    QImage _thumbnail;
};
} // namespace Quotient
