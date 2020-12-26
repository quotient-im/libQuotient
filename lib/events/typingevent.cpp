/******************************************************************************
 * SPDX-FileCopyrightText: 2015 Felix Rohrbach <kde@fxrh.de>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "typingevent.h"

using namespace Quotient;

QStringList TypingEvent::users() const
{
    return fromJson<QStringList>(contentJson()["user_ids"_ls]);
}
