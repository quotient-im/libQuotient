/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "profile.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

SetDisplayNameJob::SetDisplayNameJob(const QString& userId, const QString& displayname)
    : BaseJob(HttpVerb::Put, "SetDisplayNameJob",
        basePath % "/profile/" % userId % "/displayname")
{
    QJsonObject _data;
    addToJson<IfNotEmpty>(_data, "displayname", displayname);
    setRequestData(_data);
}

class GetDisplayNameJob::Private
{
    public:
        QString displayname;
};

QUrl GetDisplayNameJob::makeRequestUrl(QUrl baseUrl, const QString& userId)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
            basePath % "/profile/" % userId % "/displayname");
}

GetDisplayNameJob::GetDisplayNameJob(const QString& userId)
    : BaseJob(HttpVerb::Get, "GetDisplayNameJob",
        basePath % "/profile/" % userId % "/displayname", false)
    , d(new Private)
{
}

GetDisplayNameJob::~GetDisplayNameJob() = default;

const QString& GetDisplayNameJob::displayname() const
{
    return d->displayname;
}

BaseJob::Status GetDisplayNameJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    d->displayname = fromJson<QString>(json.value("displayname"));
    return Success;
}

SetAvatarUrlJob::SetAvatarUrlJob(const QString& userId, const QString& avatarUrl)
    : BaseJob(HttpVerb::Put, "SetAvatarUrlJob",
        basePath % "/profile/" % userId % "/avatar_url")
{
    QJsonObject _data;
    addToJson<IfNotEmpty>(_data, "avatar_url", avatarUrl);
    setRequestData(_data);
}

class GetAvatarUrlJob::Private
{
    public:
        QString avatarUrl;
};

QUrl GetAvatarUrlJob::makeRequestUrl(QUrl baseUrl, const QString& userId)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
            basePath % "/profile/" % userId % "/avatar_url");
}

GetAvatarUrlJob::GetAvatarUrlJob(const QString& userId)
    : BaseJob(HttpVerb::Get, "GetAvatarUrlJob",
        basePath % "/profile/" % userId % "/avatar_url", false)
    , d(new Private)
{
}

GetAvatarUrlJob::~GetAvatarUrlJob() = default;

const QString& GetAvatarUrlJob::avatarUrl() const
{
    return d->avatarUrl;
}

BaseJob::Status GetAvatarUrlJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    d->avatarUrl = fromJson<QString>(json.value("avatar_url"));
    return Success;
}

class GetUserProfileJob::Private
{
    public:
        QString avatarUrl;
        QString displayname;
};

QUrl GetUserProfileJob::makeRequestUrl(QUrl baseUrl, const QString& userId)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
            basePath % "/profile/" % userId);
}

GetUserProfileJob::GetUserProfileJob(const QString& userId)
    : BaseJob(HttpVerb::Get, "GetUserProfileJob",
        basePath % "/profile/" % userId, false)
    , d(new Private)
{
}

GetUserProfileJob::~GetUserProfileJob() = default;

const QString& GetUserProfileJob::avatarUrl() const
{
    return d->avatarUrl;
}

const QString& GetUserProfileJob::displayname() const
{
    return d->displayname;
}

BaseJob::Status GetUserProfileJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    d->avatarUrl = fromJson<QString>(json.value("avatar_url"));
    d->displayname = fromJson<QString>(json.value("displayname"));
    return Success;
}

