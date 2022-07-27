// SPDX-FileCopyrightText: 2019 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "roomevent.h"
#include "eventrelation.h"

namespace Quotient {

DEFINE_SIMPLE_EVENT(ReactionEvent, RoomEvent, "m.reaction", EventRelation,
                    relation, "m.relates_to")

} // namespace Quotient
