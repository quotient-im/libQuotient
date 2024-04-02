// SPDX-FileCopyrightText: 2024 James Graham <james.h.graham@protonmail.com>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "serveraclevent.h"

using namespace Quotient;

QStringList ServerAclEvent::allow() const
{
    return contentPart<QStringList>("allow"_ls);
}

bool ServerAclEvent::allowIpLiterals() const
{
    return contentPart<bool>("allow_ip_literals"_ls);
}

QStringList ServerAclEvent::deny() const
{
    return contentPart<QStringList>("deny"_ls);
}
