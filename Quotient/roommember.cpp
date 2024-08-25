// SPDX-FileCopyrightText: 2023 James Graham <james.h.graham@protonmail.com>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "roommember.h"

#include "events/roommemberevent.h"
#include "room.h"
#include "util.h"

#include <QtGui/QGuiApplication>
#include <QtGui/QPalette>

using namespace Quotient;

RoomMember::RoomMember(const Room* room, const RoomMemberEvent* member)
    : _room(room)
    , _member(member)
    , _hueF(member == nullptr ? 0.0 : stringToHueF(member->userId()))
{
}

bool RoomMember::operator==(const RoomMember& other) const { return id() == other.id(); }

QString RoomMember::id() const
{
    if (_member == nullptr) {
        return {};
    }
    return _member->userId();
}

Uri RoomMember::uri() const { return Uri(id().toLatin1()); }

bool RoomMember::isLocalMember() const
{
    if (_room == nullptr) {
        return false;
    }
    return id() == _room->localMember().id();
}

Membership RoomMember::membershipState() const
{
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
    return name() % u" (" % id() % u')';
}

QString RoomMember::htmlSafeFullName() const { return fullName().toHtmlEscaped(); }

QString RoomMember::disambiguatedName() const { return _room->needsDisambiguation(id()) ? fullName() : displayName(); }

QString RoomMember::htmlSafeDisambiguatedName() const { return disambiguatedName().toHtmlEscaped(); }

bool RoomMember::matches(QStringView substr, Qt::CaseSensitivity cs) const
{
    return name().contains(substr, cs) || id().contains(substr, cs);
}

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
    } else if (_member->prevContent() && _member->prevContent()->avatarUrl) {
        baseUrl = *_member->prevContent()->avatarUrl;
    }
    if (baseUrl.isEmpty() || baseUrl.scheme() != "mxc"_L1) {
        return {};
    }
    return baseUrl.toString();
}

QUrl RoomMember::avatarUrl() const {
    if (_room == nullptr || _member == nullptr) {
        return {};
    }
    // See https://github.com/matrix-org/matrix-doc/issues/1375
    QUrl baseUrl;
    if (_member->newAvatarUrl()) {
        baseUrl = *_member->newAvatarUrl();
    } else if (_member->prevContent() && _member->prevContent()->avatarUrl) {
        baseUrl = *_member->prevContent()->avatarUrl;
    }
    if (baseUrl.isEmpty() || baseUrl.scheme() != "mxc"_L1) {
        return {};
    }

    const auto mediaUrl = _room->connection()->makeMediaUrl(baseUrl);
    if (mediaUrl.isValid() && mediaUrl.scheme() == "mxc"_L1) {
        return mediaUrl;
    }
    return {};
}

int RoomMember::powerLevel() const
{
    if (_room == nullptr || _member == nullptr) {
        return std::numeric_limits<int>::min();
    }
    return _room->memberEffectivePowerLevel(id());
}

namespace {
inline QStringView removeLeadingAt(QStringView sv) { return sv.mid(sv.startsWith(u'@') ? 1 : 0); }
}

bool MemberSorter::operator()(QStringView u1name, QStringView u2name) const
{
    return removeLeadingAt(u1name).localeAwareCompare(removeLeadingAt(u2name)) < 0;
}
