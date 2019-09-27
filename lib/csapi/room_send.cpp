/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "room_send.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace Quotient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

class SendMessageJob::Private
{
public:
    QString eventId;
};

static const auto SendMessageJobName = QStringLiteral("SendMessageJob");

SendMessageJob::SendMessageJob(const QString& roomId, const QString& eventType,
                               const QString& txnId, const QJsonObject& body)
    : BaseJob(HttpVerb::Put, SendMessageJobName,
              basePath % "/rooms/" % roomId % "/send/" % eventType % "/" % txnId)
    , d(new Private)
{
    setRequestData(Data(toJson(body)));
}

SendMessageJob::~SendMessageJob() = default;

const QString& SendMessageJob::eventId() const { return d->eventId; }

BaseJob::Status SendMessageJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    fromJson(json.value("event_id"_ls), d->eventId);

    return Success;
}
