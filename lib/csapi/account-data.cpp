/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "account-data.h"

using namespace Quotient;

SetAccountDataJob::SetAccountDataJob(const QString& userId, const QString& type,
                                     const QJsonObject& content)
    : BaseJob(HttpVerb::Put, QStringLiteral("SetAccountDataJob"),
              makePath("/_matrix/client/v3", "/user/", userId, "/account_data/",
                       type))
{
    setRequestData({ toJson(content) });
}

QUrl GetAccountDataJob::makeRequestUrl(QUrl baseUrl, const QString& userId,
                                       const QString& type)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   makePath("/_matrix/client/v3", "/user/",
                                            userId, "/account_data/", type));
}

GetAccountDataJob::GetAccountDataJob(const QString& userId, const QString& type)
    : BaseJob(HttpVerb::Get, QStringLiteral("GetAccountDataJob"),
              makePath("/_matrix/client/v3", "/user/", userId, "/account_data/",
                       type))
{}

SetAccountDataPerRoomJob::SetAccountDataPerRoomJob(const QString& userId,
                                                   const QString& roomId,
                                                   const QString& type,
                                                   const QJsonObject& content)
    : BaseJob(HttpVerb::Put, QStringLiteral("SetAccountDataPerRoomJob"),
              makePath("/_matrix/client/v3", "/user/", userId, "/rooms/",
                       roomId, "/account_data/", type))
{
    setRequestData({ toJson(content) });
}

QUrl GetAccountDataPerRoomJob::makeRequestUrl(QUrl baseUrl,
                                              const QString& userId,
                                              const QString& roomId,
                                              const QString& type)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   makePath("/_matrix/client/v3", "/user/",
                                            userId, "/rooms/", roomId,
                                            "/account_data/", type));
}

GetAccountDataPerRoomJob::GetAccountDataPerRoomJob(const QString& userId,
                                                   const QString& roomId,
                                                   const QString& type)
    : BaseJob(HttpVerb::Get, QStringLiteral("GetAccountDataPerRoomJob"),
              makePath("/_matrix/client/v3", "/user/", userId, "/rooms/",
                       roomId, "/account_data/", type))
{}
