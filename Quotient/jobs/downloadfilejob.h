// SPDX-FileCopyrightText: 2018 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <Quotient/csapi/content-repo.h>

namespace Quotient {
struct EncryptedFileMetadata;

class QUOTIENT_API DownloadFileJob : public GetContentJob {
public:
    using GetContentJob::makeRequestUrl;
    static QUrl makeRequestUrl(QUrl baseUrl, const QUrl& mxcUri);

    DownloadFileJob(const QString& serverName, const QString& mediaId,
                    const QString& localFilename = {});

    DownloadFileJob(const QString& serverName, const QString& mediaId,
                    const EncryptedFileMetadata& file,
                    const QString& localFilename = {});
    QString targetFileName() const;

private:
    class Private;
    ImplPtr<Private> d;

    void doPrepare() override;
    void onSentRequest(QNetworkReply* reply) override;
    void beforeAbandon() override;
    Status prepareResult() override;
};
} // namespace Quotient
