// SPDX-FileCopyrightText: 2018 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "downloadfilejob.h"

#include <QtCore/QFile>
#include <QtCore/QTemporaryFile>
#include <QtNetwork/QNetworkReply>

using namespace Quotient;

class DownloadFileJob::Private {
public:
    Private() : tempFile(new QTemporaryFile()) {}

    explicit Private(const QString& localFilename)
        : targetFile(new QFile(localFilename))
        , tempFile(new QFile(targetFile->fileName() + ".qtntdownload"))
    {}

    QScopedPointer<QFile> targetFile;
    QScopedPointer<QFile> tempFile;
};

QUrl DownloadFileJob::makeRequestUrl(QUrl baseUrl, const QUrl& mxcUri)
{
    return makeRequestUrl(std::move(baseUrl), mxcUri.authority(),
                          mxcUri.path().mid(1));
}

DownloadFileJob::DownloadFileJob(const QString& serverName,
                                 const QString& mediaId,
                                 const QString& localFilename)
    : GetContentJob(serverName, mediaId)
    , d(localFilename.isEmpty() ? new Private : new Private(localFilename))
{
    setObjectName(QStringLiteral("DownloadFileJob"));
}

DownloadFileJob::~DownloadFileJob() = default;

QString DownloadFileJob::targetFileName() const
{
    return (d->targetFile ? d->targetFile : d->tempFile)->fileName();
}

void DownloadFileJob::doPrepare()
{
    if (d->targetFile && !d->targetFile->isReadable()
        && !d->targetFile->open(QIODevice::WriteOnly)) {
        qCWarning(JOBS) << "Couldn't open the file" << d->targetFile->fileName()
                        << "for writing";
        setStatus(FileError, "Could not open the target file for writing");
        return;
    }
    if (!d->tempFile->isReadable() && !d->tempFile->open(QIODevice::WriteOnly)) {
        qCWarning(JOBS) << "Couldn't open the temporary file"
                        << d->tempFile->fileName() << "for writing";
        setStatus(FileError, "Could not open the temporary download file");
        return;
    }
    qCDebug(JOBS) << "Downloading to" << d->tempFile->fileName();
}

void DownloadFileJob::onSentRequest(QNetworkReply* reply)
{
    connect(reply, &QNetworkReply::metaDataChanged, this, [this, reply] {
        if (!status().good())
            return;
        auto sizeHeader = reply->header(QNetworkRequest::ContentLengthHeader);
        if (sizeHeader.isValid()) {
            auto targetSize = sizeHeader.toLongLong();
            if (targetSize != -1)
                if (!d->tempFile->resize(targetSize)) {
                    qCWarning(JOBS) << "Failed to allocate" << targetSize
                                    << "bytes for" << d->tempFile->fileName();
                    setStatus(FileError,
                              "Could not reserve disk space for download");
                }
        }
    });
    connect(reply, &QIODevice::readyRead, this, [this, reply] {
        if (!status().good())
            return;
        auto bytes = reply->read(reply->bytesAvailable());
        if (!bytes.isEmpty())
            d->tempFile->write(bytes);
        else
            qCWarning(JOBS) << "Unexpected empty chunk when downloading from"
                            << reply->url() << "to" << d->tempFile->fileName();
    });
}

void DownloadFileJob::beforeAbandon()
{
    if (d->targetFile)
        d->targetFile->remove();
    d->tempFile->remove();
}

BaseJob::Status DownloadFileJob::prepareResult()
{
    if (d->targetFile) {
        d->targetFile->close();
        if (!d->targetFile->remove()) {
            qCWarning(JOBS) << "Failed to remove the target file placeholder";
            return { FileError, "Couldn't finalise the download" };
        }
        if (!d->tempFile->rename(d->targetFile->fileName())) {
            qCWarning(JOBS) << "Failed to rename" << d->tempFile->fileName()
                            << "to" << d->targetFile->fileName();
            return { FileError, "Couldn't finalise the download" };
        }
    } else
        d->tempFile->close();
    qCDebug(JOBS) << "Saved a file as" << targetFileName();
    return Success;
}
