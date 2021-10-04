/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "create_room.h"

using namespace Quotient;

CreateRoomJob::CreateRoomJob(const QString& visibility,
                             const QString& roomAliasName, const QString& name,
                             const QString& topic, const QStringList& invite,
                             const QVector<Invite3pid>& invite3pid,
                             const QString& roomVersion,
                             const QJsonObject& creationContent,
                             const QVector<StateEvent>& initialState,
                             const QString& preset, Omittable<bool> isDirect,
                             const QJsonObject& powerLevelContentOverride)
    : BaseJob(HttpVerb::Post, QStringLiteral("CreateRoomJob"),
              makePath("/_matrix/client/r0", "/createRoom"))
{
    QJsonObject _data;
    addParam<IfNotEmpty>(_data, QStringLiteral("visibility"), visibility);
    addParam<IfNotEmpty>(_data, QStringLiteral("room_alias_name"),
                         roomAliasName);
    addParam<IfNotEmpty>(_data, QStringLiteral("name"), name);
    addParam<IfNotEmpty>(_data, QStringLiteral("topic"), topic);
    addParam<IfNotEmpty>(_data, QStringLiteral("invite"), invite);
    addParam<IfNotEmpty>(_data, QStringLiteral("invite_3pid"), invite3pid);
    addParam<IfNotEmpty>(_data, QStringLiteral("room_version"), roomVersion);
    addParam<IfNotEmpty>(_data, QStringLiteral("creation_content"),
                         creationContent);
    addParam<IfNotEmpty>(_data, QStringLiteral("initial_state"), initialState);
    addParam<IfNotEmpty>(_data, QStringLiteral("preset"), preset);
    addParam<IfNotEmpty>(_data, QStringLiteral("is_direct"), isDirect);
    addParam<IfNotEmpty>(_data, QStringLiteral("power_level_content_override"),
                         powerLevelContentOverride);
    setRequestData(std::move(_data));
    addExpectedKey("room_id");
}
