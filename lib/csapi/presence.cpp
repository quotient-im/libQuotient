/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "presence.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

static const auto SetPresenceJobName = QStringLiteral("SetPresenceJob");

SetPresenceJob::SetPresenceJob(const QString& userId, const QString& presence, const QString& statusMsg)
    : BaseJob(HttpVerb::Put, SetPresenceJobName,
        basePath % "/presence/" % userId % "/status")
{
    QJsonObject _data;
    addParam<>(_data, QStringLiteral("presence"), presence);
    addParam<IfNotEmpty>(_data, QStringLiteral("status_msg"), statusMsg);
    setRequestData(_data);
}

class GetPresenceJob::Private
{
    public:
        QString presence;
        Omittable<int> lastActiveAgo;
        QString statusMsg;
        Omittable<bool> currentlyActive;
};

QUrl GetPresenceJob::makeRequestUrl(QUrl baseUrl, const QString& userId)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
            basePath % "/presence/" % userId % "/status");
}

static const auto GetPresenceJobName = QStringLiteral("GetPresenceJob");

GetPresenceJob::GetPresenceJob(const QString& userId)
    : BaseJob(HttpVerb::Get, GetPresenceJobName,
        basePath % "/presence/" % userId % "/status")
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

Omittable<bool> GetPresenceJob::currentlyActive() const
{
    return d->currentlyActive;
}

BaseJob::Status GetPresenceJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    if (!json.contains("presence"_ls))
        return { IncorrectResponse,
            "The key 'presence' not found in the response" };
    fromJson(json.value("presence"_ls), d->presence);
    fromJson(json.value("last_active_ago"_ls), d->lastActiveAgo);
    fromJson(json.value("status_msg"_ls), d->statusMsg);
    fromJson(json.value("currently_active"_ls), d->currentlyActive);
    return Success;
}

