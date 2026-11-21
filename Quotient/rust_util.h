// SPDX-FileCopyrightText: The Quotient Project Contributors
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include "quotient_rust_cxxbridge/lib.h"
#include "util.h"

#include <QtCore/QJsonObject>

namespace Quotient {

inline QUOTIENT_API QByteArray bytesFromRust(const auto &bytes)
{
    return {std::bit_cast<const char *>(bytes.data()), std::ssize(bytes)};
}

inline QUOTIENT_API QString stringFromRust(const rust::String& string)
{
    return QString::fromUtf8(bytesFromRust(string));
}

QUOTIENT_API QJsonObject jsonFromRust(const rust::String& string);

//! \brief Loads a C++ object from its JSON representation stored in a Rust string
//! \note JsonConverter or JsonObjectConverter specialisation for \p PodT has to be defined prior to
//!       calling this function.
template <typename PodT>
inline QUOTIENT_API auto fromRustJson(const rust::String& s)
{
    return fromJson<PodT>(jsonFromRust(s));
}

inline QUOTIENT_API rust::String bytesToRust(const QByteArray& bytes)
{
    return rust::String(bytes.data(), unsignedSize(bytes));
}

inline QUOTIENT_API rust::String stringToRust(const QString& string)
{
    return bytesToRust(string.toUtf8());
}

QUOTIENT_API rust::Vec<rust::String> stringsToRust(const QStringList& strings);

template <typename OverrideSliceValueT = void, typename R>
inline QUOTIENT_API auto toRustSlice(const R &r)
    requires std::ranges::contiguous_range<const R &>
{
    // The = void hack is needed because we cannot take R to the first place without losing ability
    // to override SliceValueT at invocation point, and we cannot use std::ranges::range_value_t<R>
    // as default SliceValueT before R is defined
    using SliceValueT = std::conditional_t<std::is_void_v<OverrideSliceValueT>,
                                           std::ranges::range_value_t<R>, OverrideSliceValueT>;
    static_assert(sizeof(SliceValueT) == sizeof(std::ranges::range_value_t<R>)
                  && sizeof(unsignedSize(r))
                         <= sizeof(std::ranges::range_size_t<rust::Slice<SliceValueT>>));
    return rust::Slice<SliceValueT>(std::bit_cast<SliceValueT *>(std::data(r)), unsignedSize(r));
}

} // namespace Quotient
