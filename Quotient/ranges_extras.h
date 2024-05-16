#pragma once

#include <ranges>

namespace Quotient {

//! Same as std::projected but Proj is checked against the reference under the iterator
template <std::indirectly_readable IterT,
          std::indirectly_regular_unary_invocable<std::iter_reference_t<IterT>> Proj>
using IndirectlyProjected = std::projected<std::iter_reference_t<IterT>, Proj>;

//! \brief Find a value in a container of (smart) pointers
//!
//! This is a replica of std::ranges::find that automatically applies dereferencing projection
//! before applying the provided projection. Quotient has a few containers with pointers or wrappers
//! for other types - think of timeline items or `event_ptr_tt<>`s. This is meant to streamline
//! searching for events that match a specific simple criterion; e.g., to find an event with a given
//! id in a container you can now write `findIndirect(events, eventId, &RoomEvent::id);` instead
//! of having to supply your own lambda to dereference the timeline item and check the event id.
template <std::input_iterator IterT, typename ValT, typename Proj = std::identity>
    requires std::indirect_binary_predicate<std::ranges::equal_to,
                                            IndirectlyProjected<IterT, Proj>, const ValT*>
// Most of constraints here (including IndirectlyProjected) are based on the definition of
// std::ranges::find and things around it
inline constexpr auto findIndirect(IterT from, IterT to, const ValT& value, Proj proj = {})
{
    return std::ranges::find(from, to, value, [p = std::move(proj)](auto& itemPtr) {
        return std::invoke(p, *itemPtr);
    });
}

//! The overload of findIndirect for ranges
template <typename RangeT, typename ValT, typename Proj = std::identity>
    requires std::indirect_binary_predicate<
        std::ranges::equal_to, IndirectlyProjected<std::ranges::iterator_t<RangeT>, Proj>,
        const ValT*>
inline constexpr auto findIndirect(RangeT&& range, const ValT& value, Proj proj = {})
{
    return findIndirect(std::ranges::begin(range), std::ranges::end(range), value, std::move(proj));
}

}
