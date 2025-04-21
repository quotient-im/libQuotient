// SPDX-FileCopyrightText: The Quotient Project Contributors
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <ranges>

namespace Quotient {

//! \brief An indexOf() alternative for any range
//!
//! Unlike QList::indexOf(), returns `range.size()` if \p value is not found
template <typename RangeT, typename ValT, typename ProjT = std::identity>
    requires std::indirectly_comparable<std::ranges::iterator_t<RangeT>, const ValT*,
                                        std::ranges::equal_to, ProjT>
inline auto findIndex(const RangeT& range, const ValT& value, ProjT proj = {})
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
[[nodiscard]] constexpr auto rangeTo(SourceT&& sourceRange)
{
#if defined(__cpp_lib_ranges_to_container)
    return std::ranges::to<TargetT>(std::forward<SourceT>(sourceRange));
#else
    using std::begin, std::end;
    return TargetT(begin(sourceRange), end(sourceRange));
#endif
}

//! An overload that accepts unspecialised container template
template <template <typename> class TargetT, typename SourceT>
[[nodiscard]] constexpr auto rangeTo(SourceT&& sourceRange)
{
    // Avoid template argument deduction because Xcode still can't do it when TargetT is an alias
#if defined(__cpp_lib_ranges_to_container)
    return std::ranges::to<TargetT<std::ranges::range_value_t<SourceT>>>(
        std::forward<SourceT>(sourceRange));
#else
    using std::begin, std::end;
    return TargetT<std::ranges::range_value_t<SourceT>>(begin(sourceRange), end(sourceRange));
#endif
}

#ifdef __cpp_lib_ranges_contains
constexpr auto rangeContains = std::ranges::contains;
#else
template <typename RangeT, typename ValT, typename ProjT = std::identity>
    requires std::indirect_binary_predicate<
        std::ranges::equal_to, std::projected<std::ranges::iterator_t<RangeT>, ProjT>, const ValT*>
[[nodiscard]] constexpr auto rangeContains(const RangeT& r, const ValT& v, ProjT proj = {})
{
    return std::ranges::find(r, v, std::move(proj)) != std::ranges::end(r);
}
#endif

}
