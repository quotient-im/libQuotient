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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "user.h"

#include "connection.h"
#include "room.h"
#include "avatar.h"
#include "events/event.h"
#include "events/roommemberevent.h"
#include "jobs/setroomstatejob.h"
#include "jobs/generated/profile.h"
#include "jobs/generated/content-repo.h"

#include <QtCore/QTimer>
#include <QtCore/QRegularExpression>
#include <QtCore/QPointer>
#include <QtCore/QStringBuilder>
#include <QtCore/QElapsedTimer>

#include <functional>
#include <unordered_set>

using namespace QMatrixClient;
using std::move;
using std::exchange;

class User::Private
{
    public:
        static QIcon defaultIcon();
        static Avatar makeAvatar(QUrl url) { return { url, defaultIcon() }; }

        Private(QString userId, Connection* connection)
            : userId(move(userId)), connection(connection)
        { }

        QString userId;
        Connection* connection;

        QString mostUsedName;
        QString bridged;
        Avatar mostUsedAvatar { defaultIcon() };
        QMultiHash<QString, const Room*> otherNames;
        std::vector<std::pair<Avatar,
                              std::unordered_set<const Room*>>> otherAvatars;

        mutable int totalRooms = 0;

        QString nameForRoom(const Room* r, const QString& hint = {}) const;
        void setNameForRoom(const Room* r, QString newName, QString oldName);
        const Avatar& avatarForRoom(const Room* r) const;
        bool setAvatarUrlForRoom(const Room* r, const QUrl& avatarUrl);

        void setAvatar(QString contentUri, User* q);

};

QIcon User::Private::defaultIcon()
{
    static const QIcon icon
        { QIcon::fromTheme(QStringLiteral("user-available")) };
    return icon;
}

QString User::Private::nameForRoom(const Room* r, const QString& hint) const
{
    // If the hint is accurate, this function is O(1) instead of O(n)
    if (hint == mostUsedName || otherNames.contains(hint, r))
        return hint;
    return otherNames.key(r, mostUsedName);
}

static constexpr int MIN_JOINED_ROOMS_TO_LOG = 20;

void User::Private::setNameForRoom(const Room* r, QString newName,
                                   QString oldName)
{
    Q_ASSERT(oldName != newName);
    Q_ASSERT(oldName == mostUsedName || otherNames.contains(oldName, r));
    if (totalRooms < 2)
    {
        Q_ASSERT_X(totalRooms > 0 && otherNames.empty(), __FUNCTION__,
                   "Internal structures inconsistency");
        mostUsedName = move(newName);
        return;
    }
    otherNames.remove(oldName, r);
    if (newName != mostUsedName)
    {
        // Check if the newName is about to become most used.
        if (otherNames.count(newName) >= totalRooms - otherNames.size())
        {
            Q_ASSERT(totalRooms > 1);
            QElapsedTimer et;
            if (totalRooms > MIN_JOINED_ROOMS_TO_LOG)
            {
                qCDebug(MAIN) << "Switching the most used name of user" << userId
                              << "from" << mostUsedName << "to" << newName;
                qCDebug(MAIN) << "The user is in" << totalRooms << "rooms";
                et.start();
            }

            for (auto* r1: connection->roomMap())
                if (nameForRoom(r1) == mostUsedName)
                    otherNames.insert(mostUsedName, r1);

            mostUsedName = newName;
            otherNames.remove(newName);
            if (totalRooms > MIN_JOINED_ROOMS_TO_LOG)
                qCDebug(PROFILER) << et.elapsed()
                                  << "ms to switch the most used name";
        }
        else
            otherNames.insert(newName, r);
    }
}

const Avatar& User::Private::avatarForRoom(const Room* r) const
{
    for (const auto& p: otherAvatars)
    {
        auto roomIt = p.second.find(r);
        if (roomIt != p.second.end())
            return p.first;
    }
    return mostUsedAvatar;
}

bool User::Private::setAvatarUrlForRoom(const Room* r, const QUrl& avatarUrl)
{
    if (totalRooms < 2)
    {
        Q_ASSERT_X(totalRooms > 0 && otherAvatars.empty(), __FUNCTION__,
                   "Internal structures inconsistency");
        return
            exchange(mostUsedAvatar, makeAvatar(avatarUrl)).url() != avatarUrl;
    }
    for (auto it = otherAvatars.begin(); it != otherAvatars.end(); ++it)
    {
        auto roomIt = it->second.find(r);
        if (roomIt != it->second.end())
        {
            if (it->first.url() == avatarUrl)
                return false; // old url and new url coincide
            it->second.erase(r);
            if (avatarUrl == mostUsedAvatar.url())
            {
                if (it->second.empty())
                    otherAvatars.erase(it);
                // The most used avatar will be used for this room
                return true;
            }
        }
    }
    if (avatarUrl != mostUsedAvatar.url())
    {
        size_t othersCount = 0;
        auto entryToReplace = otherAvatars.end();
        for (auto it = otherAvatars.begin(); it != otherAvatars.end(); ++it)
        {
            othersCount += it->second.size();
            if (it->first.url() == avatarUrl)
                entryToReplace = it;
        }
        // Check if the new avatar is about to become most used.
        if (entryToReplace != otherAvatars.end() &&
                entryToReplace->second.size() >= size_t(totalRooms) - othersCount)
        {
            QElapsedTimer et;
            if (totalRooms > MIN_JOINED_ROOMS_TO_LOG)
            {
                qCDebug(MAIN) << "Switching the most used avatar of user" << userId
                              << "from" << mostUsedAvatar.url().toDisplayString()
                              << "to" << avatarUrl.toDisplayString();
                et.start();
            }
            entryToReplace->first =
                exchange(mostUsedAvatar, makeAvatar(avatarUrl));
            entryToReplace->second.clear();
            for (const auto* r1: connection->roomMap())
            {
                if (avatarForRoom(r1).url() == mostUsedAvatar.url())
                    entryToReplace->second.insert(r1);
            }
            if (totalRooms > MIN_JOINED_ROOMS_TO_LOG)
                qCDebug(PROFILER) << et.elapsed()
                                  << "ms to switch the most used avatar";
        }
    }
    if (avatarUrl != mostUsedAvatar.url()) // It could have changed above
    {
        // Create a new entry if necessary and add the room to it.
        auto it = find_if(otherAvatars.begin(), otherAvatars.end(),
                    [&avatarUrl] (const auto& p) {
                        return p.first.url() == avatarUrl;
                    });
        if (it == otherAvatars.end())
        {
            otherAvatars.push_back({ Avatar(avatarUrl, defaultIcon()), {} });
            it = otherAvatars.end() - 1;
        }
        it->second.insert(r);
    }
    return true;
}

User::User(QString userId, Connection* connection)
    : QObject(connection), d(new Private(std::move(userId), connection))
{
    setObjectName(userId);
}

User::~User() = default;

QString User::id() const
{
    return d->userId;
}

bool User::isGuest() const
{
    Q_ASSERT(!d->userId.isEmpty() && d->userId.startsWith('@'));
    auto it = std::find_if_not(d->userId.begin() + 1, d->userId.end(),
                               [] (QChar c) { return c.isDigit(); });
    Q_ASSERT(it != d->userId.end());
    return *it == ':';
}

QString User::name(const Room* room) const
{
    return d->nameForRoom(room);
}

void User::updateName(const QString& newName, const Room* room)
{
    updateName(newName, d->nameForRoom(room), room);
}

void User::updateName(const QString& newName, const QString& oldName,
                      const Room* room)
{
    Q_ASSERT(oldName == d->mostUsedName || d->otherNames.contains(oldName, room));
    if (newName != oldName)
    {
        emit nameAboutToChange(newName, oldName, room);
        d->setNameForRoom(room, newName, oldName);
        setObjectName(displayname());
        emit nameChanged(newName, oldName, room);
    }
}

void User::rename(const QString& newName)
{
    auto job = d->connection->callApi<SetDisplayNameJob>(id(), newName);
    connect(job, &BaseJob::success, this, [=] { updateName(newName); });
}

void User::rename(const QString& newName, const Room* r)
{
    if (!r)
    {
        qCWarning(MAIN) << "Passing a null room to two-argument User::rename()"
                           "is incorrect; client developer, please fix it";
        rename(newName);
    }
    Q_ASSERT_X(r->memberJoinState(this) == JoinState::Join, __FUNCTION__,
               "Attempt to rename a user that's not a room member");
    MemberEventContent evtC;
    evtC.displayName = newName;
    auto job = d->connection->callApi<SetRoomStateJob>(
                r->id(), id(), RoomMemberEvent(move(evtC)));
    connect(job, &BaseJob::success, this, [=] { updateName(newName, r); });
}

bool User::setAvatar(const QString& fileName)
{
    return avatarObject().upload(d->connection, fileName,
                [this] (QString contentUri) { d->setAvatar(contentUri, this); });
}

bool User::setAvatar(QIODevice* source)
{
    return avatarObject().upload(d->connection, source,
                [this] (QString contentUri) { d->setAvatar(contentUri, this); });
}

void User::Private::setAvatar(QString contentUri, User* q)
{
    auto* j = connection->callApi<SetAvatarUrlJob>(userId, contentUri);
    connect(j, &BaseJob::success, q, [q] { emit q->avatarChanged(q, nullptr); });
}

QString User::displayname(const Room* room) const
{
    auto name = d->nameForRoom(room);
    return name.isEmpty() ? d->userId :
           room ? room->roomMembername(name) : name;
}

QString User::fullName(const Room* room) const
{
    auto name = d->nameForRoom(room);
    return name.isEmpty() ? d->userId : name % " (" % d->userId % ')';
}

QString User::bridged() const
{
    return d->bridged;
}

const Avatar& User::avatarObject(const Room* room) const
{
    return d->avatarForRoom(room);
}

QImage User::avatar(int dimension, const Room* room)
{
    return avatar(dimension, dimension, room);
}

QImage User::avatar(int width, int height, const Room* room)
{
    return avatar(width, height, room, []{});
}

QImage User::avatar(int width, int height, const Room* room,
                    Avatar::get_callback_t callback)
{
    return avatarObject(room).get(d->connection, width, height,
                [=] { emit avatarChanged(this, room); callback(); });
}

QString User::avatarMediaId(const Room* room) const
{
    return avatarObject(room).mediaId();
}

QUrl User::avatarUrl(const Room* room) const
{
    return avatarObject(room).url();
}

void User::processEvent(RoomMemberEvent* event, const Room* room)
{
    if (event->membership() != MembershipType::Invite &&
            event->membership() != MembershipType::Join)
        return;

    auto aboutToEnter = room->memberJoinState(this) == JoinState::Leave &&
            (event->membership() == MembershipType::Join ||
             event->membership() == MembershipType::Invite);
    if (aboutToEnter)
        ++d->totalRooms;

    auto newName = event->displayName();
    // `bridged` value uses the same notification signal as the name;
    // it is assumed that first setting of the bridge occurs together with
    // the first setting of the name, and further bridge updates are
    // exceptionally rare (the only reasonable case being that the bridge
    // changes the naming convention). For the same reason room-specific
    // bridge tags are not supported at all.
    QRegularExpression reSuffix(" \\((IRC|Gitter|Telegram)\\)$");
    auto match = reSuffix.match(newName);
    if (match.hasMatch())
    {
        if (d->bridged != match.captured(1))
        {
            if (!d->bridged.isEmpty())
                qCWarning(MAIN) << "Bridge for user" << id() << "changed:"
                                << d->bridged << "->" << match.captured(1);
            d->bridged = match.captured(1);
        }
        newName.truncate(match.capturedStart(0));
    }
    if (event->prevContent())
        updateName(event->displayName(),
                   d->nameForRoom(room, event->prevContent()->displayName),
                   room);
    else
        updateName(event->displayName(), room);
    if (d->setAvatarUrlForRoom(room, event->avatarUrl()))
        emit avatarChanged(this, room);
}
