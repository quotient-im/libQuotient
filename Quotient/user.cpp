// SPDX-FileCopyrightText: 2015 Felix Rohrbach <kde@fxrh.de>
// SPDX-FileCopyrightText: 2016 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "user.h"

#include "avatar.h"
#include "connection.h"
#include "logging_categories_p.h"
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

class Q_DECL_HIDDEN User::Private {
public:
    Private(QString userId) : id(std::move(userId)), hueF(stringToHueF(id)) { }

    QString id;
    qreal hueF;

    QString defaultName;
    QUrl defaultAvatarUrl;
};

User::User(QString userId, Connection* connection)
    : QObject(connection), d(makeImpl<Private>(std::move(userId)))
{
    setObjectName(id());
}

Connection* User::connection() const
{
    Q_ASSERT(parent());
    return static_cast<Connection*>(parent());
}

void User::load()
{
    auto* profileJob =
        connection()->callApi<GetUserProfileJob>(id());
    connect(profileJob, &BaseJob::result, this, [this, profileJob] {
        d->defaultName = profileJob->displayname();
        d->defaultAvatarUrl = profileJob->avatarUrl();
        emit defaultNameChanged();
        emit defaultAvatarChanged();
    });
}

QString User::id() const { return d->id; }

QString User::name(const Room* room) const
{
    return room ? room->member(id()).name() : d->defaultName;
}

QString User::displayname(const Room* room) const
{
    return room ? room->member(id()).disambiguatedName()
                : d->defaultName.isEmpty() ? d->id : d->defaultName;
}

QString User::fullName(const Room* room) const
{
    const auto displayName = name(room);
    return displayName.isEmpty() ? id() : (displayName % " ("_ls % id() % u')');
}

bool User::isGuest() const
{
    Q_ASSERT(!d->id.isEmpty() && d->id.startsWith(u'@'));
    auto it = std::find_if_not(d->id.cbegin() + 1, d->id.cend(),
                               [](QChar c) { return c.isDigit(); });
    Q_ASSERT(it != d->id.cend());
    return *it == u':';
}

int User::hue() const { return int(hueF() * 359); }

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

void User::rename(const QString& newName, Room* r)
{
    if (!r) {
        qCWarning(MAIN) << "Passing a null room to two-argument User::rename()"
                           "is incorrect; client developer, please fix it";
        rename(newName);
        return;
    }
    // #481: take the current state and update it with the new name
    if (const auto& maybeEvt = r->currentState().get<RoomMemberEvent>(id())) {
        auto content = maybeEvt->content();
        if (content.membership == Membership::Join) {
            content.displayName = sanitized(newName);
            r->setState<RoomMemberEvent>(id(), std::move(content));
            // The state will be updated locally after it arrives with sync
            return;
        }
    }
    qCCritical(MEMBERS)
        << "Attempt to rename a non-member in a room context - ignored";
}

template <typename SourceT>
inline bool User::doSetAvatar(SourceT&& source)
{
    return connection()->userAvatar(d->defaultAvatarUrl).upload(
        connection(), source, [this](const QUrl& contentUri) {
            auto* j = connection()->callApi<SetAvatarUrlJob>(id(), contentUri);
            connect(j, &BaseJob::success, this,
                    [this, contentUri] {
                        if (contentUri == d->defaultAvatarUrl) {
                            connection()->userAvatar(d->defaultAvatarUrl).updateUrl(contentUri);
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

void User::ignore() { connection()->addToIgnoredUsers(d->id); }

void User::unmarkIgnore() { connection()->removeFromIgnoredUsers(d->id); }

bool User::isIgnored() const { return connection()->isIgnored(this); }

const Avatar& User::avatarObject(const Room* room) const
{
    if (!room)
        return connection()->userAvatar(d->defaultAvatarUrl);

    return connection()->userAvatar(room->memberAvatarUrl(id()));
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
    QT_IGNORE_DEPRECATIONS(return avatarObject(room).mediaId();)
}

QUrl User::avatarUrl(const Room* room) const
{
    QT_IGNORE_DEPRECATIONS(return avatarObject(room).url();)
}

qreal User::hueF() const { return d->hueF; }
