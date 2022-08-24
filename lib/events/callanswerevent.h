// SPDX-FileCopyrightText: 2017 Marius Gripsgard <marius@ubports.com>
// SPDX-FileCopyrightText: 2018 Josip Delic <delijati@googlemail.com>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "roomevent.h"

namespace Quotient {
class QUOTIENT_API CallAnswerEvent : public CallEventBase {
public:
    DEFINE_EVENT_TYPEID("m.call.answer", CallAnswerEvent)

    explicit CallAnswerEvent(const QJsonObject& obj);

    explicit CallAnswerEvent(const QString& callId, const QString& sdp);

    QString sdp() const
    {
        return contentPart<QJsonObject>("answer"_ls).value("sdp"_ls).toString();
    }
};
REGISTER_EVENT_TYPE(CallAnswerEvent)
} // namespace Quotient
