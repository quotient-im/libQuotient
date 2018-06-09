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
        addParam<>(_json, "id_server", pod.idServer);
        addParam<>(_json, "medium", pod.medium);
        addParam<>(_json, "address", pod.address);
        return _json;
    }

    QJsonObject toJson(const CreateRoomJob::StateEvent& pod)
    {
        QJsonObject _json;
        addParam<IfNotEmpty>(_json, "type", pod.type);
        addParam<IfNotEmpty>(_json, "state_key", pod.stateKey);
        addParam<IfNotEmpty>(_json, "content", pod.content);
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
    addParam<IfNotEmpty>(_data, "visibility", visibility);
    addParam<IfNotEmpty>(_data, "room_alias_name", roomAliasName);
    addParam<IfNotEmpty>(_data, "name", name);
    addParam<IfNotEmpty>(_data, "topic", topic);
    addParam<IfNotEmpty>(_data, "invite", invite);
    addParam<IfNotEmpty>(_data, "invite_3pid", invite3pid);
    addParam<IfNotEmpty>(_data, "creation_content", creationContent);
    addParam<IfNotEmpty>(_data, "initial_state", initialState);
    addParam<IfNotEmpty>(_data, "preset", preset);
    addParam<IfNotEmpty>(_data, "is_direct", isDirect);
    addParam<IfNotEmpty>(_data, "guest_can_join", guestCanJoin);
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

