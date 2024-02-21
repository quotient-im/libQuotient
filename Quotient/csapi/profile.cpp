// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "profile.h"

using namespace Quotient;

SetDisplayNameJob::SetDisplayNameJob(const QString& userId, const QString& displayname)
    : BaseJob(HttpVerb::Put, QStringLiteral("SetDisplayNameJob"),
              makePath("/_matrix/client/v3", "/profile/", userId, "/displayname"))
{
    QJsonObject _dataJson;
    addParam<>(_dataJson, QStringLiteral("displayname"), displayname);
    setRequestData({ _dataJson });
}

QUrl GetDisplayNameJob::makeRequestUrl(QUrl baseUrl, const QString& userId)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl), makePath("/_matrix/client/v3", "/profile/",
                                                                userId, "/displayname"));
}

GetDisplayNameJob::GetDisplayNameJob(const QString& userId)
    : BaseJob(HttpVerb::Get, QStringLiteral("GetDisplayNameJob"),
              makePath("/_matrix/client/v3", "/profile/", userId, "/displayname"), false)
{}

SetAvatarUrlJob::SetAvatarUrlJob(const QString& userId, const QUrl& avatarUrl)
    : BaseJob(HttpVerb::Put, QStringLiteral("SetAvatarUrlJob"),
              makePath("/_matrix/client/v3", "/profile/", userId, "/avatar_url"))
{
    QJsonObject _dataJson;
    addParam<>(_dataJson, QStringLiteral("avatar_url"), avatarUrl);
    setRequestData({ _dataJson });
}

QUrl GetAvatarUrlJob::makeRequestUrl(QUrl baseUrl, const QString& userId)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl), makePath("/_matrix/client/v3", "/profile/",
                                                                userId, "/avatar_url"));
}

GetAvatarUrlJob::GetAvatarUrlJob(const QString& userId)
    : BaseJob(HttpVerb::Get, QStringLiteral("GetAvatarUrlJob"),
              makePath("/_matrix/client/v3", "/profile/", userId, "/avatar_url"), false)
{}

QUrl GetUserProfileJob::makeRequestUrl(QUrl baseUrl, const QString& userId)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   makePath("/_matrix/client/v3", "/profile/", userId));
}

GetUserProfileJob::GetUserProfileJob(const QString& userId)
    : BaseJob(HttpVerb::Get, QStringLiteral("GetUserProfileJob"),
              makePath("/_matrix/client/v3", "/profile/", userId), false)
{}
