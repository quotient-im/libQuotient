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

static const auto RedactEventJobName = QStringLiteral("RedactEventJob");

RedactEventJob::RedactEventJob(const QString& roomId, const QString& eventId, const QString& txnId, const QString& reason)
    : BaseJob(HttpVerb::Put, RedactEventJobName,
        basePath % "/rooms/" % roomId % "/redact/" % eventId % "/" % txnId)
    , d(new Private)
{
    QJsonObject _data;
    addParam<IfNotEmpty>(_data, QStringLiteral("reason"), reason);
    setRequestData(_data);
}

RedactEventJob::~RedactEventJob() = default;

const QString& RedactEventJob::eventId() const
{
    return d->eventId;
}

BaseJob::Status RedactEventJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    d->eventId = fromJson<QString>(json.value("event_id"_ls));
    return Success;
}

