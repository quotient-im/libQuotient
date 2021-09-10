// SPDX-FileCopyrightText: Tobias Fella <fella@posteo.de>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "mxcreply.h"

#include <QtCore/QBuffer>
#include <QtCore/QTimer>
#include "connection.h"
#include "room.h"
#include "networkaccessmanager.h"
#include "events/stickerevent.h"

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
    connect(d->m_reply, &QNetworkReply::finished, this, [this, eventId]() {
        setError(d->m_reply->error(), d->m_reply->errorString());
        setOpenMode(ReadOnly);
        Q_EMIT finished();
    });
}

MxcReply::MxcReply()
{
    QTimer::singleShot(0, this, [this](){
        setError(QNetworkReply::ProtocolInvalidOperationError, QStringLiteral("Invalid Request"));
        setFinished(true);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
        Q_EMIT errorOccurred(QNetworkReply::ProtocolInvalidOperationError);
#else
        Q_EMIT error(QNetworkReply::ProtocolInvalidOperationError);
#endif
        Q_EMIT finished();
    });
}

bool MxcReply::isSequential() const
{
    return true;
}

qint64 MxcReply::readData(char *data, qint64 maxSize)
{
    return d->m_reply->read(data, maxSize);
}

void MxcReply::abort()
{
    d->m_reply->abort();
}