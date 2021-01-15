/******************************************************************************
 * SPDX-FileCopyrightText: 2017 Marius Gripsgard <marius@ubports.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include "roomevent.h"

namespace Quotient {
class CallAnswerEvent : public CallEventBase {
public:
    DEFINE_EVENT_TYPEID("m.call.answer", CallAnswerEvent)

    explicit CallAnswerEvent(const QJsonObject& obj);

    explicit CallAnswerEvent(const QString& callId, const int lifetime,
                             const QString& sdp);
    explicit CallAnswerEvent(const QString& callId, const QString& sdp);

    int lifetime() const
    {
        return content<int>("lifetime"_ls);
    } // FIXME: Omittable<>?
    QString sdp() const
    {
        return contentJson()["answer"_ls].toObject().value("sdp"_ls).toString();
    }
};

REGISTER_EVENT_TYPE(CallAnswerEvent)
} // namespace Quotient
