#include "roompowerlevelsevent.h"

#include <QJsonDocument>

using namespace Quotient;

inline QHash<QString, int> qVariantHashToQHash(const QVariantHash& vHash) {
    QHash<QString, int> hash;

    QVariantHash::const_iterator i = vHash.constBegin();

    while (i != vHash.constEnd()) {
        hash.insert(i.key(), i.value().toInt());
        ++i;
    }

    return hash;
}

inline QJsonObject qHashToJson(const QHash<QString, int>& hash) {
    QJsonObject json;

    QHash<QString, int>::const_iterator i = hash.constBegin();

    while (i != hash.constEnd()) {
        json.insert(i.key(), i.value());
        ++i;
    }

    return json;
}

PowerLevelsEventContent::PowerLevelsEventContent(const QJsonObject& json) :
    invite(json["invite"_ls].toInt(50)),
    kick(json["kick"_ls].toInt(50)),
    ban(json["ban"_ls].toInt(50)),
    redact(json["redact"_ls].toInt(50)),
    events(qVariantHashToQHash(json["events"_ls].toVariant().toHash())),
    eventsDefault(json["events_default"_ls].toInt(0)),
    stateDefault(json["state_default"_ls].toInt(0)),
    users(qVariantHashToQHash(json["users"_ls].toVariant().toHash())),
    usersDefault(json["users_default"_ls].toInt(0)),
    notifications(Notifications{json["notifications"_ls]["room"_ls].toInt(50)})
{
}

void PowerLevelsEventContent::fillJson(QJsonObject* o) const {
    o->insert(QStringLiteral("invite"), invite);
    o->insert(QStringLiteral("kick"), kick);
    o->insert(QStringLiteral("ban"), ban);
    o->insert(QStringLiteral("redact"), redact);
    o->insert(QStringLiteral("events"), qHashToJson(events));
    o->insert(QStringLiteral("events_default"), eventsDefault);
    o->insert(QStringLiteral("state_default"), stateDefault);
    o->insert(QStringLiteral("users"), qHashToJson(users));
    o->insert(QStringLiteral("users_default"), usersDefault);
    o->insert(QStringLiteral("notifications"), QJsonObject{{"room", notifications.room}});
}

int RoomPowerLevelsEvent::powerLevelForEvent(const QString &eventId) {
    auto e = events();

    if (e.contains(eventId)) {
        return e[eventId];
    }

    return eventsDefault();
}

int RoomPowerLevelsEvent::powerLevelForUser(const QString &userId) {
    auto u = users();

    if (u.contains(userId)) {
        return u[userId];
    }

    return usersDefault();
}
