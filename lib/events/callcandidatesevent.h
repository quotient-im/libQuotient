// SPDX-FileCopyrightText: 2017 Marius Gripsgard <marius@ubports.com>
// SPDX-FileCopyrightText: 2018 Josip Delic <delijati@googlemail.com>
// SPDX-FileCopyrightText: 2018 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-FileCopyrightText: 2020 Carl Schwan <carlschwan@kde.org>
// SPDX-License-Identifier: LGPL-2.1-or-later

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

    QUO_CONTENT_GETTER(QJsonArray, candidates)
    QUO_CONTENT_GETTER(QString, callId)
    QUO_CONTENT_GETTER(int, version)
};

REGISTER_EVENT_TYPE(CallCandidatesEvent)
} // namespace Quotient
