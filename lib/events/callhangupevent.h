// SPDX-FileCopyrightText: 2017 Marius Gripsgard <marius@ubports.com>
// SPDX-FileCopyrightText: 2018 Josip Delic <delijati@googlemail.com>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "roomevent.h"

namespace Quotient {
class QUOTIENT_API CallHangupEvent
    : public EventTemplate<CallHangupEvent, CallEventBase> {
public:
    QUO_EVENT(CallHangupEvent, "m.call.hangup")
    using EventTemplate::EventTemplate;
};
//REGISTER_EVENT_TYPE(CallHangupEvent)
} // namespace Quotient
