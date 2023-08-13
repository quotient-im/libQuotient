// SPDX-FileCopyrightText: 2023 James Graham <james.h.graham@protonmail.com>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "roommember.h"

#include "room.h"
#include "events/roommemberevent.h"

#include <QGuiApplication>
#include <QPalette>

using namespace Quotient;

class Q_DECL_HIDDEN RoomMember::Private {
public:
    Private(const RoomMemberEvent* member, const Room* room) : member(member), room(room), hueF(stringToHueF(member->userId())) { }

    const RoomMemberEvent* member;
    const Room* room;

    qreal hueF;
};

RoomMember::RoomMember(const RoomMemberEvent* member, const Room* room)
    : d(makeImpl<Private>(member, room))
{
}

QString RoomMember::id() const { return d->member->userId(); }

QString RoomMember::displayName() const
{
    // See https://github.com/matrix-org/matrix-doc/issues/1375
    if (d->member) {
        if (d->member->newDisplayName())
            return *d->member->newDisplayName();
        if (d->member->prevContent() && d->member->prevContent()->displayName)
            return *d->member->prevContent()->displayName;
        return d->member->userId();
    }
    return {};
}

QString RoomMember::fullName() const { return displayName() % " ("_ls % id() % u')'; }

int RoomMember::hue() const { return int(hueF() * 359); }

qreal RoomMember::hueF() const { return d->hueF; }

QColor RoomMember::color() const
{
    const auto lightness = QGuiApplication::palette().color(QPalette::Active, QPalette::Window).lightnessF();
    // https://github.com/quotient-im/libQuotient/wiki/User-color-coding-standard-draft-proposal
    return QColor::fromHslF(hueF(), 1, -0.7 * lightness + 0.9, 1);
}

QUrl RoomMember::avatarUrl() const {
    // See https://github.com/matrix-org/matrix-doc/issues/1375
    QUrl baseUrl;
    if (d->member->newAvatarUrl()) {
        baseUrl = *d->member->newAvatarUrl();
    }
    if (d->member->prevContent() && d->member->prevContent()->avatarUrl) {
        baseUrl = *d->member->prevContent()->avatarUrl;
    }
    if (baseUrl.isEmpty() || baseUrl.scheme() != "mxc"_ls) {
        return {};
    }

    const auto mediaUrl = d->room->connection()->makeMediaUrl(baseUrl);
    if (mediaUrl.isValid() && mediaUrl.scheme() == "mxc"_ls) {
        return mediaUrl;
    }
    return {};
}
