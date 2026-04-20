// SPDX-License-Identifier: LGPL-2.0-or-later
// SPDX-FileCopyrightText: 2025 Tobias Fella <tobias.fella@kde.org>

#pragma once

#include "event.h"

namespace Quotient
{

class QUOTIENT_API PresenceEvent : public Quotient::Event {
public:
    QUO_EVENT(PresenceEvent, "m.presence")

    using Event::Event;

    QUO_CONTENT_GETTER(QString, avatarUrl)
    QUO_CONTENT_GETTER(bool, currentlyActive)
    QUO_CONTENT_GETTER(QString, displayName)
    QUO_CONTENT_GETTER(qint64, lastActiveTime)
    QUO_CONTENT_GETTER(QString, presence)
    QUO_CONTENT_GETTER(QString, statusMsg)
};

}
