// SPDX-FileCopyrightText: 2017 Marius Gripsgard <marius@ubports.com>
// SPDX-FileCopyrightText: 2018 Josip Delic <delijati@googlemail.com>
// SPDX-FileCopyrightText: 2018 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-FileCopyrightText: 2020 Carl Schwan <carlschwan@kde.org>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "roomevent.h"

namespace Quotient {
class CallCandidatesEvent : public EventTemplate<CallCandidatesEvent, CallEventBase> {
public:
    QUO_EVENT(CallCandidatesEvent, "m.call.candidates")

    using EventTemplate::EventTemplate;

    explicit CallCandidatesEvent(const QString& callId,
                                 const QJsonArray& candidates)
        : EventTemplate(callId, { { QStringLiteral("candidates"), candidates } })
    {}

    QUO_CONTENT_GETTER(QJsonArray, candidates)
    QUO_CONTENT_GETTER(QString, callId)
    QUO_CONTENT_GETTER(int, version)
};
} // namespace Quotient
