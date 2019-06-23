/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "rooms.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

class GetOneRoomEventJob::Private
{
public:
    EventPtr data;
};

QUrl GetOneRoomEventJob::makeRequestUrl(QUrl baseUrl, const QString& roomId,
                                        const QString& eventId)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl), basePath % "/rooms/"
                                                           % roomId % "/event/"
                                                           % eventId);
}

static const auto GetOneRoomEventJobName = QStringLiteral("GetOneRoomEventJob");

GetOneRoomEventJob::GetOneRoomEventJob(const QString& roomId,
                                       const QString& eventId)
    : BaseJob(HttpVerb::Get, GetOneRoomEventJobName,
              basePath % "/rooms/" % roomId % "/event/" % eventId)
    , d(new Private)
{}

GetOneRoomEventJob::~GetOneRoomEventJob() = default;

EventPtr&& GetOneRoomEventJob::data() { return std::move(d->data); }

BaseJob::Status GetOneRoomEventJob::parseJson(const QJsonDocument& data)
{
    fromJson(data, d->data);
    return Success;
}

QUrl GetRoomStateWithKeyJob::makeRequestUrl(QUrl baseUrl, const QString& roomId,
                                            const QString& eventType,
                                            const QString& stateKey)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   basePath % "/rooms/" % roomId % "/state/"
                                       % eventType % "/" % stateKey);
}

static const auto GetRoomStateWithKeyJobName =
    QStringLiteral("GetRoomStateWithKeyJob");

GetRoomStateWithKeyJob::GetRoomStateWithKeyJob(const QString& roomId,
                                               const QString& eventType,
                                               const QString& stateKey)
    : BaseJob(HttpVerb::Get, GetRoomStateWithKeyJobName,
              basePath % "/rooms/" % roomId % "/state/" % eventType % "/"
                  % stateKey)
{}

QUrl GetRoomStateByTypeJob::makeRequestUrl(QUrl baseUrl, const QString& roomId,
                                           const QString& eventType)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl), basePath % "/rooms/"
                                                           % roomId % "/state/"
                                                           % eventType);
}

static const auto GetRoomStateByTypeJobName =
    QStringLiteral("GetRoomStateByTypeJob");

GetRoomStateByTypeJob::GetRoomStateByTypeJob(const QString& roomId,
                                             const QString& eventType)
    : BaseJob(HttpVerb::Get, GetRoomStateByTypeJobName,
              basePath % "/rooms/" % roomId % "/state/" % eventType)
{}

class GetRoomStateJob::Private
{
public:
    StateEvents data;
};

QUrl GetRoomStateJob::makeRequestUrl(QUrl baseUrl, const QString& roomId)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   basePath % "/rooms/" % roomId % "/state");
}

static const auto GetRoomStateJobName = QStringLiteral("GetRoomStateJob");

GetRoomStateJob::GetRoomStateJob(const QString& roomId)
    : BaseJob(HttpVerb::Get, GetRoomStateJobName,
              basePath % "/rooms/" % roomId % "/state")
    , d(new Private)
{}

GetRoomStateJob::~GetRoomStateJob() = default;

StateEvents&& GetRoomStateJob::data() { return std::move(d->data); }

BaseJob::Status GetRoomStateJob::parseJson(const QJsonDocument& data)
{
    fromJson(data, d->data);
    return Success;
}

class GetMembersByRoomJob::Private
{
public:
    EventsArray<RoomMemberEvent> chunk;
};

BaseJob::Query queryToGetMembersByRoom(const QString& at,
                                       const QString& membership,
                                       const QString& notMembership)
{
    BaseJob::Query _q;
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
        std::move(baseUrl), basePath % "/rooms/" % roomId % "/members",
        queryToGetMembersByRoom(at, membership, notMembership));
}

static const auto GetMembersByRoomJobName =
    QStringLiteral("GetMembersByRoomJob");

GetMembersByRoomJob::GetMembersByRoomJob(const QString& roomId,
                                         const QString& at,
                                         const QString& membership,
                                         const QString& notMembership)
    : BaseJob(HttpVerb::Get, GetMembersByRoomJobName,
              basePath % "/rooms/" % roomId % "/members",
              queryToGetMembersByRoom(at, membership, notMembership))
    , d(new Private)
{}

GetMembersByRoomJob::~GetMembersByRoomJob() = default;

EventsArray<RoomMemberEvent>&& GetMembersByRoomJob::chunk()
{
    return std::move(d->chunk);
}

BaseJob::Status GetMembersByRoomJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    fromJson(json.value("chunk"_ls), d->chunk);

    return Success;
}

// Converters
namespace QMatrixClient
{

template <>
struct JsonObjectConverter<GetJoinedMembersByRoomJob::RoomMember>
{
    static void fillFrom(const QJsonObject& jo,
                         GetJoinedMembersByRoomJob::RoomMember& result)
    {
        fromJson(jo.value("display_name"_ls), result.displayName);
        fromJson(jo.value("avatar_url"_ls), result.avatarUrl);
    }
};

} // namespace QMatrixClient

class GetJoinedMembersByRoomJob::Private
{
public:
    QHash<QString, RoomMember> joined;
};

QUrl GetJoinedMembersByRoomJob::makeRequestUrl(QUrl baseUrl,
                                               const QString& roomId)
{
    return BaseJob::makeRequestUrl(
        std::move(baseUrl), basePath % "/rooms/" % roomId % "/joined_members");
}

static const auto GetJoinedMembersByRoomJobName =
    QStringLiteral("GetJoinedMembersByRoomJob");

GetJoinedMembersByRoomJob::GetJoinedMembersByRoomJob(const QString& roomId)
    : BaseJob(HttpVerb::Get, GetJoinedMembersByRoomJobName,
              basePath % "/rooms/" % roomId % "/joined_members")
    , d(new Private)
{}

GetJoinedMembersByRoomJob::~GetJoinedMembersByRoomJob() = default;

const QHash<QString, GetJoinedMembersByRoomJob::RoomMember>&
GetJoinedMembersByRoomJob::joined() const
{
    return d->joined;
}

BaseJob::Status GetJoinedMembersByRoomJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    fromJson(json.value("joined"_ls), d->joined);

    return Success;
}
