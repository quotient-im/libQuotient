/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "create_room.h"

#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

namespace QMatrixClient
{
    // Converters

    QJsonObject toJson(const CreateRoomJob::Invite3pid& pod)
    {
        QJsonObject o;
        o.insert("id_server", toJson(pod.idServer));
        o.insert("medium", toJson(pod.medium));
        o.insert("address", toJson(pod.address));

        return o;
    }

    QJsonObject toJson(const CreateRoomJob::StateEvent& pod)
    {
        QJsonObject o;
        o.insert("type", toJson(pod.type));
        o.insert("state_key", toJson(pod.stateKey));
        o.insert("content", toJson(pod.content));

        return o;
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
    if (!visibility.isEmpty())
        _data.insert("visibility", toJson(visibility));
    if (!roomAliasName.isEmpty())
        _data.insert("room_alias_name", toJson(roomAliasName));
    if (!name.isEmpty())
        _data.insert("name", toJson(name));
    if (!topic.isEmpty())
        _data.insert("topic", toJson(topic));
    _data.insert("invite", toJson(invite));
    _data.insert("invite_3pid", toJson(invite3pid));
    _data.insert("creation_content", toJson(creationContent));
    _data.insert("initial_state", toJson(initialState));
    if (!preset.isEmpty())
        _data.insert("preset", toJson(preset));
    _data.insert("is_direct", toJson(isDirect));
    _data.insert("guest_can_join", toJson(guestCanJoin));
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

