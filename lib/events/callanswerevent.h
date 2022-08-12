// SPDX-FileCopyrightText: 2017 Marius Gripsgard <marius@ubports.com>
// SPDX-FileCopyrightText: 2018 Josip Delic <delijati@googlemail.com>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "roomevent.h"

namespace Quotient {
class QUOTIENT_API CallAnswerEvent
    : public EventTemplate<CallAnswerEvent, CallEventBase> {
public:
    QUO_EVENT(CallAnswerEvent, "m.call.answer")

    using EventTemplate::EventTemplate;

    explicit CallAnswerEvent(const QString& callId, const QString& sdp);

    QString sdp() const
    {
        return contentPart<QJsonObject>("answer"_ls).value("sdp"_ls).toString();
    }
};
} // namespace Quotient
