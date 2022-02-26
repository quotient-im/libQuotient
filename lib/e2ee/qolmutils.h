// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <QByteArray>

#include "e2ee/e2ee.h"

namespace Quotient {
// Convert PicklingMode to key
QUOTIENT_API QByteArray toKey(const PicklingMode &mode);
QUOTIENT_API QByteArray getRandom(size_t bufferSize);
}
