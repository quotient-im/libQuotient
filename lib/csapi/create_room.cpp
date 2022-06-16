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
              makePath("/_matrix/client/v3", "/createRoom"))
{
    QJsonObject _dataJson;
    addParam<IfNotEmpty>(_dataJson, QStringLiteral("visibility"), visibility);
    addParam<IfNotEmpty>(_dataJson, QStringLiteral("room_alias_name"),
                         roomAliasName);
    addParam<IfNotEmpty>(_dataJson, QStringLiteral("name"), name);
    addParam<IfNotEmpty>(_dataJson, QStringLiteral("topic"), topic);
    addParam<IfNotEmpty>(_dataJson, QStringLiteral("invite"), invite);
    addParam<IfNotEmpty>(_dataJson, QStringLiteral("invite_3pid"), invite3pid);
    addParam<IfNotEmpty>(_dataJson, QStringLiteral("room_version"), roomVersion);
    addParam<IfNotEmpty>(_dataJson, QStringLiteral("creation_content"),
                         creationContent);
    addParam<IfNotEmpty>(_dataJson, QStringLiteral("initial_state"),
                         initialState);
    addParam<IfNotEmpty>(_dataJson, QStringLiteral("preset"), preset);
    addParam<IfNotEmpty>(_dataJson, QStringLiteral("is_direct"), isDirect);
    addParam<IfNotEmpty>(_dataJson,
                         QStringLiteral("power_level_content_override"),
                         powerLevelContentOverride);
    setRequestData({ _dataJson });
    addExpectedKey("room_id");
}
