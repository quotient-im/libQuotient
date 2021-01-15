/******************************************************************************
 * SPDX-FileCopyrightText: 2017 Marius Gripsgard <marius@ubports.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include "roomevent.h"

namespace Quotient {
class CallCandidatesEvent : public CallEventBase {
public:
    DEFINE_EVENT_TYPEID("m.call.candidates", CallCandidatesEvent)

    explicit CallCandidatesEvent(const QJsonObject& obj)
        : CallEventBase(typeId(), obj)
    {}

    explicit CallCandidatesEvent(const QString& callId,
                                 const QJsonArray& candidates)
        : CallEventBase(typeId(), matrixTypeId(), callId, 0,
                        { { QStringLiteral("candidates"), candidates } })
    {}

    QJsonArray candidates() const
    {
        return content<QJsonArray>("candidates"_ls);
    }
};

REGISTER_EVENT_TYPE(CallCandidatesEvent)
} // namespace Quotient
