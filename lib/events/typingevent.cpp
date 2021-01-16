// SPDX-FileCopyrightText: 2017 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "typingevent.h"

using namespace Quotient;

QStringList TypingEvent::users() const
{
    return fromJson<QStringList>(contentJson()["user_ids"_ls]);
}
