/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "profile.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

static const auto SetDisplayNameJobName = QStringLiteral("SetDisplayNameJob");

SetDisplayNameJob::SetDisplayNameJob(const QString& userId,
                                     const QString& displayname)
    : BaseJob(HttpVerb::Put, SetDisplayNameJobName,
              basePath % "/profile/" % userId % "/displayname")
{
    QJsonObject _data;
    addParam<IfNotEmpty>(_data, QStringLiteral("displayname"), displayname);
    setRequestData(_data);
}

class GetDisplayNameJob::Private
{
public:
    QString displayname;
};

QUrl GetDisplayNameJob::makeRequestUrl(QUrl baseUrl, const QString& userId)
{
    return BaseJob::makeRequestUrl(
        std::move(baseUrl), basePath % "/profile/" % userId % "/displayname");
}

static const auto GetDisplayNameJobName = QStringLiteral("GetDisplayNameJob");

GetDisplayNameJob::GetDisplayNameJob(const QString& userId)
    : BaseJob(HttpVerb::Get, GetDisplayNameJobName,
              basePath % "/profile/" % userId % "/displayname", false)
    , d(new Private)
{}

GetDisplayNameJob::~GetDisplayNameJob() = default;

const QString& GetDisplayNameJob::displayname() const { return d->displayname; }

BaseJob::Status GetDisplayNameJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    fromJson(json.value("displayname"_ls), d->displayname);

    return Success;
}

static const auto SetAvatarUrlJobName = QStringLiteral("SetAvatarUrlJob");

SetAvatarUrlJob::SetAvatarUrlJob(const QString& userId, const QString& avatarUrl)
    : BaseJob(HttpVerb::Put, SetAvatarUrlJobName,
              basePath % "/profile/" % userId % "/avatar_url")
{
    QJsonObject _data;
    addParam<IfNotEmpty>(_data, QStringLiteral("avatar_url"), avatarUrl);
    setRequestData(_data);
}

class GetAvatarUrlJob::Private
{
public:
    QString avatarUrl;
};

QUrl GetAvatarUrlJob::makeRequestUrl(QUrl baseUrl, const QString& userId)
{
    return BaseJob::makeRequestUrl(
        std::move(baseUrl), basePath % "/profile/" % userId % "/avatar_url");
}

static const auto GetAvatarUrlJobName = QStringLiteral("GetAvatarUrlJob");

GetAvatarUrlJob::GetAvatarUrlJob(const QString& userId)
    : BaseJob(HttpVerb::Get, GetAvatarUrlJobName,
              basePath % "/profile/" % userId % "/avatar_url", false)
    , d(new Private)
{}

GetAvatarUrlJob::~GetAvatarUrlJob() = default;

const QString& GetAvatarUrlJob::avatarUrl() const { return d->avatarUrl; }

BaseJob::Status GetAvatarUrlJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    fromJson(json.value("avatar_url"_ls), d->avatarUrl);

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

static const auto GetUserProfileJobName = QStringLiteral("GetUserProfileJob");

GetUserProfileJob::GetUserProfileJob(const QString& userId)
    : BaseJob(HttpVerb::Get, GetUserProfileJobName,
              basePath % "/profile/" % userId, false)
    , d(new Private)
{}

GetUserProfileJob::~GetUserProfileJob() = default;

const QString& GetUserProfileJob::avatarUrl() const { return d->avatarUrl; }

const QString& GetUserProfileJob::displayname() const { return d->displayname; }

BaseJob::Status GetUserProfileJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    fromJson(json.value("avatar_url"_ls), d->avatarUrl);
    fromJson(json.value("displayname"_ls), d->displayname);

    return Success;
}
