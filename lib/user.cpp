/******************************************************************************
 * Copyright (C) 2015 Felix Rohrbach <kde@fxrh.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include "user.h"

#include "avatar.h"
#include "connection.h"
#include "room.h"

#include "csapi/content-repo.h"
#include "csapi/profile.h"
#include "csapi/room_state.h"

#include "events/event.h"
#include "events/roommemberevent.h"

#include <QtCore/QElapsedTimer>
#include <QtCore/QPointer>
#include <QtCore/QRegularExpression>
#include <QtCore/QStringBuilder>
#include <QtCore/QTimer>

#include <functional>

using namespace Quotient;
using namespace std::placeholders;
using std::move;

class User::Private {
public:
    static Avatar makeAvatar(QUrl url) { return Avatar(move(url)); }

    Private(QString userId, Connection* connection)
        : userId(move(userId))
        , connection(connection)
        , hueF(stringToHueF(this->userId))
    {}

    QString userId;
    Connection* connection;

    QString bridged;
    QString mostUsedName;
    QMultiHash<QString, const Room*> otherNames;
    qreal hueF;
    Avatar mostUsedAvatar { makeAvatar({}) };
    std::vector<Avatar> otherAvatars;
    auto otherAvatar(const QUrl& url)
    {
        return std::find_if(otherAvatars.begin(), otherAvatars.end(),
                            [&url](const auto& av) { return av.url() == url; });
    }
    QMultiHash<QUrl, const Room*> avatarsToRooms;

    mutable int totalRooms = 0;

    QString nameForRoom(const Room* r, const QString& hint = {}) const;
    void setNameForRoom(const Room* r, QString newName, const QString& oldName);
    QUrl avatarUrlForRoom(const Room* r, const QUrl& hint = {}) const;
    void setAvatarForRoom(const Room* r, const QUrl& newUrl, const QUrl& oldUrl);

    void setAvatarOnServer(QString contentUri, User* q);
};

QString User::Private::nameForRoom(const Room* r, const QString& hint) const
{
    // If the hint is accurate, this function is O(1) instead of O(n)
    if (!hint.isNull() && (hint == mostUsedName || otherNames.contains(hint, r)))
        return hint;
    return otherNames.key(r, mostUsedName);
}

static constexpr int MIN_JOINED_ROOMS_TO_LOG = 20;

void User::Private::setNameForRoom(const Room* r, QString newName,
                                   const QString& oldName)
{
    Q_ASSERT(oldName != newName);
    Q_ASSERT(oldName == mostUsedName || otherNames.contains(oldName, r));
    if (totalRooms < 2) {
        Q_ASSERT_X(totalRooms > 0 && otherNames.empty(), __FUNCTION__,
                   "Internal structures inconsistency");
        mostUsedName = move(newName);
        return;
    }
    otherNames.remove(oldName, r);
    if (newName != mostUsedName) {
        // Check if the newName is about to become most used.
        if (otherNames.count(newName) >= totalRooms - otherNames.size()) {
            Q_ASSERT(totalRooms > 1);
            QElapsedTimer et;
            if (totalRooms > MIN_JOINED_ROOMS_TO_LOG) {
                qCDebug(MAIN) << "Switching the most used name of user" << userId
                              << "from" << mostUsedName << "to" << newName;
                qCDebug(MAIN) << "The user is in" << totalRooms << "rooms";
                et.start();
            }

            for (auto* r1: connection->allRooms())
                if (nameForRoom(r1) == mostUsedName)
                    otherNames.insert(mostUsedName, r1);

            mostUsedName = newName;
            otherNames.remove(newName);
            if (totalRooms > MIN_JOINED_ROOMS_TO_LOG)
                qCDebug(PROFILER) << et << "to switch the most used name";
        } else
            otherNames.insert(newName, r);
    }
}

QUrl User::Private::avatarUrlForRoom(const Room* r, const QUrl& hint) const
{
    // If the hint is accurate, this function is O(1) instead of O(n)
    if (hint == mostUsedAvatar.url() || avatarsToRooms.contains(hint, r))
        return hint;
    auto it = std::find(avatarsToRooms.begin(), avatarsToRooms.end(), r);
    return it == avatarsToRooms.end() ? mostUsedAvatar.url() : it.key();
}

void User::Private::setAvatarForRoom(const Room* r, const QUrl& newUrl,
                                     const QUrl& oldUrl)
{
    Q_ASSERT(oldUrl != newUrl);
    Q_ASSERT(oldUrl == mostUsedAvatar.url()
             || avatarsToRooms.contains(oldUrl, r));
    if (totalRooms < 2) {
        Q_ASSERT_X(totalRooms > 0 && otherAvatars.empty(), __FUNCTION__,
                   "Internal structures inconsistency");
        mostUsedAvatar.updateUrl(newUrl);
        return;
    }
    avatarsToRooms.remove(oldUrl, r);
    if (!avatarsToRooms.contains(oldUrl)) {
        auto it = otherAvatar(oldUrl);
        if (it != otherAvatars.end())
            otherAvatars.erase(it);
    }
    if (newUrl != mostUsedAvatar.url()) {
        // Check if the new avatar is about to become most used.
        const auto newUrlUsage = avatarsToRooms.count(newUrl);
        if (newUrlUsage >= totalRooms - avatarsToRooms.size()) {
            QElapsedTimer et;
            if (totalRooms > MIN_JOINED_ROOMS_TO_LOG) {
                qCInfo(MAIN)
                    << "Switching the most used avatar of user" << userId
                    << "from" << mostUsedAvatar.url().toDisplayString() << "to"
                    << newUrl.toDisplayString();
                et.start();
            }
            avatarsToRooms.remove(newUrl);
            auto nextMostUsedIt = otherAvatar(newUrl);
            if (nextMostUsedIt == otherAvatars.end()) {
                qCCritical(MAIN)
                    << userId << "doesn't have" << newUrl.toDisplayString()
                    << "in otherAvatars though it seems to be used in"
                    << newUrlUsage << "rooms";
                Q_ASSERT(false);
                otherAvatars.emplace_back(makeAvatar(newUrl));
                nextMostUsedIt = otherAvatars.end() - 1;
            }
            std::swap(mostUsedAvatar, *nextMostUsedIt);
            for (const auto* r1: connection->allRooms())
                if (avatarUrlForRoom(r1) == nextMostUsedIt->url())
                    avatarsToRooms.insert(nextMostUsedIt->url(), r1);

            if (totalRooms > MIN_JOINED_ROOMS_TO_LOG)
                qCDebug(PROFILER) << et << "to switch the most used avatar";
        } else {
            if (otherAvatar(newUrl) == otherAvatars.end())
                otherAvatars.emplace_back(makeAvatar(newUrl));
            avatarsToRooms.insert(newUrl, r);
        }
    }
}

User::User(QString userId, Connection* connection)
    : QObject(connection), d(new Private(move(userId), connection))
{
    setObjectName(userId);
}

Connection* User::connection() const
{
    Q_ASSERT(d->connection);
    return d->connection;
}

User::~User() = default;

QString User::id() const { return d->userId; }

bool User::isGuest() const
{
    Q_ASSERT(!d->userId.isEmpty() && d->userId.startsWith('@'));
    auto it = std::find_if_not(d->userId.begin() + 1, d->userId.end(),
                               [](QChar c) { return c.isDigit(); });
    Q_ASSERT(it != d->userId.end());
    return *it == ':';
}

int User::hue() const { return int(hueF() * 359); }

QString User::name(const Room* room) const { return d->nameForRoom(room); }

QString User::rawName(const Room* room) const
{
    return d->bridged.isEmpty() ? name(room)
                                : name(room) % " (" % d->bridged % ')';
}

void User::updateName(const QString& newName, const Room* room)
{
    updateName(newName, d->nameForRoom(room), room);
}

void User::updateName(const QString& newName, const QString& oldName,
                      const Room* room)
{
    Q_ASSERT(oldName == d->mostUsedName
             || d->otherNames.contains(oldName, room));
    if (newName != oldName) {
        emit nameAboutToChange(newName, oldName, room);
        d->setNameForRoom(room, newName, oldName);
        setObjectName(displayname());
        emit nameChanged(newName, oldName, room);
    }
}

void User::updateAvatarUrl(const QUrl& newUrl, const QUrl& oldUrl,
                           const Room* room)
{
    Q_ASSERT(oldUrl == d->mostUsedAvatar.url()
             || d->avatarsToRooms.contains(oldUrl, room));
    if (newUrl != oldUrl) {
        d->setAvatarForRoom(room, newUrl, oldUrl);
        setObjectName(displayname());
        emit avatarChanged(this, room);
    }
}

void User::rename(const QString& newName)
{
    const auto actualNewName = sanitized(newName);
    connect(connection()->callApi<SetDisplayNameJob>(id(), actualNewName),
            &BaseJob::success, this, [=] { updateName(actualNewName); });
}

void User::rename(const QString& newName, const Room* r)
{
    if (!r) {
        qCWarning(MAIN) << "Passing a null room to two-argument User::rename()"
                           "is incorrect; client developer, please fix it";
        rename(newName);
        return;
    }
    Q_ASSERT_X(r->memberJoinState(this) == JoinState::Join, __FUNCTION__,
               "Attempt to rename a user that's not a room member");
    const auto actualNewName = sanitized(newName);
    MemberEventContent evtC;
    evtC.displayName = actualNewName;
    connect(r->setState<RoomMemberEvent>(id(), move(evtC)), &BaseJob::success,
            this, [=] { updateName(actualNewName, r); });
}

bool User::setAvatar(const QString& fileName)
{
    return avatarObject().upload(connection(), fileName,
                                 std::bind(&Private::setAvatarOnServer,
                                           d.data(), _1, this));
}

bool User::setAvatar(QIODevice* source)
{
    return avatarObject().upload(connection(), source,
                                 std::bind(&Private::setAvatarOnServer,
                                           d.data(), _1, this));
}

void User::requestDirectChat() { connection()->requestDirectChat(this); }

void User::ignore() { connection()->addToIgnoredUsers(this); }

void User::unmarkIgnore() { connection()->removeFromIgnoredUsers(this); }

bool User::isIgnored() const { return connection()->isIgnored(this); }

void User::Private::setAvatarOnServer(QString contentUri, User* q)
{
    auto* j = connection->callApi<SetAvatarUrlJob>(userId, contentUri);
    connect(j, &BaseJob::success, q,
            [=] { q->updateAvatarUrl(contentUri, avatarUrlForRoom(nullptr)); });
}

QString User::displayname(const Room* room) const
{
    if (room)
        return room->roomMembername(this);

    const auto name = d->nameForRoom(nullptr);
    return name.isEmpty() ? d->userId : name;
}

QString User::fullName(const Room* room) const
{
    const auto name = d->nameForRoom(room);
    return name.isEmpty() ? d->userId : name % " (" % d->userId % ')';
}

QString User::bridged() const { return d->bridged; }

const Avatar& User::avatarObject(const Room* room) const
{
    auto it = d->otherAvatar(d->avatarUrlForRoom(room));
    return it != d->otherAvatars.end() ? *it : d->mostUsedAvatar;
}

QImage User::avatar(int dimension, const Room* room)
{
    return avatar(dimension, dimension, room);
}

QImage User::avatar(int width, int height, const Room* room)
{
    return avatar(width, height, room, [] {});
}

QImage User::avatar(int width, int height, const Room* room,
                    const Avatar::get_callback_t& callback)
{
    return avatarObject(room).get(d->connection, width, height, [=] {
        emit avatarChanged(this, room);
        callback();
    });
}

QString User::avatarMediaId(const Room* room) const
{
    return avatarObject(room).mediaId();
}

QUrl User::avatarUrl(const Room* room) const
{
    return avatarObject(room).url();
}

void User::processEvent(const RoomMemberEvent& event, const Room* room,
                        bool firstMention)
{
    Q_ASSERT(room);

    if (firstMention)
        ++d->totalRooms;

    if (event.membership() != MembershipType::Invite
        && event.membership() != MembershipType::Join)
        return;

    auto newName = event.displayName();
    // `bridged` value uses the same notification signal as the name;
    // it is assumed that first setting of the bridge occurs together with
    // the first setting of the name, and further bridge updates are
    // exceptionally rare (the only reasonable case being that the bridge
    // changes the naming convention). For the same reason room-specific
    // bridge tags are not supported at all.
    QRegularExpression reSuffix(
        QStringLiteral(" \\((IRC|Gitter|Telegram)\\)$"));
    auto match = reSuffix.match(newName);
    if (match.hasMatch()) {
        if (d->bridged != match.captured(1)) {
            if (!d->bridged.isEmpty())
                qCWarning(MAIN)
                    << "Bridge for user" << id() << "changed:" << d->bridged
                    << "->" << match.captured(1);
            d->bridged = match.captured(1);
        }
        newName.truncate(match.capturedStart(0));
    }
    updateName(newName, room);
    updateAvatarUrl(event.avatarUrl(), d->avatarUrlForRoom(room), room);
}

qreal User::hueF() const { return d->hueF; }
