// SPDX-FileCopyrightText: 2016 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-FileCopyrightText: 2017 Roman Plášil <me@rplasil.name>
// SPDX-FileCopyrightText: 2017 Marius Gripsgard <marius@ubports.com>
// SPDX-FileCopyrightText: 2018 Josip Delic <delijati@googlemail.com>
// SPDX-FileCopyrightText: 2018 Black Hat <bhat@encom.eu.org>
// SPDX-FileCopyrightText: 2019 Alexey Andreyev <aa13q@ya.ru>
// SPDX-FileCopyrightText: 2020 Ram Nad <ramnad1999@gmail.com>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "connection.h"
#include "roomstateview.h"
#include "eventitem.h"
#include "quotient_common.h"

#include "csapi/message_pagination.h"

#include "events/accountdataevents.h"
#include "events/encryptedevent.h"
#include "events/roomkeyevent.h"
#include "events/roommessageevent.h"
#include "events/roomcreateevent.h"
#include "events/roomtombstoneevent.h"
#include "events/eventrelation.h"

#include <QtCore/QJsonObject>
#include <QtGui/QImage>

#include <deque>
#include <memory>
#include <utility>

namespace Quotient {
class Event;
class Avatar;
class SyncRoomData;
class RoomMemberEvent;
class User;
class MemberSorter;
class LeaveRoomJob;
class SetRoomStateWithKeyJob;
class RedactEventJob;
class SlidingSyncRoom;

/** The data structure used to expose file transfer information to views
 *
 * This is specifically tuned to work with QML exposing all traits as
 * Q_PROPERTY values.
 */
class QUOTIENT_API FileTransferInfo {
    Q_GADGET
    Q_PROPERTY(bool isUpload MEMBER isUpload CONSTANT)
    Q_PROPERTY(bool active READ active CONSTANT)
    Q_PROPERTY(bool started READ started CONSTANT)
    Q_PROPERTY(bool completed READ completed CONSTANT)
    Q_PROPERTY(bool failed READ failed CONSTANT)
    Q_PROPERTY(int progress MEMBER progress CONSTANT)
    Q_PROPERTY(int total MEMBER total CONSTANT)
    Q_PROPERTY(QUrl localDir MEMBER localDir CONSTANT)
    Q_PROPERTY(QUrl localPath MEMBER localPath CONSTANT)
public:
    enum Status { None, Started, Completed, Failed, Cancelled };
    Status status = None;
    bool isUpload = false;
    int progress = 0;
    int total = -1;
    QUrl localDir {};
    QUrl localPath {};

    bool started() const { return status == Started; }
    bool completed() const { return status == Completed; }
    bool active() const { return started() || completed(); }
    bool failed() const { return status == Failed; }
};

//! \brief Data structure for a room member's read receipt
//! \sa Room::lastReadReceipt
class QUOTIENT_API ReadReceipt {
    Q_GADGET
    Q_PROPERTY(QString eventId MEMBER eventId CONSTANT)
    Q_PROPERTY(QDateTime timestamp MEMBER timestamp CONSTANT)
public:
    QString eventId;
    QDateTime timestamp = {};

    bool operator==(const ReadReceipt& other) const
    {
        return eventId == other.eventId && timestamp == other.timestamp;
    }
    bool operator!=(const ReadReceipt& other) const
    {
        return !operator==(other);
    }
};
inline void swap(ReadReceipt& lhs, ReadReceipt& rhs)
{
    swap(lhs.eventId, rhs.eventId);
    swap(lhs.timestamp, rhs.timestamp);
}

struct EventStats;

struct Notification
{
    enum Type { None = 0, Basic, Highlight };
    Q_ENUM(Type)

    Type type = None;

private:
    Q_GADGET
    Q_PROPERTY(Type type MEMBER type CONSTANT)
};

class QUOTIENT_API Room : public QObject {
    Q_OBJECT
    Q_PROPERTY(Connection* connection READ connection CONSTANT)
    Q_PROPERTY(User* localUser READ localUser CONSTANT)
    Q_PROPERTY(QString id READ id CONSTANT)
    Q_PROPERTY(QString version READ version NOTIFY baseStateLoaded)
    Q_PROPERTY(bool isUnstable READ isUnstable NOTIFY stabilityUpdated)
    Q_PROPERTY(QString predecessorId READ predecessorId NOTIFY baseStateLoaded)
    Q_PROPERTY(QString successorId READ successorId NOTIFY upgraded)
    Q_PROPERTY(QString name READ name NOTIFY namesChanged)
    Q_PROPERTY(QStringList aliases READ aliases NOTIFY namesChanged)
    Q_PROPERTY(QStringList altAliases READ altAliases NOTIFY namesChanged)
    Q_PROPERTY(QString canonicalAlias READ canonicalAlias NOTIFY namesChanged)
    Q_PROPERTY(QString displayName READ displayName NOTIFY displaynameChanged)
    Q_PROPERTY(QStringList pinnedEventIds READ pinnedEventIds WRITE setPinnedEvents
                   NOTIFY pinnedEventsChanged)
    Q_PROPERTY(QString displayNameForHtml READ displayNameForHtml NOTIFY displaynameChanged)
    Q_PROPERTY(QString topic READ topic NOTIFY topicChanged)
    Q_PROPERTY(QString avatarMediaId READ avatarMediaId NOTIFY avatarChanged
                   STORED false)
    Q_PROPERTY(QUrl avatarUrl READ avatarUrl NOTIFY avatarChanged)
    Q_PROPERTY(bool usesEncryption READ usesEncryption NOTIFY encryption)

    Q_PROPERTY(int timelineSize READ timelineSize NOTIFY addedMessages)
    Q_PROPERTY(QStringList memberNames READ safeMemberNames NOTIFY memberListChanged)
    Q_PROPERTY(int joinedCount READ joinedCount NOTIFY memberListChanged)
    Q_PROPERTY(int invitedCount READ invitedCount NOTIFY memberListChanged)
    Q_PROPERTY(int totalMemberCount READ totalMemberCount NOTIFY memberListChanged)

    Q_PROPERTY(bool displayed READ displayed WRITE setDisplayed NOTIFY
                   displayedChanged)
    Q_PROPERTY(QString firstDisplayedEventId READ firstDisplayedEventId WRITE
                   setFirstDisplayedEventId NOTIFY firstDisplayedEventChanged)
    Q_PROPERTY(QString lastDisplayedEventId READ lastDisplayedEventId WRITE
                   setLastDisplayedEventId NOTIFY lastDisplayedEventChanged)
    //! \deprecated since 0.7
    Q_PROPERTY(QString readMarkerEventId READ readMarkerEventId WRITE
                   markMessagesAsRead NOTIFY readMarkerMoved)
    Q_PROPERTY(QString lastFullyReadEventId READ lastFullyReadEventId WRITE
                   markMessagesAsRead NOTIFY fullyReadMarkerMoved)
    //! \deprecated since 0.7
    Q_PROPERTY(bool hasUnreadMessages READ hasUnreadMessages NOTIFY
                   partiallyReadStatsChanged STORED false)
    //! \deprecated since 0.7
    Q_PROPERTY(int unreadCount READ unreadCount NOTIFY partiallyReadStatsChanged
                   STORED false)
    Q_PROPERTY(qsizetype highlightCount READ highlightCount
                   NOTIFY highlightCountChanged)
    Q_PROPERTY(qsizetype notificationCount READ notificationCount
                   NOTIFY notificationCountChanged)
    Q_PROPERTY(EventStats partiallyReadStats READ partiallyReadStats NOTIFY partiallyReadStatsChanged)
    Q_PROPERTY(EventStats unreadStats READ unreadStats NOTIFY unreadStatsChanged)
    Q_PROPERTY(bool allHistoryLoaded READ allHistoryLoaded NOTIFY addedMessages
                   STORED false)
    Q_PROPERTY(QStringList tagNames READ tagNames NOTIFY tagsChanged)
    Q_PROPERTY(bool isFavourite READ isFavourite NOTIFY tagsChanged STORED false)
    Q_PROPERTY(bool isLowPriority READ isLowPriority NOTIFY tagsChanged STORED false)

    Q_PROPERTY(GetRoomEventsJob* eventsHistoryJob READ eventsHistoryJob NOTIFY
                   eventsHistoryJobChanged)

    Q_PROPERTY(QStringList accountDataEventTypes READ accountDataEventTypes NOTIFY accountDataChanged)

public:
    using Timeline = std::deque<TimelineItem>;
    using PendingEvents = std::vector<PendingEventItem>;
    using RelatedEvents = QVector<const RoomEvent*>;
    using rev_iter_t = Timeline::const_reverse_iterator;
    using timeline_iter_t = Timeline::const_iterator;

    //! \brief Room changes that can be tracked using Room::changed() signal
    //!
    //! This enumeration lists kinds of changes that can be tracked with
    //! a "cumulative" changed() signal instead of using individual signals for
    //! each change. Specific enumerators mention these individual signals.
    //! \sa changed
    enum class Change : quint32 { // QFlags can't go more than 32-bit
        None = 0x0, //!< No changes occurred in the room
        RoomNames = 0x1, //!< \sa namesChanged, displaynameChanged
        DECL_DEPRECATED_ENUMERATOR(Name, RoomNames),
        DECL_DEPRECATED_ENUMERATOR(Aliases, RoomNames),
        DECL_DEPRECATED_ENUMERATOR(CanonicalAlias, RoomNames),
        // Aliases/CanonicalAlias pre-0.8 = 0x2,
        Topic = 0x4, //!< \sa topicChanged
        PartiallyReadStats = 0x8, //!< \sa partiallyReadStatsChanged
        DECL_DEPRECATED_ENUMERATOR(UnreadNotifs, PartiallyReadStats),
        Avatar = 0x10, //!< \sa avatarChanged
        JoinState = 0x20, //!< \sa joinStateChanged
        Tags = 0x40, //!< \sa tagsChanged
        //! \sa userAdded, userRemoved, memberRenamed, memberListChanged,
        //!     displaynameChanged
        Members = 0x80,
        UnreadStats = 0x100, //!< \sa unreadStatsChanged
        AccountData Q_DECL_ENUMERATOR_DEPRECATED_X(
            "Change::AccountData will be merged into Change::Other in 0.8") =
            0x200,
        Summary = 0x400, //!< \sa summaryChanged, displaynameChanged
        ReadMarker Q_DECL_ENUMERATOR_DEPRECATED_X(
            "Change::ReadMarker will be merged into Change::Other in 0.8") =
            0x800,
        Highlights = 0x1000, //!< \sa highlightCountChanged
        //! A catch-all value that covers changes not listed above (such as
        //! encryption turned on or the room having been upgraded), as well as
        //! changes in the room state that the library is not aware of (e.g.,
        //! custom state events) and m.read/m.fully_read position changes.
        //! \sa encryptionChanged, upgraded, accountDataChanged
        Other = 0x8000,
        //! This is intended to test a Change/Changes value for non-emptiness;
        //! adding <tt>& Change::Any</tt> has the same meaning as
        //! !testFlag(Change::None) or adding <tt>!= Change::None</tt>
        //! \note testFlag(Change::Any) tests that _all_ bits are on and
        //!       will always return false.
        Any = 0xFFFF
    };
    QUO_DECLARE_FLAGS(Changes, Change)

    Room(Connection* connection, QString id, JoinState initialJoinState);
    Q_DISABLE_COPY_MOVE(Room)
    ~Room() override;

    // Property accessors

    Connection* connection() const;
    User* localUser() const;
    const QString& id() const;
    QString version() const;
    bool isUnstable() const;
    QString predecessorId() const;
    /// Room predecessor
    /** This function validates that the predecessor has a tombstone and
     * the tombstone refers to the current room. If that's not the case,
     * or if the predecessor is in a join state not matching \p stateFilter,
     * the function returns nullptr.
     */
    Room* predecessor(JoinStates statesFilter = JoinState::Invite
                                                | JoinState::Join) const;
    QString successorId() const;
    /// Room successor
    /** This function validates that the successor room's creation event
     * refers to the current room. If that's not the case, or if the successor
     * is in a join state not matching \p stateFilter, it returns nullptr.
     */
    Room* successor(JoinStates statesFilter = JoinState::Invite
                                              | JoinState::Join) const;
    QString name() const;
    QString canonicalAlias() const;
    QStringList altAliases() const;
    //! Get a list of both canonical and alternative aliases
    QStringList aliases() const;
    QString displayName() const;
    QStringList pinnedEventIds() const;
    // Returns events available locally, use pinnedEventIds() for full list
    QVector<const RoomEvent*> pinnedEvents() const;
    QString displayNameForHtml() const;
    QString topic() const;
    QString avatarMediaId() const;
    QUrl avatarUrl() const;
    const Avatar& avatarObject() const;
    Q_INVOKABLE JoinState joinState() const;
    Q_INVOKABLE QList<Quotient::User*> usersTyping() const;
    QList<User*> membersLeft() const;

    Q_INVOKABLE QList<Quotient::User*> users() const;
    Q_DECL_DEPRECATED_X("Use safeMemberNames() or htmlSafeMemberNames() instead") //
    QStringList memberNames() const;
    QStringList safeMemberNames() const;
    QStringList htmlSafeMemberNames() const;
    int timelineSize() const;
    bool usesEncryption() const;
    RoomEventPtr decryptMessage(const EncryptedEvent& encryptedEvent);
    void handleRoomKeyEvent(const RoomKeyEvent& roomKeyEvent,
                            const QString& senderId,
                            const QByteArray& olmSessionId);
    int joinedCount() const;
    int invitedCount() const;
    int totalMemberCount() const;

    GetRoomEventsJob* eventsHistoryJob() const;

    /**
     * Returns a square room avatar with the given size and requests it
     * from the network if needed
     * \return a pixmap with the avatar or a placeholder if there's none
     * available yet
     */
    Q_INVOKABLE QImage avatar(int dimension);
    /**
     * Returns a room avatar with the given dimensions and requests it
     * from the network if needed
     * \return a pixmap with the avatar or a placeholder if there's none
     * available yet
     */
    Q_INVOKABLE QImage avatar(int width, int height);

    /**
     * \brief Get a user object for a given user id
     * This is the recommended way to get a user object in a room
     * context. The actual object type may be changed in further
     * versions to provide room-specific user information (display name,
     * avatar etc.).
     * \note The method will return a valid user regardless of
     *       the membership.
     */
    Q_INVOKABLE Quotient::User* user(const QString& userId) const;

    /**
     * \brief Check the join state of a given user in this room
     *
     * \note Banned and invited users are not tracked separately for now (Leave
     *       will be returned for them).
     *
     * \return Join if the user is a room member; Leave otherwise
     */
    Q_DECL_DEPRECATED_X("Use isMember() instead")
    Q_INVOKABLE Quotient::JoinState memberJoinState(Quotient::User* user) const;

    //! \brief Check the join state of a given user in this room
    //!
    //! \return the given user's state with respect to the room
    Q_INVOKABLE Quotient::Membership memberState(const QString& userId) const;

    //! Check whether a user with the given id is a member of the room
    Q_INVOKABLE bool isMember(const QString& userId) const;

    //! \brief Get a display name (without disambiguation) for the given member
    //!
    //! \sa safeMemberName, htmlSafeMemberName
    Q_INVOKABLE QString memberName(const QString& mxId) const;

    //! \brief Get a disambiguated name for the given user in the room context
    Q_DECL_DEPRECATED_X("Use safeMemberName() instead")
    Q_INVOKABLE QString roomMembername(const Quotient::User* u) const;
    //! \brief Get a disambiguated name for a user with this id in the room
    Q_DECL_DEPRECATED_X("Use safeMemberName() instead")
    Q_INVOKABLE QString roomMembername(const QString& userId) const;

    /*!
     * \brief Get a disambiguated name for the member with the given MXID
     *
     * This function should only be used for non-UI code; consider using
     * safeMemberName() or htmlSafeMemberName() for displayed strings.
     */
    Q_INVOKABLE QString disambiguatedMemberName(const QString& mxId) const;

    /*! Get a display-safe member name in the context of this room
     *
     * Display-safe means disambiguated and without RLO/LRO markers
     * (see https://github.com/quotient-im/Quaternion/issues/545).
     */
    Q_INVOKABLE QString safeMemberName(const QString& userId) const;

    /*! Get an HTML-safe member name in the context of this room
     *
     * This function adds HTML escaping on top of safeMemberName() safeguards.
     */
    Q_INVOKABLE QString htmlSafeMemberName(const QString& userId) const;

    //! \brief Get an avatar for the member with the given MXID
    QUrl memberAvatarUrl(const QString& mxId) const;

    const Timeline& messageEvents() const;
    const PendingEvents& pendingEvents() const;

    /// Check whether all historical messages are already loaded
    /**
     * \return true if the "oldest" event in the timeline is
     *         a room creation event and there's no further history
     *         to load; false otherwise
     */
    bool allHistoryLoaded() const;
    /**
     * A convenience method returning the read marker to the position
     * before the "oldest" event; same as messageEvents().crend()
     */
    rev_iter_t historyEdge() const;
    /**
     * A convenience method returning the iterator beyond the latest
     * arrived event; same as messageEvents().cend()
     */
    Timeline::const_iterator syncEdge() const;
    Q_INVOKABLE Quotient::TimelineItem::index_t minTimelineIndex() const;
    Q_INVOKABLE Quotient::TimelineItem::index_t maxTimelineIndex() const;
    Q_INVOKABLE bool
    isValidIndex(Quotient::TimelineItem::index_t timelineIndex) const;

    rev_iter_t findInTimeline(TimelineItem::index_t index) const;
    rev_iter_t findInTimeline(const QString& evtId) const;
    PendingEvents::iterator findPendingEvent(const QString& txnId);
    PendingEvents::const_iterator findPendingEvent(const QString& txnId) const;

    const RelatedEvents relatedEvents(const QString& evtId,
                                      EventRelation::reltypeid_t relType) const;
    const RelatedEvents relatedEvents(const RoomEvent& evt,
                                      EventRelation::reltypeid_t relType) const;

    const RoomCreateEvent* creation() const;
    const RoomTombstoneEvent* tombstone() const;

    bool displayed() const;
    /// Mark the room as currently displayed to the user
    /**
     * Marking the room displayed causes the room to obtain the full
     * list of members if it's been lazy-loaded before; in the future
     * it may do more things bound to "screen time" of the room, e.g.
     * measure that "screen time".
     */
    void setDisplayed(bool displayed = true);
    QString firstDisplayedEventId() const;
    rev_iter_t firstDisplayedMarker() const;
    void setFirstDisplayedEventId(const QString& eventId);
    void setFirstDisplayedEvent(TimelineItem::index_t index);
    QString lastDisplayedEventId() const;
    rev_iter_t lastDisplayedMarker() const;
    void setLastDisplayedEventId(const QString& eventId);
    void setLastDisplayedEvent(TimelineItem::index_t index);

    //! \brief Obtain a read receipt of any user
    //! \deprecated Use lastReadReceipt or fullyReadMarker instead.
    //!
    //! Historically, readMarker was returning a "converged" read marker
    //! representing both the read receipt and the fully read marker, as
    //! Quotient managed them together. Since 0.6.8, a single-argument call of
    //! readMarker returns the last read receipt position (for any room member)
    //! and a call without arguments returns the last _fully read_ position,
    //! to provide access to both positions separately while maintaining API
    //! stability guarantees. 0.7 has separate methods to return read receipts
    //! and the fully read marker - use them instead.
    //! \sa lastReadReceipt
    [[deprecated("Use lastReadReceipt() to get m.read receipt or"
                 " fullyReadMarker() to get m.fully_read marker")]] //
    rev_iter_t readMarker(const User* user) const;
    //! \brief Obtain the local user's fully-read marker
    //! \deprecated Use fullyReadMarker instead
    //!
    //! See the documentation for the single-argument overload.
    //! \sa fullyReadMarker
    [[deprecated("Use localReadReceiptMarker() or fullyReadMarker()")]] //
    rev_iter_t readMarker() const;
    //! \brief Get the event id for the local user's fully-read marker
    //! \deprecated Use lastFullyReadEventId instead
    //!
    //! See the readMarker documentation
    [[deprecated("Use lastReadReceipt() to get m.read receipt or"
                 " lastFullyReadEventId() to get an event id that"
                 " m.fully_read marker points to")]] //
    QString readMarkerEventId() const;

    //! \brief Get the latest read receipt from a user
    //!
    //! The user id must be valid. A read receipt with an empty event id
    //! is returned if the user id is valid but there was no read receipt
    //! from them.
    //! \sa usersAtEventId
    ReadReceipt lastReadReceipt(const QString& userId) const;

    //! \brief Get the latest read receipt from the local user
    //!
    //! This is a shortcut for <tt>lastReadReceipt(localUserId)</tt>.
    //! \sa lastReadReceipt
    ReadReceipt lastLocalReadReceipt() const;

    //! \brief Find the timeline item the local read receipt is at
    //!
    //! This is a shortcut for \code
    //! room->findInTimeline(room->lastLocalReadReceipt().eventId);
    //! \endcode
    rev_iter_t localReadReceiptMarker() const;

    //! \brief Get the latest event id marked as fully read
    //!
    //! This can be either the event id pointed to by the actual latest
    //! m.fully_read event, or the latest event id marked locally as fully read
    //! if markMessagesAsRead or markAllMessagesAsRead has been called and
    //! the homeserver didn't return an updated m.fully_read event yet.
    //! \sa markMessagesAsRead, markAllMessagesAsRead, fullyReadMarker
    QString lastFullyReadEventId() const;

    //! \brief Get the iterator to the latest timeline item marked as fully read
    //!
    //! This method calls findInTimeline on the result of lastFullyReadEventId.
    //! If the fully read marker turns out to be outside the timeline (because
    //! the event marked as fully read is too far back in the history) the
    //! returned value will be equal to historyEdge.
    //!
    //! Be sure to read the caveats on iterators returned by findInTimeline.
    //! \sa lastFullyReadEventId, findInTimeline
    rev_iter_t fullyReadMarker() const;

    //! \brief Get users whose latest read receipts point to the event
    //!
    //! This method is for cases when you need to show users who have read
    //! an event. Calling it on inexistent or empty event id will return
    //! an empty set.
    //! \note The returned list may contain ids resolving to users that are
    //!       not loaded as room members yet (in particular, if members are not
    //!       yet lazy-loaded). For now this merely means that the user's
    //!       room-specific name and avatar will not be there; but generally
    //!       it's recommended to ensure that all room members are loaded
    //!       before operating on the result of this function.
    //! \sa lastReadReceipt, allMembersLoaded
    QSet<QString> userIdsAtEvent(const QString& eventId) const;

    [[deprecated("Use userIdsAtEvent instead")]]
    QSet<User*> usersAtEventId(const QString& eventId);

    //! \brief Mark the event with uptoEventId as fully read
    //!
    //! Marks the event with the specified id as fully read locally and also
    //! sends an update to m.fully_read account data to the server either
    //! for this message or, if it's from the local user, for
    //! the nearest non-local message before. uptoEventId must point to a known
    //! event in the timeline; the method will do nothing if the event is behind
    //! the current m.fully_read marker or is not loaded, to prevent
    //! accidentally trying to move the marker back in the timeline.
    //! \sa markAllMessagesAsRead, fullyReadMarker
    Q_INVOKABLE void markMessagesAsRead(const QString& uptoEventId);

    //! \brief Determine whether an event should be counted as unread
    //!
    //! The criteria of including an event in unread counters are described in
    //! [MSC2654](https://github.com/matrix-org/matrix-doc/pull/2654); according
    //! to these, the event should be counted as unread (or, in libQuotient
    //! parlance, is "notable") if it is:
    //! - either
    //!   - a message event that is not m.notice, or
    //!   - a state event with type being one of:
    //!     `m.room.topic`, `m.room.name`, `m.room.avatar`, `m.room.tombstone`;
    //! - neither redacted, nor an edit (redactions cause the redacted event
    //!   to stop being notable, while edits are not notable themselves while
    //!   the original event usually is);
    //! - from a non-local user (events from other devices of the local
    //!   user are not notable).
    //! \sa partiallyReadStats, unreadStats
    virtual bool isEventNotable(const TimelineItem& ti) const;

    //! \brief Get notification details for an event
    //!
    //! This allows to get details on the kind of notification that should
    //! generated for \p evt.
    Notification notificationFor(const TimelineItem& ti) const;

    //! \brief Get event statistics since the fully read marker
    //!
    //! This call returns a structure containing:
    //! - the number of notable unread events since the fully read marker;
    //!   depending on the fully read marker state with respect to the local
    //!   timeline, this number may be either exact or estimated
    //!   (see EventStats::isEstimate);
    //! - the number of highlights (TODO).
    //!
    //! Note that this is different from the unread count defined by MSC2654
    //! and from the notification/highlight numbers defined by the spec in that
    //! it counts events since the fully read marker, not since the last
    //! read receipt position.
    //!
    //! As E2EE is not supported in the library, the returned result will always
    //! be an estimate (<tt>isEstimate == true</tt>) for encrypted rooms;
    //! moreover, since the library doesn't know how to tackle push rules yet
    //! the number of highlights returned here will always be zero (there's no
    //! good substitute for that now).
    //!
    //! \sa isEventNotable, fullyReadMarker, unreadStats, EventStats
    EventStats partiallyReadStats() const;

    //! \brief Get event statistics since the last read receipt
    //!
    //! This call returns a structure that contains the following three numbers,
    //! all counted on the timeline segment between the event pointed to by
    //! the m.fully_read marker and the sync edge:
    //! - the number of unread events - depending on the read receipt state
    //!   with respect to the local timeline, this number may be either precise
    //!   or estimated (see EventStats::isEstimate);
    //! - the number of highlights (TODO).
    //!
    //! As E2EE is not supported in the library, the returned result will always
    //! be an estimate (<tt>isEstimate == true</tt>) for encrypted rooms;
    //! moreover, since the library doesn't know how to tackle push rules yet
    //! the number of highlights returned here will always be zero - use
    //! highlightCount() for now.
    //!
    //! \sa isEventNotable, lastLocalReadReceipt, partiallyReadStats,
    //!     highlightCount
    EventStats unreadStats() const;

    [[deprecated(
        "Use partiallyReadStats/unreadStats() and EventStats::empty()")]]
    bool hasUnreadMessages() const;

    //! \brief Get the number of notable events since the fully read marker
    //!
    //! \deprecated Since 0.7 there are two ways to count unread events: since
    //! the fully read marker (used by libQuotient pre-0.7) and since the last
    //! read receipt (as used by most of Matrix ecosystem, including the spec
    //! and MSCs). This function currently returns a value derived from
    //! partiallyReadStats() for compatibility with libQuotient 0.6; it will be
    //! removed due to ambiguity. Use unreadStats() to obtain the spec-compliant
    //! count of unread events and the highlight count; partiallyReadStats() to
    //! obtain the unread events count since the fully read marker.
    //!
    //! \return -1 (_not 0_) when all messages are known to have been fully read,
    //!         i.e. the fully read marker points to _the latest notable_ event
    //!         loaded in the local timeline (which may be different from
    //!         the latest event in the local timeline as that might not be
    //!         notable);
    //!         0 when there may be unread messages but the current local
    //!         timeline doesn't have any notable ones (often but not always
    //!         because it's entirely empty yet);
    //!         a positive integer when there is (or estimated to be) a number
    //!         of unread notable events as described above.
    //!
    //! \sa partiallyReadStats, unreadStats
    [[deprecated("Use partiallyReadStats() or unreadStats() instead")]] //
    int unreadCount() const;

    //! \brief Get the number of notifications since the last read receipt
    //!
    //! This is the same as <tt>unreadStats().notableCount</tt>.
    //!
    //! \sa unreadStats, lastLocalReadReceipt
    qsizetype notificationCount() const;

    //! \brief Get the number of highlights since the last read receipt
    //!
    //! As of 0.7, this is defined by the homeserver as Quotient doesn't process
    //! push rules.
    //!
    //! \sa unreadStats, lastLocalReadReceipt
    qsizetype highlightCount() const;

    /** Check whether the room has account data of the given type
     * Tags and read markers are not supported by this method _yet_.
     */
    bool hasAccountData(const QString& type) const;

    /** Get a generic account data event of the given type
     * This returns a generic hash map for any room account data event
     * stored on the server. Tags and read markers cannot be retrieved
     * using this method _yet_.
     */
    const EventPtr& accountData(const QString& type) const;

    //! Get a list of all room account data events
    //! \return A list of event types that exist in the room
    QStringList accountDataEventTypes() const;

    QStringList tagNames() const;
    TagsMap tags() const;
    TagRecord tag(const QString& name) const;

    /** Add a new tag to this room
     * If this room already has this tag, nothing happens. If it's a new
     * tag for the room, the respective tag record is added to the set
     * of tags and the new set is sent to the server to update other
     * clients.
     */
    void addTag(const QString& name, const TagRecord& record = {});
    Q_INVOKABLE void addTag(const QString& name, float order);

    /// Remove a tag from the room
    Q_INVOKABLE void removeTag(const QString& name);

    /// The scope to apply an action on
    /*! This enumeration is used to pick a strategy to propagate certain
     * actions on the room to its predecessors and successors.
     */
    enum ActionScope {
        ThisRoomOnly,    ///< Do not apply to predecessors and successors
        WithinSameState, ///< Apply to predecessors and successors in the same
                         ///< state as the current one
        OmitLeftState,   ///< Apply to all reachable predecessors and successors
                         ///< except those in Leave state
        WholeSequence    ///< Apply to all reachable predecessors and successors
    };

    /** Overwrite the room's tags
     * This completely replaces the existing room's tags with a set
     * of new ones and updates the new set on the server. Unlike
     * most other methods in Room, this one sends a signal about changes
     * immediately, not waiting for confirmation from the server
     * (because tags are saved in account data rather than in shared
     * room state).
     * \param applyOn setting this to Room::OnAllConversations will set tags
     *                on this and all _known_ predecessors and successors;
     *                by default only the current room is changed
     */
    void setTags(TagsMap newTags, ActionScope applyOn = ThisRoomOnly);

    /// Check whether the list of tags has m.favourite
    bool isFavourite() const;
    /// Check whether the list of tags has m.lowpriority
    bool isLowPriority() const;
    /// Check whether this room is for server notices (MSC1452)
    bool isServerNoticeRoom() const;

    /// Check whether this room is a direct chat
    Q_INVOKABLE bool isDirectChat() const;

    /// Get the list of users this room is a direct chat with
    QList<User*> directChatUsers() const;

    Q_INVOKABLE QUrl makeMediaUrl(const QString& eventId,
                                  const QUrl &mxcUrl) const;

    Q_INVOKABLE QUrl urlToThumbnail(const QString& eventId) const;
    Q_INVOKABLE QUrl urlToDownload(const QString& eventId) const;

    /// Get a file name for downloading for a given event id
    /*!
     * The event MUST be RoomMessageEvent and have content
     * for downloading. \sa RoomMessageEvent::hasContent
     */
    Q_INVOKABLE QString fileNameToDownload(const QString& eventId) const;

    /// Get information on file upload/download
    /*!
     * \param id uploads are identified by the corresponding event's
     *           transactionId (because uploads are done before
     *           the event is even sent), while downloads are using
     *           the normal event id for identifier.
     */
    Q_INVOKABLE Quotient::FileTransferInfo
    fileTransferInfo(const QString& id) const;

    /// Get the URL to the actual file source in a unified way
    /*!
     * For uploads it will return a URL to a local file; for downloads
     * the URL will be taken from the corresponding room event.
     */
    Q_INVOKABLE QUrl fileSource(const QString& id) const;

    /** Pretty-prints plain text into HTML
     * As of now, it's exactly the same as Quotient::prettyPrint();
     * in the future, it will also linkify room aliases, mxids etc.
     * using the room context.
     */
    Q_INVOKABLE QString prettyPrint(const QString& plainText) const;

    MemberSorter memberSorter() const;

    Q_INVOKABLE bool supportsCalls() const;

    /// Whether the current user is allowed to upgrade the room
    Q_INVOKABLE bool canSwitchVersions() const;

    /// Get a state event with the given event type and state key
    /*! This method returns a (potentially empty) state event corresponding
     * to the pair of event type \p evtType and state key \p stateKey.
     */
    [[deprecated("Use currentState().get() instead; "
                 "make sure to check its result for nullptrs")]] //
    const StateEvent* getCurrentState(const QString& evtType,
                                      const QString& stateKey = {}) const;

    /// Get a state event with the given event type and state key
    /*! This is a typesafe overload that accepts a C++ event type instead of
     * its Matrix name.
     */
    template <typename EvT>
    [[deprecated("Use currentState().get() instead; "
                 "make sure to check its result for nullptrs")]] //
    const EvT* getCurrentState(const QString& stateKey = {}) const
    {
        QT_IGNORE_DEPRECATIONS(const auto* evt = eventCast<const EvT>(
                                   getCurrentState(EvT::TypeId, stateKey));)
        Q_ASSERT(evt);
        Q_ASSERT(evt->matrixType() == EvT::TypeId
                 && evt->stateKey() == stateKey);
        return evt;
    }

    /// \brief Get the current room state
    RoomStateView currentState() const;

    //! Send a request to update the room state with the given event
    SetRoomStateWithKeyJob* setState(const StateEvent& evt);

    //! \brief Set a state event of the given type with the given arguments
    //!
    //! This typesafe overload attempts to send a state event with the type
    //! \p EvT and the content defined by \p args. Specifically, the function
    //! constructs a temporary object of type \p EvT with its content
    //! list-initialised from \p args, and sends a request to the homeserver
    //! using the Matrix event type defined by \p EvT and the event content
    //! produced via EvT::contentJson().
    //!
    //! \note This call is not suitable for events that assume non-empty
    //!       stateKey, such as member events; for those you have to create
    //!       a temporary event object yourself and use the setState() overload
    //!       that accepts StateEvent const-ref.
    template <typename EvT, typename... ArgTs>
    auto setState(ArgTs&&... args)
    {
        return setState(EvT(std::forward<ArgTs>(args)...));
    }

public Q_SLOTS:
    /** Check whether the room should be upgraded */
    void checkVersion();

    QString postMessage(const QString& plainText, MessageEventType type);
    QString postPlainText(const QString& plainText);
    QString postHtmlMessage(const QString& plainText, const QString& html,
                            MessageEventType type = MessageEventType::Text);
    QString postHtmlText(const QString& plainText, const QString& html);
    /// Send a reaction on a given event with a given key
    QString postReaction(const QString& eventId, const QString& key);

    QString postFile(const QString& plainText, EventContent::TypedBase* content);
#if QT_VERSION_MAJOR < 6
    Q_DECL_DEPRECATED_X("Use postFile(QString, MessageEventType, EventContent)") //
    QString postFile(const QString& plainText, const QUrl& localPath,
                     bool asGenericFile = false);
#endif
    /** Post a pre-created room message event
     *
     * Takes ownership of the event, deleting it once the matching one
     * arrives with the sync
     * \return transaction id associated with the event.
     */
    QString postEvent(RoomEvent* event);
    QString postJson(const QString& matrixType, const QJsonObject& eventContent);
    QString retryMessage(const QString& txnId);
    void discardMessage(const QString& txnId);

    //! Send a request to update the room state based on freeform inputs
    SetRoomStateWithKeyJob* setState(const QString& evtType,
                                     const QString& stateKey,
                                     const QJsonObject& contentJson);
    void setName(const QString& newName);
    void setCanonicalAlias(const QString& newAlias);
    void setPinnedEvents(const QStringList& events);
    /// Set room aliases on the user's current server
    void setLocalAliases(const QStringList& aliases);
    void setTopic(const QString& newTopic);

    /// You shouldn't normally call this method; it's here for debugging
    void refreshDisplayName();

    void getPreviousContent(int limit = 10, const QString &filter = {});

    void inviteToRoom(const QString& memberId);
    LeaveRoomJob* leaveRoom();
    void kickMember(const QString& memberId, const QString& reason = {});
    void ban(const QString& userId, const QString& reason = {});
    void unban(const QString& userId);
    void redactEvent(const QString& eventId, const QString& reason = {});

    void uploadFile(const QString& id, const QUrl& localFilename,
                    const QString& overrideContentType = {});
    // If localFilename is empty a temporary file is created
    void downloadFile(const QString& eventId, const QUrl& localFilename = {});
    void cancelFileTransfer(const QString& id);

    //! \brief Set a given event as last read and post a read receipt on it
    //!
    //! Does nothing if the event is behind the current read receipt.
    //! \sa lastReadReceipt, markMessagesAsRead, markAllMessagesAsRead
    void setReadReceipt(const QString& atEventId);
    //! Put the fully-read marker at the latest message in the room
    void markAllMessagesAsRead();

    /// Switch the room's version (aka upgrade)
    void switchVersion(QString newVersion);

    void inviteCall(const QString& callId, const int lifetime,
                    const QString& sdp);
    void sendCallCandidates(const QString& callId, const QJsonArray& candidates);
    [[deprecated("Lifetime argument is no more passed; "
                 "use 2-arg Room::answerCall() instead")]]
    void answerCall(const QString& callId, int lifetime, const QString& sdp);
    void answerCall(const QString& callId, const QString& sdp);
    void hangupCall(const QString& callId);

    /**
     * Activates encryption for this room.
     * Warning: Cannot be undone
     */
    void activateEncryption();

Q_SIGNALS:
    /// Initial set of state events has been loaded
    /**
     * The initial set is what comes from the initial sync for the room.
     * This includes all basic things like RoomCreateEvent,
     * RoomNameEvent, a (lazy-loaded, not full) set of RoomMemberEvents
     * etc. This is a per-room reflection of Connection::loadedRoomState
     * \sa Connection::loadedRoomState
     */
    void baseStateLoaded();
    void eventsHistoryJobChanged();
    void aboutToAddHistoricalMessages(Quotient::RoomEventsRange events);
    void aboutToAddNewMessages(Quotient::RoomEventsRange events);
    void addedMessages(int fromIndex, int toIndex);
    /// The event is about to be appended to the list of pending events
    void pendingEventAboutToAdd(Quotient::RoomEvent* event);
    /// An event has been appended to the list of pending events
    void pendingEventAdded();
    /// The remote echo has arrived with the sync and will be merged
    /// with its local counterpart
    /** NB: Requires a sync loop to be emitted */
    void pendingEventAboutToMerge(Quotient::RoomEvent* serverEvent,
                                  int pendingEventIndex);
    /// The remote and local copies of the event have been merged
    /** NB: Requires a sync loop to be emitted */
    void pendingEventMerged();
    /// An event will be removed from the list of pending events
    void pendingEventAboutToDiscard(int pendingEventIndex);
    /// An event has just been removed from the list of pending events
    void pendingEventDiscarded();
    /// The status of a pending event has changed
    /** \sa PendingEventItem::deliveryStatus */
    void pendingEventChanged(int pendingEventIndex);
    /// The server accepted the message
    /** This is emitted when an event sending request has successfully
     * completed. This does not mean that the event is already in the
     * local timeline, only that the server has accepted it.
     * \param txnId transaction id assigned by the client during sending
     * \param eventId event id assigned by the server upon acceptance
     * \sa postEvent, postPlainText, postMessage, postHtmlMessage
     * \sa pendingEventMerged, aboutToAddNewMessages
     */
    void messageSent(QString txnId, QString eventId);

    /** A common signal for various kinds of changes in the room
     * Aside from all changes in the room state
     * @param changes a set of flags describing what changes occurred
     *                upon the last sync
     * \sa Changes
     */
    void changed(Quotient::Room::Changes changes);
    /**
     * \brief The room name, the canonical alias or other aliases changed
     *
     * Not triggered when display name changes.
     */
    void namesChanged(Quotient::Room* room);
    void displaynameAboutToChange(Quotient::Room* room);
    void displaynameChanged(Quotient::Room* room, QString oldName);
    void pinnedEventsChanged();
    void topicChanged();
    void avatarChanged();
    void userAdded(Quotient::User* user);
    void userRemoved(Quotient::User* user);
    void memberAboutToRename(Quotient::User* user, QString newName);
    void memberRenamed(Quotient::User* user);
    void memberAvatarChanged(Quotient::User* user);
    /// The list of members has changed
    /** Emitted no more than once per sync, this is a good signal to
     * for cases when some action should be done upon any change in
     * the member list. If you need per-item granularity you should use
     * userAdded, userRemoved and memberAboutToRename / memberRenamed
     * instead.
     */
    void memberListChanged();
    /// The previously lazy-loaded members list is now loaded entirely
    /// \sa setDisplayed
    void allMembersLoaded();
    void encryption();

    void joinStateChanged(Quotient::JoinState oldState,
                          Quotient::JoinState newState);
    void typingChanged();

    void highlightCountChanged(); ///< \sa highlightCount
    void notificationCountChanged(); ///< \sa notificationCount

    void displayedChanged(bool displayed);
    void firstDisplayedEventChanged();
    void lastDisplayedEventChanged();
    //! The event the m.read receipt points to has changed for the listed users
    //! \sa lastReadReceipt
    void lastReadEventChanged(QVector<QString> userIds);
    void fullyReadMarkerMoved(QString fromEventId, QString toEventId);
    [[deprecated("Since 0.7, use fullyReadMarkerMoved")]]
    void readMarkerMoved(QString fromEventId, QString toEventId);
    [[deprecated("Since 0.7, use lastReadEventChanged")]]
    void readMarkerForUserMoved(Quotient::User* user, QString fromEventId,
                                QString toEventId);
    [[deprecated("Since 0.7, use either partiallyReadStatsChanged "
                 "or unreadStatsChanged")]]
    void unreadMessagesChanged(Quotient::Room* room);
    void partiallyReadStatsChanged();
    void unreadStatsChanged();

    void accountDataAboutToChange(QString type);
    void accountDataChanged(QString type);
    void tagsAboutToChange();
    void tagsChanged();

    void updatedEvent(QString eventId);
    void replacedEvent(const Quotient::RoomEvent* newEvent,
                       const Quotient::RoomEvent* oldEvent);

    void newFileTransfer(QString id, QUrl localFile);
    void fileTransferProgress(QString id, qint64 progress, qint64 total);
    void fileTransferCompleted(QString id, QUrl localFile,
                               FileSourceInfo fileMetadata);
    void fileTransferFailed(QString id, QString errorMessage = {});
    // fileTransferCancelled() is no more here; use fileTransferFailed() and
    // check the transfer status instead

    void callEvent(Quotient::Room* room, const Quotient::RoomEvent* event);

    /// The room's version stability may have changed
    void stabilityUpdated(QString recommendedDefault,
                          QStringList stableVersions);
    /// This room has been upgraded and won't receive updates any more
    void upgraded(QString serverMessage, Quotient::Room* successor);
    /// An attempted room upgrade has failed
    void upgradeFailed(QString errorMessage);

    /// The room is about to be deleted
    void beforeDestruction(Quotient::Room*);

protected:
    virtual Changes processStateEvent(const RoomEvent& e);
    virtual Changes processEphemeralEvent(EventPtr&& event);
    virtual Changes processAccountDataEvent(EventPtr&& event);
    virtual void onAddNewTimelineEvents(timeline_iter_t /*from*/) {}
    virtual void onAddHistoricalTimelineEvents(rev_iter_t /*from*/) {}
    virtual void onRedaction(const RoomEvent& /*prevEvent*/,
                             const RoomEvent& /*after*/)
    {}
    virtual QJsonObject toJson() const;
    virtual void updateData(SyncRoomData&& data, bool fromCache = false);
    virtual void updateSlidingData(SlidingSyncRoom&& data);
    virtual Notification checkForNotifications(const TimelineItem& ti);

private:
    friend class Connection;

    class Private;
    Private* d;

    // This is called from Connection, reflecting a state change that
    // arrived from the server. Clients should use
    // Connection::joinRoom() and Room::leaveRoom() to change the state.
    void setJoinState(JoinState state);
};

class QUOTIENT_API MemberSorter {
public:
    explicit MemberSorter(const Room* r) : room(r) {}

    bool operator()(User* u1, User* u2) const;
    bool operator()(User* u1, QStringView u2name) const;

    template <typename ContT, typename ValT>
    typename ContT::size_type lowerBoundIndex(const ContT& c, const ValT& v) const
    {
        return std::lower_bound(c.begin(), c.end(), v, *this) - c.begin();
    }

private:
    const Room* room;
};
} // namespace Quotient
Q_DECLARE_METATYPE(Quotient::FileTransferInfo)
Q_DECLARE_METATYPE(Quotient::ReadReceipt)
Q_DECLARE_OPERATORS_FOR_FLAGS(Quotient::Room::Changes)
