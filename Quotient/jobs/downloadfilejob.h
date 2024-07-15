// SPDX-FileCopyrightText: 2018 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "basejob.h"

namespace Quotient {
struct EncryptedFileMetadata;

class QUOTIENT_API DownloadFileJob : public BaseJob {
public:
    static QUrl makeRequestUrl(const HomeserverData& hsData, const QUrl& mxcUri);
    static QUrl makeRequestUrl(const HomeserverData& hsData, const QString& serverName,
                               const QString& mediaId);

    DownloadFileJob(QString serverName, QString mediaId, const QString& localFilename = {});

    DownloadFileJob(QString serverName, QString mediaId, const EncryptedFileMetadata& file,
                    const QString& localFilename = {});
    QString targetFileName() const;

private:
    class Private;
    ImplPtr<Private> d;

    void doPrepare(const ConnectionData* connectionData) override;
    void onSentRequest(QNetworkReply* reply) override;
    void beforeAbandon() override;
    Status prepareResult() override;
};
} // namespace Quotient
