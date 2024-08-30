// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "account-data.h"

using namespace Quotient;

SetAccountDataJob::SetAccountDataJob(const QString& userId, const QString& type,
                                     const QJsonObject& content)
    : BaseJob(HttpVerb::Put, u"SetAccountDataJob"_s,
              makePath("/_matrix/client/v3", "/user/", userId, "/account_data/", type))
{
    setRequestData({ toJson(content) });
}

QUrl GetAccountDataJob::makeRequestUrl(const HomeserverData& hsData, const QString& userId,
                                       const QString& type)
{
    return BaseJob::makeRequestUrl(hsData, makePath("/_matrix/client/v3", "/user/", userId,
                                                    "/account_data/", type));
}

GetAccountDataJob::GetAccountDataJob(const QString& userId, const QString& type)
    : BaseJob(HttpVerb::Get, u"GetAccountDataJob"_s,
              makePath("/_matrix/client/v3", "/user/", userId, "/account_data/", type))
{}

SetAccountDataPerRoomJob::SetAccountDataPerRoomJob(const QString& userId, const QString& roomId,
                                                   const QString& type, const QJsonObject& content)
    : BaseJob(HttpVerb::Put, u"SetAccountDataPerRoomJob"_s,
              makePath("/_matrix/client/v3", "/user/", userId, "/rooms/", roomId, "/account_data/",
                       type))
{
    setRequestData({ toJson(content) });
}

QUrl GetAccountDataPerRoomJob::makeRequestUrl(const HomeserverData& hsData, const QString& userId,
                                              const QString& roomId, const QString& type)
{
    return BaseJob::makeRequestUrl(hsData, makePath("/_matrix/client/v3", "/user/", userId,
                                                    "/rooms/", roomId, "/account_data/", type));
}

GetAccountDataPerRoomJob::GetAccountDataPerRoomJob(const QString& userId, const QString& roomId,
                                                   const QString& type)
    : BaseJob(HttpVerb::Get, u"GetAccountDataPerRoomJob"_s,
              makePath("/_matrix/client/v3", "/user/", userId, "/rooms/", roomId, "/account_data/",
                       type))
{}
