// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "create_room.h"

using namespace Quotient;

CreateRoomJob::CreateRoomJob(const QString& visibility, const QString& roomAliasName,
                             const QString& name, const QString& topic, const QStringList& invite,
                             const QVector<Invite3pid>& invite3pid, const QString& roomVersion,
                             const QJsonObject& creationContent,
                             const QVector<StateEvent>& initialState, const QString& preset,
                             std::optional<bool> isDirect,
                             const QJsonObject& powerLevelContentOverride)
    : BaseJob(HttpVerb::Post, u"CreateRoomJob"_s, makePath("/_matrix/client/v3", "/createRoom"))
{
    QJsonObject _dataJson;
    addParam<IfNotEmpty>(_dataJson, "visibility"_L1, visibility);
    addParam<IfNotEmpty>(_dataJson, "room_alias_name"_L1, roomAliasName);
    addParam<IfNotEmpty>(_dataJson, "name"_L1, name);
    addParam<IfNotEmpty>(_dataJson, "topic"_L1, topic);
    addParam<IfNotEmpty>(_dataJson, "invite"_L1, invite);
    addParam<IfNotEmpty>(_dataJson, "invite_3pid"_L1, invite3pid);
    addParam<IfNotEmpty>(_dataJson, "room_version"_L1, roomVersion);
    addParam<IfNotEmpty>(_dataJson, "creation_content"_L1, creationContent);
    addParam<IfNotEmpty>(_dataJson, "initial_state"_L1, initialState);
    addParam<IfNotEmpty>(_dataJson, "preset"_L1, preset);
    addParam<IfNotEmpty>(_dataJson, "is_direct"_L1, isDirect);
    addParam<IfNotEmpty>(_dataJson, "power_level_content_override"_L1, powerLevelContentOverride);
    setRequestData({ _dataJson });
    addExpectedKey(u"room_id"_s);
}
