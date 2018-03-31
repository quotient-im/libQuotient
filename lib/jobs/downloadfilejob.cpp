#include "downloadfilejob.h"

#include <QtNetwork/QNetworkReply>
#include <QtCore/QFile>
#include <QtCore/QTemporaryFile>

using namespace QMatrixClient;

class DownloadFileJob::Private
{
    public:
        Private() : tempFile(new QTemporaryFile()) { }

        explicit Private(const QString& localFilename)
            : targetFile(new QFile(localFilename))
            , tempFile(new QFile(targetFile->fileName() + ".qmcdownload"))
        { }

        QScopedPointer<QFile> targetFile;
        QScopedPointer<QFile> tempFile;
};

QUrl DownloadFileJob::makeRequestUrl(QUrl baseUrl, const QUrl& mxcUri)
{
    return makeRequestUrl(baseUrl, mxcUri.authority(), mxcUri.path().mid(1));
}

DownloadFileJob::DownloadFileJob(const QString& serverName,
                                 const QString& mediaId,
                                 const QString& localFilename)
    : GetContentJob(serverName, mediaId)
    , d(localFilename.isEmpty() ? new Private : new Private(localFilename))
{
    setObjectName("DownloadFileJob");
}

QString DownloadFileJob::targetFileName() const
{
    return (d->targetFile ? d->targetFile : d->tempFile)->fileName();
}

void DownloadFileJob::beforeStart(const ConnectionData*)
{
    if (d->targetFile && !d->targetFile->isReadable() &&
            !d->targetFile->open(QIODevice::WriteOnly))
    {
        qCWarning(JOBS) << "Couldn't open the file"
                        << d->targetFile->fileName() << "for writing";
        setStatus(FileError, "Could not open the target file for writing");
        return;
    }
    if (!d->tempFile->isReadable() && !d->tempFile->open(QIODevice::WriteOnly))
    {
        qCWarning(JOBS) << "Couldn't open the temporary file"
                        << d->tempFile->fileName() << "for writing";
        setStatus(FileError, "Could not open the temporary download file");
        return;
    }
    qCDebug(JOBS) << "Downloading to" << d->tempFile->fileName();
}

void DownloadFileJob::afterStart(const ConnectionData*, QNetworkReply* reply)
{
    connect(reply, &QNetworkReply::metaDataChanged, this, [this,reply] {
        auto sizeHeader = reply->header(QNetworkRequest::ContentLengthHeader);
        if (sizeHeader.isValid())
        {
            auto targetSize = sizeHeader.value<qint64>();
            if (targetSize != -1)
                if (!d->tempFile->resize(targetSize))
                {
                    qCWarning(JOBS) << "Failed to allocate" << targetSize
                                    << "bytes for" << d->tempFile->fileName();
                    setStatus(FileError,
                              "Could not reserve disk space for download");
                }
        }
    });
    connect(reply, &QIODevice::readyRead, this, [this,reply] {
            auto bytes = reply->read(reply->bytesAvailable());
            if (bytes.isEmpty())
            {
                qCWarning(JOBS)
                        << "Unexpected empty chunk when downloading from"
                        << reply->url() << "to" << d->tempFile->fileName();
            } else {
                d->tempFile->write(bytes);
            }
    });
}

void DownloadFileJob::beforeAbandon(QNetworkReply*)
{
    if (d->targetFile)
        d->targetFile->remove();
    d->tempFile->remove();
}

BaseJob::Status DownloadFileJob::parseReply(QNetworkReply*)
{
    if (d->targetFile)
    {
        d->targetFile->close();
        if (!d->targetFile->remove())
        {
            qCWarning(JOBS) << "Failed to remove the target file placeholder";
            return { FileError, "Couldn't finalise the download" };
        }
        if (!d->tempFile->rename(d->targetFile->fileName()))
        {
            qCWarning(JOBS) << "Failed to rename" << d->tempFile->fileName()
                            << "to" << d->targetFile->fileName();
            return { FileError, "Couldn't finalise the download" };
        }
    }
    else
        d->tempFile->close();
    qCDebug(JOBS) << "Saved a file as" << targetFileName();
    return Success;
}
