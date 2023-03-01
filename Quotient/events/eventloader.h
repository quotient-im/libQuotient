// SPDX-FileCopyrightText: 2018 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "stateevent.h"

namespace Quotient {
struct [[deprecated(
    "This header is obsolete since libQuotient 0.7; include a header with"
    " the respective event type definition instead")]] EventLoaderH;
StateEventPtr eventLoaderH(EventLoaderH&);
}
