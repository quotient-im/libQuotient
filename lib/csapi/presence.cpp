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
        bool currentlyActive;
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

bool GetPresenceJob::currentlyActive() const
{
    return d->currentlyActive;
}

BaseJob::Status GetPresenceJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    if (!json.contains("presence"_ls))
        return { JsonParseError,
            "The key 'presence' not found in the response" };
    d->presence = fromJson<QString>(json.value("presence"_ls));
    d->lastActiveAgo = fromJson<int>(json.value("last_active_ago"_ls));
    d->statusMsg = fromJson<QString>(json.value("status_msg"_ls));
    d->currentlyActive = fromJson<bool>(json.value("currently_active"_ls));
    return Success;
}

static const auto ModifyPresenceListJobName = QStringLiteral("ModifyPresenceListJob");

ModifyPresenceListJob::ModifyPresenceListJob(const QString& userId, const QStringList& invite, const QStringList& drop)
    : BaseJob(HttpVerb::Post, ModifyPresenceListJobName,
        basePath % "/presence/list/" % userId)
{
    QJsonObject _data;
    addParam<IfNotEmpty>(_data, QStringLiteral("invite"), invite);
    addParam<IfNotEmpty>(_data, QStringLiteral("drop"), drop);
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

static const auto GetPresenceForListJobName = QStringLiteral("GetPresenceForListJob");

GetPresenceForListJob::GetPresenceForListJob(const QString& userId)
    : BaseJob(HttpVerb::Get, GetPresenceForListJobName,
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
    if (!json.contains("data"_ls))
        return { JsonParseError,
            "The key 'data' not found in the response" };
    d->data = fromJson<Events>(json.value("data"_ls));
    return Success;
}

