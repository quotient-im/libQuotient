/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "presence.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

SetPresenceJob::SetPresenceJob(const QString& userId, const QString& presence, const QString& statusMsg)
    : BaseJob(HttpVerb::Put, "SetPresenceJob",
        basePath % "/presence/" % userId % "/status")
{
    QJsonObject _data;
    addToJson<>(_data, "presence", presence);
    addToJson<IfNotEmpty>(_data, "status_msg", statusMsg);
    setRequestData(_data);
}

class GetPresenceJob::Private
{
    public:
        QString presence;
        Omittable<int> lastActiveAgo;
        QString statusMsg;
        bool currentlyActive;
};

QUrl GetPresenceJob::makeRequestUrl(QUrl baseUrl, const QString& userId)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
            basePath % "/presence/" % userId % "/status");
}

GetPresenceJob::GetPresenceJob(const QString& userId)
    : BaseJob(HttpVerb::Get, "GetPresenceJob",
        basePath % "/presence/" % userId % "/status", false)
    , d(new Private)
{
}

GetPresenceJob::~GetPresenceJob() = default;

const QString& GetPresenceJob::presence() const
{
    return d->presence;
}

Omittable<int> GetPresenceJob::lastActiveAgo() const
{
    return d->lastActiveAgo;
}

const QString& GetPresenceJob::statusMsg() const
{
    return d->statusMsg;
}

bool GetPresenceJob::currentlyActive() const
{
    return d->currentlyActive;
}

BaseJob::Status GetPresenceJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    if (!json.contains("presence"))
        return { JsonParseError,
            "The key 'presence' not found in the response" };
    d->presence = fromJson<QString>(json.value("presence"));
    d->lastActiveAgo = fromJson<int>(json.value("last_active_ago"));
    d->statusMsg = fromJson<QString>(json.value("status_msg"));
    d->currentlyActive = fromJson<bool>(json.value("currently_active"));
    return Success;
}

ModifyPresenceListJob::ModifyPresenceListJob(const QString& userId, const QStringList& invite, const QStringList& drop)
    : BaseJob(HttpVerb::Post, "ModifyPresenceListJob",
        basePath % "/presence/list/" % userId)
{
    QJsonObject _data;
    addToJson<IfNotEmpty>(_data, "invite", invite);
    addToJson<IfNotEmpty>(_data, "drop", drop);
    setRequestData(_data);
}

class GetPresenceForListJob::Private
{
    public:
        Events data;
};

QUrl GetPresenceForListJob::makeRequestUrl(QUrl baseUrl, const QString& userId)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
            basePath % "/presence/list/" % userId);
}

GetPresenceForListJob::GetPresenceForListJob(const QString& userId)
    : BaseJob(HttpVerb::Get, "GetPresenceForListJob",
        basePath % "/presence/list/" % userId, false)
    , d(new Private)
{
}

GetPresenceForListJob::~GetPresenceForListJob() = default;

Events&& GetPresenceForListJob::data()
{
    return std::move(d->data);
}

BaseJob::Status GetPresenceForListJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    if (!json.contains("data"))
        return { JsonParseError,
            "The key 'data' not found in the response" };
    d->data = fromJson<Events>(json.value("data"));
    return Success;
}

