// SPDX-FileCopyrightText: 2017 Marius Gripsgard <marius@ubports.com>
// SPDX-FileCopyrightText: 2018 Josip Delic <delijati@googlemail.com>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "roomevent.h"

namespace Quotient {
class CallInviteEvent : public CallEventBase {
public:
    DEFINE_EVENT_TYPEID("m.call.invite", CallInviteEvent)

    explicit CallInviteEvent(const QJsonObject& obj);

    explicit CallInviteEvent(const QString& callId, const int lifetime,
                             const QString& sdp);

    int lifetime() const
    {
        return contentPart<int>("lifetime"_ls);
    } // FIXME: Omittable<>?
    QString sdp() const
    {
        return contentPart<QJsonObject>("offer"_ls).value("sdp"_ls).toString();
    }
};

REGISTER_EVENT_TYPE(CallInviteEvent)
} // namespace Quotient
