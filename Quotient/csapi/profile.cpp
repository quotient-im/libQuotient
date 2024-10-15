// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "profile.h"

using namespace Quotient;

SetDisplayNameJob::SetDisplayNameJob(const QString& userId, const QString& displayname)
    : BaseJob(HttpVerb::Put, u"SetDisplayNameJob"_s,
              makePath("/_matrix/client/v3", "/profile/", userId, "/displayname"))
{
    QJsonObject _dataJson;
    addParam<>(_dataJson, "displayname"_L1, displayname);
    setRequestData({ _dataJson });
}

QUrl GetDisplayNameJob::makeRequestUrl(const HomeserverData& hsData, const QString& userId)
{
    return BaseJob::makeRequestUrl(hsData, makePath("/_matrix/client/v3", "/profile/", userId,
                                                    "/displayname"));
}

GetDisplayNameJob::GetDisplayNameJob(const QString& userId)
    : BaseJob(HttpVerb::Get, u"GetDisplayNameJob"_s,
              makePath("/_matrix/client/v3", "/profile/", userId, "/displayname"))
{}

SetAvatarUrlJob::SetAvatarUrlJob(const QString& userId, const QUrl& avatarUrl)
    : BaseJob(HttpVerb::Put, u"SetAvatarUrlJob"_s,
              makePath("/_matrix/client/v3", "/profile/", userId, "/avatar_url"))
{
    QJsonObject _dataJson;
    addParam<>(_dataJson, "avatar_url"_L1, avatarUrl);
    setRequestData({ _dataJson });
}

QUrl GetAvatarUrlJob::makeRequestUrl(const HomeserverData& hsData, const QString& userId)
{
    return BaseJob::makeRequestUrl(hsData, makePath("/_matrix/client/v3", "/profile/", userId,
                                                    "/avatar_url"));
}

GetAvatarUrlJob::GetAvatarUrlJob(const QString& userId)
    : BaseJob(HttpVerb::Get, u"GetAvatarUrlJob"_s,
              makePath("/_matrix/client/v3", "/profile/", userId, "/avatar_url"))
{}

QUrl GetUserProfileJob::makeRequestUrl(const HomeserverData& hsData, const QString& userId)
{
    return BaseJob::makeRequestUrl(hsData, makePath("/_matrix/client/v3", "/profile/", userId));
}

GetUserProfileJob::GetUserProfileJob(const QString& userId)
    : BaseJob(HttpVerb::Get, u"GetUserProfileJob"_s,
              makePath("/_matrix/client/v3", "/profile/", userId))
{}
