// SPDX-FileCopyrightText: 2017 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "event.h"

namespace Quotient {
DEFINE_SIMPLE_EVENT(TypingEvent, Event, "m.typing", QStringList, users, "user_ids")
} // namespace Quotient
