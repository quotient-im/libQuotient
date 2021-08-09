// SPDX-FileCopyrightText: 2015 Felix Rohrbach <kde@fxrh.de>
// SPDX-FileCopyrightText: 2016 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

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

    QString defaultName;
    Avatar defaultAvatar;
    // NB: This container is ever-growing. Even if the user no more scrolls
    // the timeline that far back, historical avatars are still kept around.
    // This is consistent with the rest of Quotient, as room timelines
    // are never vacuumed either. This will probably change in the future.
    /// Map of mediaId to Avatar objects
    static UnorderedMap<QString, Avatar> otherAvatars;
};

decltype(User::Private::otherAvatars) User::Private::otherAvatars {};

User::User(QString userId, Connection* connection)
    : QObject(connection), d(new Private(move(userId)))
{
    setObjectName(id());
    if (connection->userId() == id()) {
        // Load profile information for local user.
        load();
    }
}

Connection* User::connection() const
{
    Q_ASSERT(parent());
    return static_cast<Connection*>(parent());
}

User::~User() = default;

void User::load()
{
    auto* profileJob =
        connection()->callApi<GetUserProfileJob>(QUrl::toPercentEncoding(id()));
    connect(profileJob, &BaseJob::result, this, [this, profileJob] {
        d->defaultName = profileJob->displayname();
        d->defaultAvatar = Avatar(QUrl(profileJob->avatarUrl()));
        emit defaultNameChanged();
        emit defaultAvatarChanged();
    });
}

QString User::id() const { return d->id; }

bool User::isGuest() const
{
    Q_ASSERT(!d->id.isEmpty() && d->id.startsWith('@'));
    auto it = std::find_if_not(d->id.cbegin() + 1, d->id.cend(),
                               [](QChar c) { return c.isDigit(); });
    Q_ASSERT(it != d->id.end());
    return *it == ':';
}

int User::hue() const { return int(hueF() * 359); }

QString User::name(const Room* room) const
{
    return room ? room->memberName(id()) : d->defaultName;
}

QString User::rawName(const Room* room) const { return name(room); }

void User::rename(const QString& newName)
{
    const auto actualNewName = sanitized(newName);
    if (actualNewName == d->defaultName)
        return; // Nothing to do

    connect(connection()->callApi<SetDisplayNameJob>(id(), actualNewName),
            &BaseJob::success, this, [this, actualNewName] {
                // Check again, it could have changed meanwhile
                if (actualNewName != d->defaultName) {
                    d->defaultName = actualNewName;
                    emit defaultNameChanged();
                } else
                    qCWarning(MAIN)
                        << "User" << id() << "already has profile name set to"
                        << actualNewName;
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
    // #481: take the current state and update it with the new name
    auto evtC = r->getCurrentState<RoomMemberEvent>(id())->content();
    Q_ASSERT_X(evtC.membership == Membership::Join, __FUNCTION__,
               "Attempt to rename a user that's not a room member");
    evtC.displayName = sanitized(newName);
    r->setState<RoomMemberEvent>(id(), move(evtC));
    // The state will be updated locally after it arrives with sync
}

template <typename SourceT>
inline bool User::doSetAvatar(SourceT&& source)
{
    return d->defaultAvatar.upload(
        connection(), source, [this](const QUrl& contentUri) {
            auto* j = connection()->callApi<SetAvatarUrlJob>(id(), contentUri);
            connect(j, &BaseJob::success, this,
                    [this, contentUri] {
                        if (contentUri == d->defaultAvatar.url()) {
                            d->defaultAvatar.updateUrl(contentUri);
                            emit defaultAvatarChanged();
                        } else
                            qCWarning(MAIN) << "User" << id()
                                            << "already has avatar URL set to"
                                            << contentUri.toDisplayString();
                    });
        });
}

bool User::setAvatar(const QString& fileName)
{
    return doSetAvatar(fileName);
}

bool User::setAvatar(QIODevice* source)
{
    return doSetAvatar(source);
}

void User::removeAvatar()
{
    connection()->callApi<SetAvatarUrlJob>(id(), QUrl());
}

void User::requestDirectChat() { connection()->requestDirectChat(this); }

void User::ignore() { connection()->addToIgnoredUsers(this); }

void User::unmarkIgnore() { connection()->removeFromIgnoredUsers(this); }

bool User::isIgnored() const { return connection()->isIgnored(this); }

QString User::displayname(const Room* room) const
{
    return room ? room->safeMemberName(id())
                : d->defaultName.isEmpty() ? d->id : d->defaultName;
}

QString User::fullName(const Room* room) const
{
    const auto displayName = name(room);
    return displayName.isEmpty() ? id() : (displayName % " (" % id() % ')');
}

QString User::bridged() const { return {}; }

const Avatar& User::avatarObject(const Room* room) const
{
    if (!room)
        return d->defaultAvatar;

    const auto& url = room->memberAvatarUrl(id());
    const auto& mediaId = url.authority() + url.path();
    return d->otherAvatars.try_emplace(mediaId, url).first->second;
}

QImage User::avatar(int dimension, const Room* room) const
{
    return avatar(dimension, dimension, room);
}

QImage User::avatar(int width, int height, const Room* room) const
{
    return avatar(width, height, room, [] {});
}

QImage User::avatar(int width, int height, const Room* room,
                    const Avatar::get_callback_t& callback) const
{
    return avatarObject(room).get(connection(), width, height, callback);
}

QString User::avatarMediaId(const Room* room) const
{
    return avatarObject(room).mediaId();
}

QUrl User::avatarUrl(const Room* room) const
{
    return avatarObject(room).url();
}

qreal User::hueF() const { return d->hueF; }
