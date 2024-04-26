// SPDX-FileCopyrightText: 2018 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <optional>

#include "util.h" // For backwards compatibility with code using lift() and merge()

namespace Quotient {

template <typename T>
using Omittable [[deprecated("Use std::optional<> instead")]] = std::optional<T>;

[[deprecated("Use std::nullopt instead")]]
constexpr auto none = std::nullopt;

} // namespace Quotient
