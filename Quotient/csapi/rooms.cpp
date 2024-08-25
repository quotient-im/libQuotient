// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "rooms.h"

using namespace Quotient;

QUrl GetOneRoomEventJob::makeRequestUrl(const HomeserverData& hsData, const QString& roomId,
                                        const QString& eventId)
{
    return BaseJob::makeRequestUrl(hsData, makePath("/_matrix/client/v3", "/rooms/", roomId,
                                                    "/event/", eventId));
}

GetOneRoomEventJob::GetOneRoomEventJob(const QString& roomId, const QString& eventId)
    : BaseJob(HttpVerb::Get, u"GetOneRoomEventJob"_s,
              makePath("/_matrix/client/v3", "/rooms/", roomId, "/event/", eventId))
{}

QUrl GetRoomStateWithKeyJob::makeRequestUrl(const HomeserverData& hsData, const QString& roomId,
                                            const QString& eventType, const QString& stateKey)
{
    return BaseJob::makeRequestUrl(hsData, makePath("/_matrix/client/v3", "/rooms/", roomId,
                                                    "/state/", eventType, "/", stateKey));
}

GetRoomStateWithKeyJob::GetRoomStateWithKeyJob(const QString& roomId, const QString& eventType,
                                               const QString& stateKey)
    : BaseJob(HttpVerb::Get, u"GetRoomStateWithKeyJob"_s,
              makePath("/_matrix/client/v3", "/rooms/", roomId, "/state/", eventType, "/", stateKey))
{}

QUrl GetRoomStateJob::makeRequestUrl(const HomeserverData& hsData, const QString& roomId)
{
    return BaseJob::makeRequestUrl(hsData,
                                   makePath("/_matrix/client/v3", "/rooms/", roomId, "/state"));
}

GetRoomStateJob::GetRoomStateJob(const QString& roomId)
    : BaseJob(HttpVerb::Get, u"GetRoomStateJob"_s,
              makePath("/_matrix/client/v3", "/rooms/", roomId, "/state"))
{}

auto queryToGetMembersByRoom(const QString& at, const QString& membership,
                             const QString& notMembership)
{
    QUrlQuery _q;
    addParam<IfNotEmpty>(_q, u"at"_s, at);
    addParam<IfNotEmpty>(_q, u"membership"_s, membership);
    addParam<IfNotEmpty>(_q, u"not_membership"_s, notMembership);
    return _q;
}

QUrl GetMembersByRoomJob::makeRequestUrl(const HomeserverData& hsData, const QString& roomId,
                                         const QString& at, const QString& membership,
                                         const QString& notMembership)
{
    return BaseJob::makeRequestUrl(hsData,
                                   makePath("/_matrix/client/v3", "/rooms/", roomId, "/members"),
                                   queryToGetMembersByRoom(at, membership, notMembership));
}

GetMembersByRoomJob::GetMembersByRoomJob(const QString& roomId, const QString& at,
                                         const QString& membership, const QString& notMembership)
    : BaseJob(HttpVerb::Get, u"GetMembersByRoomJob"_s,
              makePath("/_matrix/client/v3", "/rooms/", roomId, "/members"),
              queryToGetMembersByRoom(at, membership, notMembership))
{}

QUrl GetJoinedMembersByRoomJob::makeRequestUrl(const HomeserverData& hsData, const QString& roomId)
{
    return BaseJob::makeRequestUrl(hsData, makePath("/_matrix/client/v3", "/rooms/", roomId,
                                                    "/joined_members"));
}

GetJoinedMembersByRoomJob::GetJoinedMembersByRoomJob(const QString& roomId)
    : BaseJob(HttpVerb::Get, u"GetJoinedMembersByRoomJob"_s,
              makePath("/_matrix/client/v3", "/rooms/", roomId, "/joined_members"))
{}
