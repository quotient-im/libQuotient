// SPDX-FileCopyrightText: 2021 Quotient contributors
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "room.h"

namespace Quotient {

//! \brief Counters of unread events and highlights with a precision flag
//!
//! This structure contains a static snapshot with values of unread counters
//! returned by Room::partiallyReadStats and Room::unreadStats (properties
//! or methods).
//!
//! \note It's just a simple grouping of counters and is not automatically
//! updated from the room as subsequent syncs arrive.
//! \sa Room::unreadStats, Room::partiallyReadStats, Room::isEventNotable
struct EventStats {
    Q_GADGET
    Q_PROPERTY(qsizetype notableCount MEMBER notableCount CONSTANT)
    Q_PROPERTY(qsizetype highlightCount MEMBER highlightCount CONSTANT)
    Q_PROPERTY(bool isEstimate MEMBER isEstimate CONSTANT)
public:
    //! The number of "notable" events in an events range
    //! \sa Room::isEventNotable
    qsizetype notableCount = 0;
    qsizetype highlightCount = 0;
    //! \brief Whether the counter values above are exact
    //!
    //! This is false when the end marker (m.read receipt or m.fully_read) used
    //! to collect the stats points to an event loaded locally and the counters
    //! can therefore be calculated exactly using the locally available segment
    //! of the timeline; true when the marker points to an event outside of
    //! the local timeline (in which case the estimation is made basing on
    //! the data supplied by the homeserver as well as counters saved from
    //! the previous run of the client).
    bool isEstimate = true;

    bool operator==(const EventStats& rhs) const& = default;

    //! \brief Check whether the event statistics are empty
    //!
    //! Empty statistics have notable and highlight counters of zero and
    //! isEstimate set to false.
    Q_INVOKABLE bool empty() const
    {
        return notableCount == 0 && !isEstimate && highlightCount == 0;
    }

    using marker_t = Room::rev_iter_t;

    //! \brief Build event statistics on a range of events
    //!
    //! This is a factory that returns an EventStats instance with counts of
    //! notable and highlighted events between \p from and \p to reverse
    //! timeline iterators; the \p init parameter allows to override
    //! the initial statistics object and start from other values.
    static EventStats fromRange(const Room* room, const marker_t& from,
                                const marker_t& to,
                                const EventStats& init = { 0, 0, false });

    //! \brief Build event statistics on a range from sync edge to marker
    //!
    //! This is mainly a shortcut for \code
    //! <tt>fromRange(room, marker_t(room->syncEdge()), marker)</tt>
    //! \endcode except that it also sets isEstimate to true if (and only if)
    //! <tt>to == room->historyEdge()</tt>.
    static EventStats fromMarker(const Room* room, const marker_t& marker);

    //! \brief Loads a statistics object from the cached counters
    //!
    //! Sets isEstimate to `true` unless both notableCount and highlightCount
    //! are equal to -1.
    static EventStats fromCachedCounters(Omittable<int> notableCount,
                                         Omittable<int> highlightCount = none);

    //! \brief Update statistics when a read marker moves down the timeline
    //!
    //! Removes events between oldMarker and newMarker from statistics
    //! calculation if \p oldMarker points to an existing event in the timeline,
    //! or recalculates the statistics entirely if \p oldMarker points
    //! to <tt>room->historyEdge()</tt>. Always results in exact statistics
    //! (<tt>isEstimate == false</tt>.
    //! \param oldMarker Must point correspond to the _current_ statistics
    //!        isEstimate state, i.e. it should point to
    //!        <tt>room->historyEdge()</tt> if <tt>isEstimate == true</tt>, or
    //!        to a valid position within the timeline otherwise
    //! \param newMarker Must point to a valid position in the timeline (not to
    //!        <tt>room->historyEdge()</tt> that is equal to or closer to
    //!        the sync edge than \p oldMarker
    //! \return true if either notableCount or highlightCount changed, or if
    //!         the statistics was completely recalculated; false otherwise
    bool updateOnMarkerMove(const Room* room, const marker_t& oldMarker,
                            const marker_t& newMarker);

    //! \brief Validate the statistics object against the given marker
    //!
    //! Checks whether the statistics object data are valid for a given marker.
    //! No stats recalculation takes place, only isEstimate and zero-ness
    //! of notableCount are checked.
    bool isValidFor(const Room* room, const marker_t& marker) const;
};

QDebug operator<<(QDebug dbg, const EventStats& es);

}
