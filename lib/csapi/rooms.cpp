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

QUrl GetOneRoomEventJob::makeRequestUrl(QUrl baseUrl, const QString& roomId, const QString& eventId)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
            basePath % "/rooms/" % roomId % "/event/" % eventId);
}

GetOneRoomEventJob::GetOneRoomEventJob(const QString& roomId, const QString& eventId)
    : BaseJob(HttpVerb::Get, "GetOneRoomEventJob",
        basePath % "/rooms/" % roomId % "/event/" % eventId)
    , d(new Private)
{
}

GetOneRoomEventJob::~GetOneRoomEventJob() = default;

EventPtr&& GetOneRoomEventJob::data()
{
    return std::move(d->data);
}

BaseJob::Status GetOneRoomEventJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    if (!json.contains("data"))
        return { JsonParseError,
            "The key 'data' not found in the response" };
    d->data = fromJson<EventPtr>(json.value("data"));
    return Success;
}

class GetRoomStateWithKeyJob::Private
{
    public:
        StateEventPtr data;
};

QUrl GetRoomStateWithKeyJob::makeRequestUrl(QUrl baseUrl, const QString& roomId, const QString& eventType, const QString& stateKey)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
            basePath % "/rooms/" % roomId % "/state/" % eventType % "/" % stateKey);
}

GetRoomStateWithKeyJob::GetRoomStateWithKeyJob(const QString& roomId, const QString& eventType, const QString& stateKey)
    : BaseJob(HttpVerb::Get, "GetRoomStateWithKeyJob",
        basePath % "/rooms/" % roomId % "/state/" % eventType % "/" % stateKey)
    , d(new Private)
{
}

GetRoomStateWithKeyJob::~GetRoomStateWithKeyJob() = default;

StateEventPtr&& GetRoomStateWithKeyJob::data()
{
    return std::move(d->data);
}

BaseJob::Status GetRoomStateWithKeyJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    if (!json.contains("data"))
        return { JsonParseError,
            "The key 'data' not found in the response" };
    d->data = fromJson<StateEventPtr>(json.value("data"));
    return Success;
}

class GetRoomStateByTypeJob::Private
{
    public:
        StateEventPtr data;
};

QUrl GetRoomStateByTypeJob::makeRequestUrl(QUrl baseUrl, const QString& roomId, const QString& eventType)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
            basePath % "/rooms/" % roomId % "/state/" % eventType);
}

GetRoomStateByTypeJob::GetRoomStateByTypeJob(const QString& roomId, const QString& eventType)
    : BaseJob(HttpVerb::Get, "GetRoomStateByTypeJob",
        basePath % "/rooms/" % roomId % "/state/" % eventType)
    , d(new Private)
{
}

GetRoomStateByTypeJob::~GetRoomStateByTypeJob() = default;

StateEventPtr&& GetRoomStateByTypeJob::data()
{
    return std::move(d->data);
}

BaseJob::Status GetRoomStateByTypeJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    if (!json.contains("data"))
        return { JsonParseError,
            "The key 'data' not found in the response" };
    d->data = fromJson<StateEventPtr>(json.value("data"));
    return Success;
}

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

GetRoomStateJob::GetRoomStateJob(const QString& roomId)
    : BaseJob(HttpVerb::Get, "GetRoomStateJob",
        basePath % "/rooms/" % roomId % "/state")
    , d(new Private)
{
}

GetRoomStateJob::~GetRoomStateJob() = default;

StateEvents&& GetRoomStateJob::data()
{
    return std::move(d->data);
}

BaseJob::Status GetRoomStateJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    if (!json.contains("data"))
        return { JsonParseError,
            "The key 'data' not found in the response" };
    d->data = fromJson<StateEvents>(json.value("data"));
    return Success;
}

class GetMembersByRoomJob::Private
{
    public:
        EventsArray<RoomMemberEvent> chunk;
};

QUrl GetMembersByRoomJob::makeRequestUrl(QUrl baseUrl, const QString& roomId)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
            basePath % "/rooms/" % roomId % "/members");
}

GetMembersByRoomJob::GetMembersByRoomJob(const QString& roomId)
    : BaseJob(HttpVerb::Get, "GetMembersByRoomJob",
        basePath % "/rooms/" % roomId % "/members")
    , d(new Private)
{
}

GetMembersByRoomJob::~GetMembersByRoomJob() = default;

EventsArray<RoomMemberEvent>&& GetMembersByRoomJob::chunk()
{
    return std::move(d->chunk);
}

BaseJob::Status GetMembersByRoomJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    d->chunk = fromJson<EventsArray<RoomMemberEvent>>(json.value("chunk"));
    return Success;
}

namespace QMatrixClient
{
    // Converters

    template <> struct FromJson<GetJoinedMembersByRoomJob::RoomMember>
    {
        GetJoinedMembersByRoomJob::RoomMember operator()(const QJsonValue& jv)
        {
            const auto& _json = jv.toObject();
            GetJoinedMembersByRoomJob::RoomMember result;
            result.displayName =
                fromJson<QString>(_json.value("display_name"));
            result.avatarUrl =
                fromJson<QString>(_json.value("avatar_url"));

            return result;
        }
    };
} // namespace QMatrixClient

class GetJoinedMembersByRoomJob::Private
{
    public:
        QHash<QString, RoomMember> joined;
};

QUrl GetJoinedMembersByRoomJob::makeRequestUrl(QUrl baseUrl, const QString& roomId)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
            basePath % "/rooms/" % roomId % "/joined_members");
}

GetJoinedMembersByRoomJob::GetJoinedMembersByRoomJob(const QString& roomId)
    : BaseJob(HttpVerb::Get, "GetJoinedMembersByRoomJob",
        basePath % "/rooms/" % roomId % "/joined_members")
    , d(new Private)
{
}

GetJoinedMembersByRoomJob::~GetJoinedMembersByRoomJob() = default;

const QHash<QString, GetJoinedMembersByRoomJob::RoomMember>& GetJoinedMembersByRoomJob::joined() const
{
    return d->joined;
}

BaseJob::Status GetJoinedMembersByRoomJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    d->joined = fromJson<QHash<QString, RoomMember>>(json.value("joined"));
    return Success;
}

