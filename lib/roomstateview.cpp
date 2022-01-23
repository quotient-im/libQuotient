// SPDX-FileCopyrightText: 2021 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "roomstateview.h"

using namespace Quotient;

const StateEventBase* RoomStateView::get(const QString& evtType,
                                         const QString& stateKey) const
{
    return value({ evtType, stateKey });
}

bool RoomStateView::contains(const QString& evtType,
                             const QString& stateKey) const
{
    return contains({ evtType, stateKey });
}

QJsonObject RoomStateView::contentJson(const QString& evtType,
                                       const QString& stateKey) const
{
    return queryOr(evtType, stateKey, &Event::contentJson, QJsonObject());
}

const QVector<const StateEventBase*>
RoomStateView::eventsOfType(const QString& evtType) const
{
    auto vals = QVector<const StateEventBase*>();
    for (auto it = cbegin(); it != cend(); ++it)
        if (it.key().first == evtType)
            vals.append(it.value());

    return vals;
}
