// SPDX-FileCopyrightText: Tobias Fella <fella@posteo.de>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "mxcreply.h"

#include <QtCore/QBuffer>

#ifdef Quotient_E2EE_ENABLED
#include "events/filesourceinfo.h"
#endif

using namespace Quotient;

class Q_DECL_HIDDEN MxcReply::Private
{
public:
    QNetworkReply* m_reply;
    QIODevice* m_device;
};

MxcReply::MxcReply(QNetworkReply* reply,
                   const EncryptedFileMetadata& fileMetadata)
    : d(makeImpl<Private>(reply, fileMetadata.isValid() ? nullptr : reply))
{
    reply->setParent(this);
    connect(d->m_reply, &QNetworkReply::finished, this, [this, fileMetadata] {
        setError(d->m_reply->error(), d->m_reply->errorString());

#ifdef Quotient_E2EE_ENABLED
        if (fileMetadata.isValid()) {
            auto buffer = new QBuffer(this);
            buffer->setData(decryptFile(d->m_reply->readAll(), fileMetadata));
            buffer->open(ReadOnly);
            d->m_device = buffer;
        }
#endif
        setOpenMode(ReadOnly);
        emit finished();
    });
}

MxcReply::MxcReply()
    : d(ZeroImpl<Private>())
{
    static const auto BadRequestPhrase = tr("Bad Request");
    QMetaObject::invokeMethod(this, [this]() {
            setAttribute(QNetworkRequest::HttpStatusCodeAttribute, 400);
            setAttribute(QNetworkRequest::HttpReasonPhraseAttribute,
                         BadRequestPhrase);
            setError(QNetworkReply::ProtocolInvalidOperationError,
                     BadRequestPhrase);
            setFinished(true);
            emit errorOccurred(QNetworkReply::ProtocolInvalidOperationError);
            emit finished();
        }, Qt::QueuedConnection);
}

qint64 MxcReply::readData(char *data, qint64 maxSize)
{
    if(d != nullptr && d->m_device != nullptr) {
        return d->m_device->read(data, maxSize);
    }
    return -1;
}

void MxcReply::abort()
{
    if(d != nullptr && d->m_reply != nullptr) {
        d->m_reply->abort();
    }
}

qint64 MxcReply::bytesAvailable() const
{
    if (d != nullptr && d->m_device != nullptr) {
        return d->m_device->bytesAvailable() + QNetworkReply::bytesAvailable();
    }
    return 0;
}
