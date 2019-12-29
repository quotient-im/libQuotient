#pragma once

#include "csapi/content-repo.h"

namespace Quotient {
class DownloadFileJob : public GetContentJob {
public:
    using GetContentJob::makeRequestUrl;
    static QUrl makeRequestUrl(QUrl baseUrl, const QUrl& mxcUri);

    DownloadFileJob(const QString& serverName, const QString& mediaId,
                    const QString& localFilename = {});

    QString targetFileName() const;

private:
    class Private;
    QScopedPointer<Private> d;

    void doPrepare() override;
    void onSentRequest(QNetworkReply* reply) override;
    void beforeAbandon(QNetworkReply*) override;
    Status parseReply(QNetworkReply*) override;
};
} // namespace Quotient
