/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "rooms.h"

using namespace Quotient;

QUrl GetOneRoomEventJob::makeRequestUrl(QUrl baseUrl, const QString& roomId,
                                        const QString& eventId)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   makePath("/_matrix/client/v3", "/rooms/",
                                            roomId, "/event/", eventId));
}

GetOneRoomEventJob::GetOneRoomEventJob(const QString& roomId,
                                       const QString& eventId)
    : BaseJob(HttpVerb::Get, QStringLiteral("GetOneRoomEventJob"),
              makePath("/_matrix/client/v3", "/rooms/", roomId, "/event/",
                       eventId))
{}

QUrl GetRoomStateWithKeyJob::makeRequestUrl(QUrl baseUrl, const QString& roomId,
                                            const QString& eventType,
                                            const QString& stateKey)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   makePath("/_matrix/client/v3", "/rooms/",
                                            roomId, "/state/", eventType, "/",
                                            stateKey));
}

GetRoomStateWithKeyJob::GetRoomStateWithKeyJob(const QString& roomId,
                                               const QString& eventType,
                                               const QString& stateKey)
    : BaseJob(HttpVerb::Get, QStringLiteral("GetRoomStateWithKeyJob"),
              makePath("/_matrix/client/v3", "/rooms/", roomId, "/state/",
                       eventType, "/", stateKey))
{}

QUrl GetRoomStateJob::makeRequestUrl(QUrl baseUrl, const QString& roomId)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   makePath("/_matrix/client/v3", "/rooms/",
                                            roomId, "/state"));
}

GetRoomStateJob::GetRoomStateJob(const QString& roomId)
    : BaseJob(HttpVerb::Get, QStringLiteral("GetRoomStateJob"),
              makePath("/_matrix/client/v3", "/rooms/", roomId, "/state"))
{}

auto queryToGetMembersByRoom(const QString& at, const QString& membership,
                             const QString& notMembership)
{
    QUrlQuery _q;
    addParam<IfNotEmpty>(_q, QStringLiteral("at"), at);
    addParam<IfNotEmpty>(_q, QStringLiteral("membership"), membership);
    addParam<IfNotEmpty>(_q, QStringLiteral("not_membership"), notMembership);
    return _q;
}

QUrl GetMembersByRoomJob::makeRequestUrl(QUrl baseUrl, const QString& roomId,
                                         const QString& at,
                                         const QString& membership,
                                         const QString& notMembership)
{
    return BaseJob::makeRequestUrl(
        std::move(baseUrl),
        makePath("/_matrix/client/v3", "/rooms/", roomId, "/members"),
        queryToGetMembersByRoom(at, membership, notMembership));
}

GetMembersByRoomJob::GetMembersByRoomJob(const QString& roomId,
                                         const QString& at,
                                         const QString& membership,
                                         const QString& notMembership)
    : BaseJob(HttpVerb::Get, QStringLiteral("GetMembersByRoomJob"),
              makePath("/_matrix/client/v3", "/rooms/", roomId, "/members"),
              queryToGetMembersByRoom(at, membership, notMembership))
{}

QUrl GetJoinedMembersByRoomJob::makeRequestUrl(QUrl baseUrl,
                                               const QString& roomId)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   makePath("/_matrix/client/v3", "/rooms/",
                                            roomId, "/joined_members"));
}

GetJoinedMembersByRoomJob::GetJoinedMembersByRoomJob(const QString& roomId)
    : BaseJob(HttpVerb::Get, QStringLiteral("GetJoinedMembersByRoomJob"),
              makePath("/_matrix/client/v3", "/rooms/", roomId,
                       "/joined_members"))
{}
