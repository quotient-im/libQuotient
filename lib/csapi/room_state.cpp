/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "room_state.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

class SetRoomStateWithKeyJob::Private
{
    public:
        QString eventId;
};

SetRoomStateWithKeyJob::SetRoomStateWithKeyJob(const QString& roomId, const QString& eventType, const QString& stateKey, const QJsonObject& body)
    : BaseJob(HttpVerb::Put, "SetRoomStateWithKeyJob",
        basePath % "/rooms/" % roomId % "/state/" % eventType % "/" % stateKey)
    , d(new Private)
{
    setRequestData(Data(toJson(body)));
}

SetRoomStateWithKeyJob::~SetRoomStateWithKeyJob() = default;

const QString& SetRoomStateWithKeyJob::eventId() const
{
    return d->eventId;
}

BaseJob::Status SetRoomStateWithKeyJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    d->eventId = fromJson<QString>(json.value("event_id"));
    return Success;
}

class SetRoomStateJob::Private
{
    public:
        QString eventId;
};

SetRoomStateJob::SetRoomStateJob(const QString& roomId, const QString& eventType, const QJsonObject& body)
    : BaseJob(HttpVerb::Put, "SetRoomStateJob",
        basePath % "/rooms/" % roomId % "/state/" % eventType)
    , d(new Private)
{
    setRequestData(Data(toJson(body)));
}

SetRoomStateJob::~SetRoomStateJob() = default;

const QString& SetRoomStateJob::eventId() const
{
    return d->eventId;
}

BaseJob::Status SetRoomStateJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    d->eventId = fromJson<QString>(json.value("event_id"));
    return Success;
}

