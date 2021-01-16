// SPDX-FileCopyrightText: 2017 Marius Gripsgard <marius@ubports.com>
// SPDX-FileCopyrightText: 2018 Josip Delic <delijati@googlemail.com>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "roomevent.h"

namespace Quotient {
class CallHangupEvent : public CallEventBase {
public:
    DEFINE_EVENT_TYPEID("m.call.hangup", CallHangupEvent)

    explicit CallHangupEvent(const QJsonObject& obj);
    explicit CallHangupEvent(const QString& callId);
};

REGISTER_EVENT_TYPE(CallHangupEvent)
} // namespace Quotient
