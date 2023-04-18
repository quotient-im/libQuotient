// SPDX-FileCopyrightText: 2022 Carl Schwan <carl@carlschwan.eu>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "joinrulesevent.h"

using namespace Quotient;

QString JoinRulesEvent::joinRule() const
{
    return fromJson<QString>(contentJson()["join_rule"_ls]);
}

QJsonArray JoinRulesEvent::allow() const
{
    return contentJson()["allow"_ls].toArray();
}
