// SPDX-FileCopyrightText: 2018 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "downloadfilejob.h"

#include <QtCore/QFile>
#include <QtCore/QTemporaryFile>
#include <QtNetwork/QNetworkReply>

#ifdef Quotient_E2EE_ENABLED
#   include <QCryptographicHash>
#   include <openssl/evp.h>

QByteArray decrypt(const QByteArray &ciphertext, const QByteArray &key, const QByteArray &iv)
{
    QByteArray plaintext(ciphertext.size(), 0);
    EVP_CIPHER_CTX *ctx;
    int length;
    ctx = EVP_CIPHER_CTX_new();
    EVP_DecryptInit_ex(ctx, EVP_aes_256_ctr(), NULL, (const unsigned char *)key.data(), (const unsigned char *)iv.data());
    EVP_DecryptUpdate(ctx, (unsigned char *)plaintext.data(), &length, (const unsigned char *)ciphertext.data(), ciphertext.size());
    EVP_DecryptFinal_ex(ctx, (unsigned char *)plaintext.data() + length, &length);
    EVP_CIPHER_CTX_free(ctx);
    return plaintext;
}
#endif

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

#ifdef Quotient_E2EE_ENABLED
    QByteArray key;
    QByteArray iv;
    QByteArray sha256;
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
    , d(localFilename.isEmpty() ? new Private : new Private(localFilename))
{
    setObjectName(QStringLiteral("DownloadFileJob"));
}

#ifdef Quotient_E2EE_ENABLED
DownloadFileJob::DownloadFileJob(const QString& serverName,
                                 const QString& mediaId,
                                 const QString& key,
                                 const QString& iv,
                                 const QString& sha256,
                                 const QString& localFilename)
    : GetContentJob(serverName, mediaId)
    , d(localFilename.isEmpty() ? new Private : new Private(localFilename))
{
    setObjectName(QStringLiteral("DownloadFileJob"));
    auto _key = key;
    d->key = QByteArray::fromBase64(_key.replace(QLatin1Char('_'), QLatin1Char('/')).replace(QLatin1Char('-'), QLatin1Char('+')).toLatin1());
    d->iv = QByteArray::fromBase64(iv.toLatin1());
    d->sha256 = QByteArray::fromBase64(sha256.toLatin1());
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
        setStatus(FileError, "Could not open the target file for writing");
        return;
    }
    if (!d->tempFile->isReadable() && !d->tempFile->open(QIODevice::ReadWrite)) {
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
#ifdef Quotient_E2EE_ENABLED
        if(d->key.size() != 0) {
            d->tempFile->seek(0);
            QByteArray encrypted = d->tempFile->readAll();
            if(d->sha256 != QCryptographicHash::hash(encrypted, QCryptographicHash::Sha256)) {
                qCWarning(E2EE) << "Hash verification failed for file";
                return IncorrectResponse;
            }
            auto decrypted = decrypt(encrypted, d->key, d->iv);
            d->targetFile->write(decrypted);
            d->targetFile->remove();
        } else {
#endif
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
#ifdef Quotient_E2EE_ENABLED
        }
#endif
    } else {
#ifdef Quotient_E2EE_ENABLED
        if(d->key.size() != 0) {
            d->tempFile->seek(0);
            auto encrypted = d->tempFile->readAll();

            if(d->sha256 != QCryptographicHash::hash(encrypted, QCryptographicHash::Sha256)) {
                qCWarning(E2EE) << "Hash verification failed for file";
                return IncorrectResponse;
            }
            auto decrypted = decrypt(encrypted, d->key, d->iv);
            d->tempFile->write(decrypted);
        } else {
#endif
            d->tempFile->close();
#ifdef Quotient_E2EE_ENABLED
        }
#endif
    }
    qCDebug(JOBS) << "Saved a file as" << targetFileName();
    return Success;
}
