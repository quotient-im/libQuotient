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
using std::move;

class User::Private {
public:
    Private(QString userId) : id(move(userId)), hueF(stringToHueF(id)) { }

    QString id;
    qreal hueF;

    // In the following two, isNull/nullopt mean they are uninitialised;
    // isEmpty/Avatar::url().isEmpty() mean they are initialised but empty.
    QString defaultName;
    std::optional<Avatar> defaultAvatar;

    // NB: This container is ever-growing. Even if the user no more scrolls
    // the timeline that far back, historical avatars are still kept around.
    // This is consistent with the rest of Quotient, as room timelines
    // are never rotated either. This will probably change in the future.
    /// Map of mediaId to Avatar objects
    static UnorderedMap<QString, Avatar> otherAvatars;

    void fetchProfile(const User* q);

    template <typename SourceT>
    bool doSetAvatar(SourceT&& source, User* q);
};

decltype(User::Private::otherAvatars) User::Private::otherAvatars {};

void User::Private::fetchProfile(const User* q)
{
    defaultAvatar.emplace(Avatar {});
    defaultName = "";
    auto* j = q->connection()->callApi<GetUserProfileJob>(BackgroundRequest, id);
    // FIXME: accepting const User* and const_cast'ing it here is only
    //        until we get a better User API in 0.7
    QObject::connect(j, &BaseJob::success, q,
                     [this, q = const_cast<User*>(q), j] {
                         q->updateName(j->displayname());
                         defaultAvatar->updateUrl(j->avatarUrl());
                         emit q->avatarChanged(q, nullptr);
                     });
}

User::User(QString userId, Connection* connection)
    : QObject(connection), d(new Private(move(userId)))
{
    setObjectName(id());
}

Connection* User::connection() const
{
    Q_ASSERT(parent());
    return static_cast<Connection*>(parent());
}

User::~User() = default;

QString User::id() const { return d->id; }

bool User::isGuest() const
{
    Q_ASSERT(!d->id.isEmpty() && d->id.startsWith('@'));
    auto it = std::find_if_not(d->id.begin() + 1, d->id.end(),
                               [](QChar c) { return c.isDigit(); });
    Q_ASSERT(it != d->id.end());
    return *it == ':';
}

int User::hue() const { return int(hueF() * 359); }

QString User::name(const Room* room) const
{
    if (room)
        return room->getCurrentState<RoomMemberEvent>(id())->displayName();

    if (d->defaultName.isNull())
        d->fetchProfile(this);

    return d->defaultName;
}

QString User::rawName(const Room* room) const { return name(room); }

void User::updateName(const QString& newName, const Room* r)
{
    Q_ASSERT(r == nullptr);
    if (newName == d->defaultName)
        return;

    emit nameAboutToChange(newName, d->defaultName, nullptr);
    const auto& oldName =
        std::exchange(d->defaultName, newName);
    emit nameChanged(d->defaultName, oldName, nullptr);
}
void User::updateName(const QString&, const QString&, const Room*) {}
void User::updateAvatarUrl(const QUrl&, const QUrl&, const Room*) {}

void User::rename(const QString& newName)
{
    const auto actualNewName = sanitized(newName);
    if (actualNewName == d->defaultName)
        return; // Nothing to do

    connect(connection()->callApi<SetDisplayNameJob>(id(), actualNewName),
            &BaseJob::success, this, [this, actualNewName] {
                d->fetchProfile(this);
                updateName(actualNewName);
            });
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
    r->setState<RoomMemberEvent>(id(), move(evtC));
    // The state will be updated locally after it arrives with sync
}

template <typename SourceT>
bool User::Private::doSetAvatar(SourceT&& source, User* q)
{
    if (!defaultAvatar) {
        defaultName = "";
        defaultAvatar.emplace(Avatar {});
    }
    return defaultAvatar->upload(
        q->connection(), source, [this, q](const QString& contentUri) {
            auto* j =
                q->connection()->callApi<SetAvatarUrlJob>(id, contentUri);
            QObject::connect(j, &BaseJob::success, q,
                             [this, q, newUrl = QUrl(contentUri)] {
                                 // Fetch displayname to complete the profile
                                 fetchProfile(q);
                                 if (newUrl == defaultAvatar->url()) {
                                     qCWarning(MAIN)
                                         << "User" << id
                                         << "already has avatar URL set to"
                                         << newUrl.toDisplayString();
                                     return;
                                 }

                                 defaultAvatar->updateUrl(move(newUrl));
                                 emit q->avatarChanged(q, nullptr);
                             });
        });
}

bool User::setAvatar(const QString& fileName)
{
    return d->doSetAvatar(fileName, this);
}

bool User::setAvatar(QIODevice* source)
{
    return d->doSetAvatar(source, this);
}

void User::requestDirectChat() { connection()->requestDirectChat(this); }

void User::ignore() { connection()->addToIgnoredUsers(this); }

void User::unmarkIgnore() { connection()->removeFromIgnoredUsers(this); }

bool User::isIgnored() const { return connection()->isIgnored(this); }

QString User::displayname(const Room* room) const
{
    if (room)
        return room->roomMembername(this);

    if (auto n = name(); !n.isEmpty())
        return n;

    return d->id;
}

QString User::fullName(const Room* room) const
{
    const auto displayName = name(room);
    return displayName.isEmpty() ? id() : (displayName % " (" % id() % ')');
}

QString User::bridged() const { return {}; }

const Avatar& User::avatarObject(const Room* room) const
{
    if (!room) {
        if (!d->defaultAvatar) {
            d->fetchProfile(this);
        }
        return *d->defaultAvatar;
    }

    const auto& url = room->getCurrentState<RoomMemberEvent>(id())->avatarUrl();
    const auto& mediaId = url.authority() + url.path();
    return d->otherAvatars.try_emplace(mediaId, url).first->second;
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
    return avatarObject(room).get(connection(), width, height, [=] {
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

    const auto& oldName = firstMention || !event.prevContent() ? QString()
                          : event.prevContent()->displayName;
    const auto& newName = event.displayName();
    // A hacky way to find out if it's about to change or already changed
    bool isAboutToChange = room->getCurrentState<RoomMemberEvent>(id()) != &event;
    if (newName != oldName) {
        if (isAboutToChange)
            emit nameAboutToChange(newName, oldName, room);
        else {
            emit nameChanged(newName, oldName, room);
        }
    }
    const auto& oldAvatarUrl = firstMention || !event.prevContent() ? QUrl()
                               : event.prevContent()->avatarUrl;
    if (event.avatarUrl() != oldAvatarUrl && !isAboutToChange)
        emit avatarChanged(this, room);
}

qreal User::hueF() const { return d->hueF; }
