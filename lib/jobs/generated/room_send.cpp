/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "room_send.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

class SendMessageJob::Private
{
    public:
        QString eventId;
};

SendMessageJob::SendMessageJob(const QString& roomId, const QString& eventType, const QString& txnId, const QJsonObject& body)
    : BaseJob(HttpVerb::Put, "SendMessageJob",
        basePath % "/rooms/" % roomId % "/send/" % eventType % "/" % txnId)
    , d(new Private)
{
    setRequestData(Data(toJson(body)));
}

SendMessageJob::~SendMessageJob() = default;

const QString& SendMessageJob::eventId() const
{
    return d->eventId;
}

BaseJob::Status SendMessageJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    d->eventId = fromJson<QString>(json.value("event_id"));
    return Success;
}

