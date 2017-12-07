/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */


#include "redaction.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

class RedactEventJob::Private
{
    public:
        QString eventId;
};

RedactEventJob::RedactEventJob(const QString& roomId, const QString& eventId, const QString& txnId, const QString& reason)
    : BaseJob(HttpVerb::Put, "RedactEventJob",
        basePath % "/rooms/" % roomId % "/redact/" % eventId % "/" % txnId,
        Query { }
    ), d(new Private)
{
    QJsonObject _data;
    if (!reason.isEmpty())
        _data.insert("reason", toJson(reason));
    setRequestData(_data);
}

RedactEventJob::~RedactEventJob()
{
    delete d;
}

const QString& RedactEventJob::eventId() const
{
    return d->eventId;
}

BaseJob::Status RedactEventJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    d->eventId = fromJson<QString>(json.value("event_id"));
    return Success;
}

