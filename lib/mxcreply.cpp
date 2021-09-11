// SPDX-FileCopyrightText: Tobias Fella <fella@posteo.de>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "mxcreply.h"

#include "room.h"

using namespace Quotient;

class MxcReply::Private
{
public:
    QNetworkReply *m_reply = nullptr;
};

MxcReply::MxcReply(QNetworkReply* reply)
{
    reply->setParent(this);
    d->m_reply = reply;
    connect(d->m_reply, &QNetworkReply::finished, this, [this]() {
        setError(d->m_reply->error(), d->m_reply->errorString());
        setOpenMode(ReadOnly);
        Q_EMIT finished();
    });
}

MxcReply::MxcReply(QNetworkReply* reply, Room* room, const QString &eventId)
    : d(std::make_unique<Private>())
{
    reply->setParent(this);
    d->m_reply = reply;
    connect(d->m_reply, &QNetworkReply::finished, this, [this, room, eventId]() {
        setError(d->m_reply->error(), d->m_reply->errorString());
        setOpenMode(ReadOnly);
        emit finished();
    });
}

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
#define ERROR_SIGNAL errorOccurred
#else
#define ERROR_SIGNAL error
#endif

MxcReply::MxcReply()
{
    static const auto BadRequestPhrase = tr("Bad Request");
    QMetaObject::invokeMethod(this, [this]() {
            setAttribute(QNetworkRequest::HttpStatusCodeAttribute, 400);
            setAttribute(QNetworkRequest::HttpReasonPhraseAttribute,
                         BadRequestPhrase);
            setError(QNetworkReply::ProtocolInvalidOperationError,
                     BadRequestPhrase);
            setFinished(true);
            emit ERROR_SIGNAL(QNetworkReply::ProtocolInvalidOperationError);
            emit finished();
        }, Qt::QueuedConnection);
}

qint64 MxcReply::readData(char *data, qint64 maxSize)
{
    return d->m_reply->read(data, maxSize);
}

void MxcReply::abort()
{
    d->m_reply->abort();
}
