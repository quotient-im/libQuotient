// SPDX-FileCopyrightText: 2018 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "csapi/content-repo.h"
#include "events/encryptedfile.h"

namespace Quotient {
class DownloadFileJob : public GetContentJob {
public:
    using GetContentJob::makeRequestUrl;
    static QUrl makeRequestUrl(QUrl baseUrl, const QUrl& mxcUri);

    DownloadFileJob(const QString& serverName, const QString& mediaId,
                    const QString& localFilename = {});

#ifdef Quotient_E2EE_ENABLED
    DownloadFileJob(const QString& serverName, const QString& mediaId, const EncryptedFile& file, const QString& localFilename = {});
#endif
    QString targetFileName() const;

private:
    class Private;
    QScopedPointer<Private> d;

    void doPrepare() override;
    void onSentRequest(QNetworkReply* reply) override;
    void beforeAbandon() override;
    Status prepareResult() override;
};
} // namespace Quotient
