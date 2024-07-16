// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "tags.h"

using namespace Quotient;

QUrl GetRoomTagsJob::makeRequestUrl(QUrl baseUrl, const QString& userId, const QString& roomId)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl), makePath("/_matrix/client/v3", "/user/",
                                                                userId, "/rooms/", roomId, "/tags"));
}

GetRoomTagsJob::GetRoomTagsJob(const QString& userId, const QString& roomId)
    : BaseJob(HttpVerb::Get, QStringLiteral("GetRoomTagsJob"),
              makePath("/_matrix/client/v3", "/user/", userId, "/rooms/", roomId, "/tags"))
{}

SetRoomTagJob::SetRoomTagJob(const QString& userId, const QString& roomId, const QString& tag,
                             const Tag& data)
    : BaseJob(HttpVerb::Put, QStringLiteral("SetRoomTagJob"),
              makePath("/_matrix/client/v3", "/user/", userId, "/rooms/", roomId, "/tags/", tag))
{
    setRequestData({ toJson(data) });
}

QUrl DeleteRoomTagJob::makeRequestUrl(QUrl baseUrl, const QString& userId, const QString& roomId,
                                      const QString& tag)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   makePath("/_matrix/client/v3", "/user/", userId, "/rooms/",
                                            roomId, "/tags/", tag));
}

DeleteRoomTagJob::DeleteRoomTagJob(const QString& userId, const QString& roomId, const QString& tag)
    : BaseJob(HttpVerb::Delete, QStringLiteral("DeleteRoomTagJob"),
              makePath("/_matrix/client/v3", "/user/", userId, "/rooms/", roomId, "/tags/", tag))
{}
