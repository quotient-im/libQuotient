// SPDX-FileCopyrightText: 2021 Quotient contributors
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "eventstats.h"

#include "logging_categories_p.h"

using namespace Quotient;

EventStats EventStats::fromRange(const Room* room, const Room::rev_iter_t& from,
                                 const Room::rev_iter_t& to,
                                 const EventStats& init)
{
    Q_ASSERT(to <= room->historyEdge());
    Q_ASSERT(from >= Room::rev_iter_t(room->syncEdge()));
    Q_ASSERT(from <= to);
    QElapsedTimer et;
    et.start();
    const auto result =
        accumulate(from, to, init,
                   [room](EventStats acc, const TimelineItem& ti) {
                       acc.notableCount += room->isEventNotable(ti);
                       acc.highlightCount += room->notificationFor(ti).type
                                             == Notification::Highlight;
                       return acc;
                   });
    if (et.nsecsElapsed() > ProfilerMinNsecs / 10)
        qCDebug(PROFILER).nospace()
            << "Event statistics collection over index range [" << from->index()
            << "," << (to - 1)->index() << "] took " << et;
    return result;
}

EventStats EventStats::fromMarker(const Room* room,
                                  const EventStats::marker_t& marker)
{
    const auto s = fromRange(room, marker_t(room->syncEdge()), marker,
                             { 0, 0, marker == room->historyEdge() });
    Q_ASSERT(s.isValidFor(room, marker));
    return s;
}

EventStats EventStats::fromCachedCounters(Omittable<int> notableCount,
                                          Omittable<int> highlightCount)
{
    const auto hCount = std::max(0, highlightCount.value_or(0));
    if (!notableCount.has_value())
        return { 0, hCount, true };
    auto nCount = notableCount.value_or(0);
    return { std::max(0, nCount), hCount, nCount != -1 };
}

bool EventStats::updateOnMarkerMove(const Room* room, const marker_t& oldMarker,
                                    const marker_t& newMarker)
{
    if (newMarker == oldMarker)
        return false;

    // Double-check consistency between the old marker and the old stats
    if (!isValidFor(room, oldMarker)) {
        qCWarning(EVENTS) << "oldMarker is invalid in room" << room->displayName();
    }
    Q_ASSERT(oldMarker > newMarker);

    // A bit of optimisation: only calculate the difference if the marker moved
    // less than half the remaining timeline ahead; otherwise, recalculation
    // over the remaining timeline will very likely be faster.
    if (oldMarker != room->historyEdge()
        && oldMarker - newMarker < newMarker - marker_t(room->syncEdge())) {
        const auto removedStats = fromRange(room, newMarker, oldMarker);
        Q_ASSERT(notableCount >= removedStats.notableCount
                 && highlightCount >= removedStats.highlightCount);
        notableCount -= removedStats.notableCount;
        highlightCount -= removedStats.highlightCount;
        return removedStats.notableCount > 0 || removedStats.highlightCount > 0;
    }

    const auto newStats = EventStats::fromMarker(room, newMarker);
    if (!isEstimate && newStats == *this)
        return false;
    *this = newStats;
    return true;
}

bool EventStats::isValidFor(const Room* room, const marker_t& marker) const
{
    const auto markerAtHistoryEdge = marker == room->historyEdge();
    // Either markerAtHistoryEdge and isEstimate are in the same state, or it's
    // a special case of no notable events and the marker at history edge
    // (then isEstimate can assume any value).
    return markerAtHistoryEdge == isEstimate
           || (markerAtHistoryEdge && notableCount == 0);
}

QDebug Quotient::operator<<(QDebug dbg, const EventStats& es)
{
    QDebugStateSaver _(dbg);
    dbg.nospace() << es.notableCount << '/' << es.highlightCount;
    if (es.isEstimate)
        dbg << " (estimated)";
    return dbg;
}
