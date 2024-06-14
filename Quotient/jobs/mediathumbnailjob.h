// SPDX-FileCopyrightText: 2018 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <Quotient/csapi/content-repo.h>

#include <QtGui/QPixmap>

namespace Quotient {
class QUOTIENT_API MediaThumbnailJob : public GetContentThumbnailJob {
public:
    using GetContentThumbnailJob::makeRequestUrl;
    static QUrl makeRequestUrl(QUrl baseUrl, const QUrl& mxcUri,
                               QSize requestedSize);

    MediaThumbnailJob(const QString& serverName, const QString& mediaId,
                      QSize requestedSize);
    MediaThumbnailJob(const QUrl& mxcUri, QSize requestedSize);

    QImage thumbnail() const;
    [[deprecated("Use thumbnail().scaled() instead")]]
    QImage scaledThumbnail(QSize toSize) const;

protected:
    Status prepareResult() override;

private:
    QImage _thumbnail;
};

inline auto collectResponse(const MediaThumbnailJob* j) { return j->thumbnail(); }

} // namespace Quotient
