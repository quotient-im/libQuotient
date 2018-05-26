/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "create_room.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

namespace QMatrixClient
{
    // Converters

    QJsonObject toJson(const CreateRoomJob::Invite3pid& pod)
    {
        QJsonObject _json;
        if (pod.omitted)
            return _json;

        addToJson<>(_json, "id_server", pod.idServer);
        addToJson<>(_json, "medium", pod.medium);
        addToJson<>(_json, "address", pod.address);
        return _json;
    }

    QJsonObject toJson(const CreateRoomJob::StateEvent& pod)
    {
        QJsonObject _json;
        if (pod.omitted)
            return _json;

        addToJson<IfNotEmpty>(_json, "type", pod.type);
        addToJson<IfNotEmpty>(_json, "state_key", pod.stateKey);
        addToJson<IfNotEmpty>(_json, "content", pod.content);
        return _json;
    }
} // namespace QMatrixClient

class CreateRoomJob::Private
{
    public:
        QString roomId;
};

CreateRoomJob::CreateRoomJob(const QString& visibility, const QString& roomAliasName, const QString& name, const QString& topic, const QStringList& invite, const QVector<Invite3pid>& invite3pid, const QJsonObject& creationContent, const QVector<StateEvent>& initialState, const QString& preset, bool isDirect, bool guestCanJoin)
    : BaseJob(HttpVerb::Post, "CreateRoomJob",
        basePath % "/createRoom")
    , d(new Private)
{
    QJsonObject _data;
    addToJson<IfNotEmpty>(_data, "visibility", visibility);
    addToJson<IfNotEmpty>(_data, "room_alias_name", roomAliasName);
    addToJson<IfNotEmpty>(_data, "name", name);
    addToJson<IfNotEmpty>(_data, "topic", topic);
    addToJson<IfNotEmpty>(_data, "invite", invite);
    addToJson<IfNotEmpty>(_data, "invite_3pid", invite3pid);
    addToJson<IfNotEmpty>(_data, "creation_content", creationContent);
    addToJson<IfNotEmpty>(_data, "initial_state", initialState);
    addToJson<IfNotEmpty>(_data, "preset", preset);
    addToJson<IfNotEmpty>(_data, "is_direct", isDirect);
    addToJson<IfNotEmpty>(_data, "guest_can_join", guestCanJoin);
    setRequestData(_data);
}

CreateRoomJob::~CreateRoomJob() = default;

const QString& CreateRoomJob::roomId() const
{
    return d->roomId;
}

BaseJob::Status CreateRoomJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    d->roomId = fromJson<QString>(json.value("room_id"));
    return Success;
}

