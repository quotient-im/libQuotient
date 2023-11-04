// SPDX-FileCopyrightText: 2023 James Graham <james.h.graham@protonmail.com>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "roommember.h"

#include "events/roommemberevent.h"
#include "room.h"
#include "util.h"

#include <QGuiApplication>
#include <QPalette>

using namespace Quotient;

RoomMember::RoomMember(const Room* room, const RoomMemberEvent* member)
    : _room(room)
    , _member(member)
    , _hueF(_member == nullptr ? 0.0 : stringToHueF(member->userId()))
{
}

bool RoomMember::operator==(const RoomMember& other) const
{
    if (id().isEmpty() || other.id().isEmpty()) {
        return false;
    }
    return id() == other.id();
}

QString RoomMember::id() const {
    if (_member == nullptr) {
        return {};
    }
    return _member->userId();
}

bool RoomMember::isLocalMember() const { return id() == _room->localMember().id(); }

Membership RoomMember::membershipState() const {
    if (_member == nullptr) {
        return Membership::Undefined;
    }
    return _member->membership();
}

QString RoomMember::name() const
{
    if (_member == nullptr) {
        return {};
    }
    // See https://github.com/matrix-org/matrix-doc/issues/1375
    if (_member->newDisplayName())
        return sanitized(*_member->newDisplayName());
    if (_member->prevContent() && _member->prevContent()->displayName)
        return sanitized(*_member->prevContent()->displayName);
    return {};
}

QString RoomMember::displayName() const { return name().isEmpty() ? id() : name(); }

QString RoomMember::htmlSafeDisplayName() const { return displayName().toHtmlEscaped(); }

QString RoomMember::fullName() const {
    if (name().isEmpty()) {
        return id();
    }
    return displayName() % " ("_ls % id() % u')';
}

QString RoomMember::htmlSafeFullName() const { return fullName().toHtmlEscaped(); }

int RoomMember::hue() const { return static_cast<int>(hueF() * 359); }

qreal RoomMember::hueF() const { return _hueF; }

QColor RoomMember::color() const
{
    const auto lightness = QGuiApplication::palette().color(QPalette::Active, QPalette::Window).lightnessF();
    // https://github.com/quotient-im/libQuotient/wiki/User-color-coding-standard-draft-proposal
    return QColor::fromHslF(static_cast<float>(hueF()), 1.0f, -0.7f * lightness + 0.9f, 1.0f);
}

QString RoomMember::avatarMediaId() const
{
    if (_member == nullptr) {
        return {};
    }
    // See https://github.com/matrix-org/matrix-doc/issues/1375
    QUrl baseUrl;
    if (_member->newAvatarUrl()) {
        baseUrl = *_member->newAvatarUrl();
    }
    if (_member->prevContent() && _member->prevContent()->avatarUrl) {
        baseUrl = *_member->prevContent()->avatarUrl;
    }
    if (baseUrl.isEmpty() || baseUrl.scheme() != "mxc"_ls) {
        return {};
    }
    return baseUrl.toString();
}

QUrl RoomMember::avatarUrl() const {
    if (_member == nullptr) {
        return {};
    }
    // See https://github.com/matrix-org/matrix-doc/issues/1375
    QUrl baseUrl;
    if (_member->newAvatarUrl()) {
        baseUrl = *_member->newAvatarUrl();
    }
    if (_member->prevContent() && _member->prevContent()->avatarUrl) {
        baseUrl = *_member->prevContent()->avatarUrl;
    }
    if (baseUrl.isEmpty() || baseUrl.scheme() != "mxc"_ls) {
        return {};
    }

    const auto mediaUrl = _room->connection()->makeMediaUrl(baseUrl);
    if (mediaUrl.isValid() && mediaUrl.scheme() == "mxc"_ls) {
        return mediaUrl;
    }
    return {};
}
