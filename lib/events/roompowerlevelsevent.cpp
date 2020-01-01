#include "roompowerlevelsevent.h"

#include <QJsonDocument>

using namespace Quotient;

PowerLevelsEventContent::PowerLevelsEventContent(const QJsonObject& json) :
    invite(json["invite"_ls].toInt(50)),
    kick(json["kick"_ls].toInt(50)),
    ban(json["ban"_ls].toInt(50)),
    redact(json["redact"_ls].toInt(50)),
    events(fromJson<QHash<QString, int>>(json["events"_ls])),
    eventsDefault(json["events_default"_ls].toInt(0)),
    stateDefault(json["state_default"_ls].toInt(0)),
    users(fromJson<QHash<QString, int>>(json["users"_ls])),
    usersDefault(json["users_default"_ls].toInt(0)),
    notifications(Notifications{json["notifications"_ls]["room"_ls].toInt(50)})
{
}

void PowerLevelsEventContent::fillJson(QJsonObject* o) const {
    o->insert(QStringLiteral("invite"), invite);
    o->insert(QStringLiteral("kick"), kick);
    o->insert(QStringLiteral("ban"), ban);
    o->insert(QStringLiteral("redact"), redact);
    o->insert(QStringLiteral("events"), Quotient::toJson(events));
    o->insert(QStringLiteral("events_default"), eventsDefault);
    o->insert(QStringLiteral("state_default"), stateDefault);
    o->insert(QStringLiteral("users"), Quotient::toJson(users));
    o->insert(QStringLiteral("users_default"), usersDefault);
    o->insert(QStringLiteral("notifications"), {{"room", notifications.room}});
}

int RoomPowerLevelsEvent::powerLevelForEvent(const QString &eventId) const {
    auto e = events();

    if (e.contains(eventId)) {
        return e[eventId];
    }

    return eventsDefault();
}

int RoomPowerLevelsEvent::powerLevelForUser(const QString &userId) const {
    auto u = users();

    if (u.contains(userId)) {
        return u[userId];
    }

    return usersDefault();
}
