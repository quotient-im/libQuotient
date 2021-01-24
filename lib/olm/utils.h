// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once
#ifdef Quotient_E2EE_ENABLED

#include "olm/e2ee.h"

namespace Quotient {
// Convert PicklingMode to key
QByteArray toKey(const PicklingMode &mode);
QByteArray getRandom(size_t bufferSize);
}
#endif
