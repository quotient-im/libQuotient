// SPDX-FileCopyrightText: 2018 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "downloadfilejob.h"

#include "Quotient/connectiondata.h"

#include "../csapi/authed-content-repo.h"
#include "../csapi/content-repo.h"

#include <Quotient/events/filesourceinfo.h>

#include "../logging_categories_p.h"

#include <QtCore/QCryptographicHash>
#include <QtCore/QFile>
#include <QtCore/QTemporaryFile>
#include <QtNetwork/QNetworkReply>

using namespace Quotient;
class Q_DECL_HIDDEN DownloadFileJob::Private {
public:
    explicit Private(QString serverName, QString mediaId, const QString& localFilename)
        : serverName(std::move(serverName))
        , mediaId(std::move(mediaId))
        , targetFile(!localFilename.isEmpty() ? new QFile(localFilename) : nullptr)
        , tempFile(!localFilename.isEmpty() ? new QFile(targetFile->fileName() + ".qtntdownload"_L1)
                                            : new QTemporaryFile())
    {}

    QString serverName;
    QString mediaId;
    QScopedPointer<QFile> targetFile;
    QScopedPointer<QFile> tempFile;

    std::optional<EncryptedFileMetadata> encryptedFileMetadata;
};

QUrl DownloadFileJob::makeRequestUrl(const HomeserverData& hsData, const QUrl& mxcUri)
{
    return makeRequestUrl(hsData, mxcUri.authority(), mxcUri.path().mid(1));
}

QUrl DownloadFileJob::makeRequestUrl(const HomeserverData& hsData, const QString& serverName,
                                     const QString& mediaId)
{
    QT_IGNORE_DEPRECATIONS( // For GetContentJob
        return hsData.checkMatrixSpecVersion(u"v1.11")
                   ? GetContentAuthedJob::makeRequestUrl(hsData, serverName, mediaId)
                   : GetContentJob::makeRequestUrl(hsData, serverName, mediaId);)
}

DownloadFileJob::DownloadFileJob(QString serverName, QString mediaId, const QString& localFilename)
    : BaseJob(HttpVerb::Get, u"DownloadFileJob"_s, {})
    , d(makeImpl<Private>(std::move(serverName), std::move(mediaId), localFilename))
{}

DownloadFileJob::DownloadFileJob(QString serverName, QString mediaId,
                                 const EncryptedFileMetadata& file, const QString& localFilename)
    : DownloadFileJob(std::move(serverName), std::move(mediaId), localFilename)
{
    d->encryptedFileMetadata = file;
}

QString DownloadFileJob::targetFileName() const
{
    return (d->targetFile ? d->targetFile : d->tempFile)->fileName();
}

void DownloadFileJob::doPrepare(const ConnectionData* connectionData)
{
    const auto url = makeRequestUrl(connectionData->homeserverData(), d->serverName, d->mediaId);
    setApiEndpoint(url.toEncoded(QUrl::RemoveQuery | QUrl::RemoveFragment | QUrl::FullyEncoded));
    setRequestQuery(QUrlQuery{ url.query() });

    if (d->targetFile && !d->targetFile->isReadable()
        && !d->targetFile->open(QIODevice::WriteOnly)) {
        qCWarning(JOBS) << "Couldn't open the file" << d->targetFile->fileName()
                        << "for writing";
        setStatus(FileError, "Could not open the target file for writing"_L1);
        return;
    }
    if (!d->tempFile->isReadable() && !d->tempFile->open(QIODevice::ReadWrite)) {
        qCWarning(JOBS) << "Couldn't open the temporary file"
                        << d->tempFile->fileName() << "for writing";
        setStatus(FileError, "Could not open the temporary download file"_L1);
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
                              "Could not reserve disk space for download"_L1);
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

void decryptFile(QFile& sourceFile, const EncryptedFileMetadata& metadata,
                 QFile& targetFile)
{
    sourceFile.seek(0);
    const auto encrypted = sourceFile.readAll(); // TODO: stream decryption
    const auto decrypted = decryptFile(encrypted, metadata);
    targetFile.write(decrypted);
}

BaseJob::Status DownloadFileJob::prepareResult()
{
    if (d->targetFile) {
        if (d->encryptedFileMetadata.has_value()) {
            decryptFile(*d->tempFile, *d->encryptedFileMetadata, *d->targetFile);
            d->tempFile->remove();
        } else {
            d->targetFile->close();
            if (!d->targetFile->remove()) {
                qWarning(JOBS) << "Failed to remove the target file placeholder";
                return { FileError, "Couldn't finalise the download"_L1 };
            }
            if (!d->tempFile->rename(d->targetFile->fileName())) {
                qWarning(JOBS) << "Failed to rename" << d->tempFile->fileName()
                                << "to" << d->targetFile->fileName();
                return { FileError, "Couldn't finalise the download"_L1 };
            }
        }
    } else {
        if (d->encryptedFileMetadata.has_value()) {
            QTemporaryFile tempTempFile; // Assuming it to be next to tempFile
            decryptFile(*d->tempFile, *d->encryptedFileMetadata, tempTempFile);
            d->tempFile->close();
            if (!d->tempFile->remove()) {
                qWarning(JOBS)
                    << "Failed to remove the decrypted file placeholder";
                return { FileError, "Couldn't finalise the download"_L1 };
            }
            if (!tempTempFile.rename(d->tempFile->fileName())) {
                qWarning(JOBS) << "Failed to rename" << tempTempFile.fileName()
                                << "to" << d->tempFile->fileName();
                return { FileError, "Couldn't finalise the download"_L1 };
            }
        } else {
            d->tempFile->close();
        }
    }
    qDebug(JOBS) << "Saved a file as" << targetFileName();
    return Success;
}
