// SPDX-FileCopyrightText: 2017 Marius Gripsgard <marius@ubports.com>
// SPDX-FileCopyrightText: 2018 Josip Delic <delijati@googlemail.com>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "roomevent.h"

namespace Quotient {
class QUOTIENT_API CallHangupEvent : public CallEventBase {
public:
    QUO_EVENT(CallHangupEvent, "m.call.hangup")

    explicit CallHangupEvent(const QJsonObject& obj)
        : CallEventBase(typeId(), obj)
    {}
    explicit CallHangupEvent(const QString& callId)
        : CallEventBase(typeId(), matrixTypeId(), callId, 0)
    {}
};
} // namespace Quotient
