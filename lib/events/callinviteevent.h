// SPDX-FileCopyrightText: 2017 Marius Gripsgard <marius@ubports.com>
// SPDX-FileCopyrightText: 2018 Josip Delic <delijati@googlemail.com>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "roomevent.h"

namespace Quotient {
class QUOTIENT_API CallInviteEvent : public CallEventBase {
public:
    QUO_EVENT(CallInviteEvent, "m.call.invite")

    explicit CallInviteEvent(const QJsonObject& obj);

    explicit CallInviteEvent(const QString& callId, int lifetime,
                             const QString& sdp);

    QUO_CONTENT_GETTER(int, lifetime)
    QString sdp() const
    {
        return contentPart<QJsonObject>("offer"_ls).value("sdp"_ls).toString();
    }
};
} // namespace Quotient
