// SPDX-FileCopyrightText: 2018 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "downloadfilejob.h"

#include "../logging_categories_p.h"

#include <QtCore/QFile>
#include <QtCore/QTemporaryFile>
#include <QtNetwork/QNetworkReply>

#ifdef Quotient_E2EE_ENABLED
    #include <Quotient/events/filesourceinfo.h>

    #include <QtCore/QCryptographicHash>
#endif

using namespace Quotient;
class Q_DECL_HIDDEN DownloadFileJob::Private {
public:
    Private() : tempFile(new QTemporaryFile()) {}

    explicit Private(const QString& localFilename)
        : targetFile(new QFile(localFilename))
        , tempFile(new QFile(targetFile->fileName() + ".qtntdownload"_ls))
    {}

    QScopedPointer<QFile> targetFile;
    QScopedPointer<QFile> tempFile;

#ifdef Quotient_E2EE_ENABLED
    Omittable<EncryptedFileMetadata> encryptedFileMetadata;
#endif
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
    , d(localFilename.isEmpty() ? makeImpl<Private>()
                                : makeImpl<Private>(localFilename))
{
    setObjectName(QStringLiteral("DownloadFileJob"));
}

#ifdef Quotient_E2EE_ENABLED
DownloadFileJob::DownloadFileJob(const QString& serverName,
                                 const QString& mediaId,
                                 const EncryptedFileMetadata& file,
                                 const QString& localFilename)
    : GetContentJob(serverName, mediaId)
    , d(localFilename.isEmpty() ? makeImpl<Private>()
                                : makeImpl<Private>(localFilename))
{
    setObjectName(QStringLiteral("DownloadFileJob"));
    d->encryptedFileMetadata = file;
}
#endif

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
        setStatus(FileError, "Could not open the target file for writing"_ls);
        return;
    }
    if (!d->tempFile->isReadable() && !d->tempFile->open(QIODevice::ReadWrite)) {
        qCWarning(JOBS) << "Couldn't open the temporary file"
                        << d->tempFile->fileName() << "for writing";
        setStatus(FileError, "Could not open the temporary download file"_ls);
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
                              "Could not reserve disk space for download"_ls);
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

#ifdef Quotient_E2EE_ENABLED
void decryptFile(QFile& sourceFile, const EncryptedFileMetadata& metadata,
                 QFile& targetFile)
{
    sourceFile.seek(0);
    const auto encrypted = sourceFile.readAll(); // TODO: stream decryption
    const auto decrypted = decryptFile(encrypted, metadata);
    targetFile.write(decrypted);
}
#endif

BaseJob::Status DownloadFileJob::prepareResult()
{
    if (d->targetFile) {
#ifdef Quotient_E2EE_ENABLED
        if (d->encryptedFileMetadata.has_value()) {
            decryptFile(*d->tempFile, *d->encryptedFileMetadata, *d->targetFile);
            d->tempFile->remove();
        } else {
#endif
            d->targetFile->close();
            if (!d->targetFile->remove()) {
                qWarning(JOBS) << "Failed to remove the target file placeholder";
                return { FileError, "Couldn't finalise the download"_ls };
            }
            if (!d->tempFile->rename(d->targetFile->fileName())) {
                qWarning(JOBS) << "Failed to rename" << d->tempFile->fileName()
                                << "to" << d->targetFile->fileName();
                return { FileError, "Couldn't finalise the download"_ls };
            }
#ifdef Quotient_E2EE_ENABLED
        }
#endif
    } else {
#ifdef Quotient_E2EE_ENABLED
        if (d->encryptedFileMetadata.has_value()) {
            QTemporaryFile tempTempFile; // Assuming it to be next to tempFile
            decryptFile(*d->tempFile, *d->encryptedFileMetadata, tempTempFile);
            d->tempFile->close();
            if (!d->tempFile->remove()) {
                qWarning(JOBS)
                    << "Failed to remove the decrypted file placeholder";
                return { FileError, "Couldn't finalise the download"_ls };
            }
            if (!tempTempFile.rename(d->tempFile->fileName())) {
                qWarning(JOBS) << "Failed to rename" << tempTempFile.fileName()
                                << "to" << d->tempFile->fileName();
                return { FileError, "Couldn't finalise the download"_ls };
            }
        } else {
#endif
            d->tempFile->close();
#ifdef Quotient_E2EE_ENABLED
        }
#endif
    }
    qDebug(JOBS) << "Saved a file as" << targetFileName();
    return Success;
}
