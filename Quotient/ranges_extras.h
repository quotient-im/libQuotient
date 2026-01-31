// SPDX-FileCopyrightText: The Quotient Project Contributors
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <algorithm>
#include <ranges>

namespace Quotient {

//! \brief An indexOf() alternative for any range
//!
//! Unlike QList::indexOf(), returns `range.size()` if \p value is not found
template <typename RangeT, typename ValT, typename ProjT = std::identity>
    requires std::indirectly_comparable<std::ranges::iterator_t<RangeT>, const ValT*,
                                        std::ranges::equal_to, ProjT>
[[nodiscard]] constexpr inline auto findIndex(const RangeT& range, const ValT& value,
                                              ProjT proj = {})
{
    using namespace std::ranges;
    return distance(begin(range), find(range, value, std::move(proj)));
}

//! \brief A replacement of std::ranges::to() while toolchains catch up
//!
//! Returns a container of type \p TargetT created from \p sourceRange. Unlike std::ranges::to(),
//! you have to pass the range to it (e.g. `rangeTo<TargetT>(someRange)`); using it in a pipeline
//! (`someRange | rangeTo<TargetT>()`) won't compile. Internally calls std::ranges::to() if it's
//! available; otherwise, returns the result of calling
//! `TargetT(ranges::begin(sourceRange), ranges::end(sourceRange))`.
template <class TargetT, typename SourceT>
[[nodiscard]] constexpr inline auto rangeTo(SourceT&& sourceRange)
{
#if defined(__cpp_lib_ranges_to_container)
    return std::ranges::to<TargetT>(std::forward<SourceT>(sourceRange));
#else
    // Provide the minimal necessary subset of what std::ranges::to() can do
    using namespace std::ranges;
    if constexpr (std::constructible_from<TargetT, SourceT>)
        return TargetT(std::forward<SourceT>(sourceRange));
    else {
        using iter_t = iterator_t<SourceT>;
        using iter_category_t = typename std::iterator_traits<iter_t>::iterator_category;
        if constexpr (requires {
                          requires common_range<SourceT>;
                          typename iter_category_t;
                          requires std::derived_from<iter_category_t, std::input_iterator_tag>;
                          requires std::constructible_from<TargetT, iter_t, sentinel_t<SourceT>>;
                      })
            return TargetT(begin(sourceRange), end(sourceRange));
        else {
            TargetT c{};
            if constexpr (sized_range<SourceT>
                          && requires(range_size_t<TargetT> n) { c.reserve(n); })
                c.reserve(static_cast<range_size_t<TargetT>>(size(sourceRange)));
            using ValT = std::iter_value_t<iter_t>;
            for (auto&& e : sourceRange) {
                if constexpr (requires { c.emplace_back(std::forward<ValT>(e)); })
                    c.emplace_back(std::forward<ValT>(e));
                else if constexpr (requires { c.push_back(std::forward<ValT>(e)); })
                    c.push_back(std::forward<ValT>(e));
                else if constexpr (requires { c.emplace(c.end(), std::forward<ValT>(e)); })
                    c.emplace(c.end(), std::forward<ValT>(e));
                else
                    c.insert(c.end(), std::forward<ValT>(e));
            }
            return c;
        }
    }
#endif
}

//! An overload that accepts unspecialised container template
template <template <typename> class TargetT, typename SourceT>
[[nodiscard]] constexpr inline auto rangeTo(SourceT&& sourceRange)
{
    // Avoid template argument deduction because Xcode still can't do it when TargetT is an alias
#if defined(__cpp_lib_ranges_to_container)
    return std::ranges::to<TargetT<std::ranges::range_value_t<SourceT>>>(
        std::forward<SourceT>(sourceRange));
#else
    return rangeTo<TargetT<std::ranges::range_value_t<SourceT>>>(std::forward<SourceT>(sourceRange));
#endif
}

#ifdef __cpp_lib_ranges_contains
constexpr inline auto rangeContains = std::ranges::contains;
#else
template <typename ProjT = std::identity>
[[nodiscard]] constexpr inline auto rangeContains(const auto& c, const auto& v, ProjT proj = {})
{
    return std::ranges::find(c, v, std::move(proj)) != std::ranges::end(c);
}
#endif

} // namespace Quotient
