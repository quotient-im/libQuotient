// SPDX-FileCopyrightText: 2023 James Graham <james.h.graham@protonmail.com>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "userincontext.h"

#include "avatar.h"
#include "room.h"
#include "user.h"

using namespace Quotient;

class Q_DECL_HIDDEN UserInContext::Private {
public:
    Private(const User* user, const Room* room) : user(user), room(room) { }

    const User* user;
    const Room* room;
};

UserInContext::UserInContext(const User* user, const Room* room)
    : QObject(nullptr), d(makeImpl<Private>(user, room))
{
    Q_ASSERT(d->user);
    Q_ASSERT(d->room);
    setObjectName(d->user->objectName());
}

QString UserInContext::id() const { return d->user->id(); }

QString UserInContext::name() const { return d->user->name(d->room); }

QString UserInContext::displayname() const { return d->user->displayname(d->room); }

QString UserInContext::fullName() const { return d->user->fullName(d->room); }

bool UserInContext::isGuest() const { return d->user->isGuest(); }

int UserInContext::hue() const { return d->user->hue(); }

qreal UserInContext::hueF() const { return d->user->hueF(); }

QColor UserInContext::color() const { return d->user->color(); }

const Avatar& UserInContext::avatarObject() const { return d->user->avatarObject(d->room); }

QImage UserInContext::avatar(int dimension) const { return d->user->avatar(dimension, d->room); }

QImage UserInContext::avatar(int requestedWidth, int requestedHeight) const { return d->user->avatar(requestedWidth, requestedHeight, d->room); }

QImage UserInContext::avatar(int width, int height, const Avatar::get_callback_t& callback) const { return d->user->avatar(width, height, d->room, callback); }

QString UserInContext::avatarMediaId() const { return d->user->avatarMediaId(d->room); }

QUrl UserInContext::avatarUrl() const {
    const auto baseUrl = d->user->avatarUrl(d->room);
    if (baseUrl.isEmpty()) {
        return {};
    }

    const auto mediaUrl = d->user->connection()->makeMediaUrl(baseUrl);
    if (mediaUrl.isValid() && mediaUrl.scheme() == QStringLiteral("mxc")) {
        return mediaUrl;
    }
    return {};
}

bool UserInContext::isIgnored() const { return d->user->isIgnored(); }
