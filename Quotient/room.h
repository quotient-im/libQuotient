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
#include "roommember.h"
#include "roomstateview.h"
#include "eventitem.h"
#include "quotient_common.h"

#include "csapi/message_pagination.h"

#include "events/accountdataevents.h"
#include "events/encryptedevent.h"
#include "events/eventrelation.h"
#include "events/roomcreateevent.h"
#include "events/roomkeyevent.h"
#include "events/roommessageevent.h"
#include "events/roompowerlevelsevent.h"
#include "events/roomtombstoneevent.h"

#include <QtCore/QJsonObject>
#include <QtGui/QImage>

#include <deque>
#include <utility>

namespace Quotient {
class Event;
class Avatar;
class SyncRoomData;
class RoomMemberEvent;
class User;
class RoomMember;
struct MemberSorter;
class LeaveRoomJob;
class SetRoomStateWithKeyJob;
class RedactEventJob;

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
    Q_PROPERTY(RoomMember localMember READ localMember CONSTANT)
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
    Q_PROPERTY(int joinedCount READ joinedCount NOTIFY memberListChanged)
    Q_PROPERTY(int invitedCount READ invitedCount NOTIFY memberListChanged)
    Q_PROPERTY(int totalMemberCount READ totalMemberCount NOTIFY memberListChanged)
    Q_PROPERTY(QList<RoomMember> membersTyping READ membersTyping NOTIFY typingChanged)
    Q_PROPERTY(QList<RoomMember> otherMembersTyping READ otherMembersTyping NOTIFY typingChanged)
    Q_PROPERTY(int localMemberEffectivePowerLevel READ memberEffectivePowerLevel NOTIFY changed)

    Q_PROPERTY(bool displayed READ displayed WRITE setDisplayed NOTIFY
                   displayedChanged)
    Q_PROPERTY(QString firstDisplayedEventId READ firstDisplayedEventId WRITE
                   setFirstDisplayedEventId NOTIFY firstDisplayedEventChanged)
    Q_PROPERTY(QString lastDisplayedEventId READ lastDisplayedEventId WRITE
                   setLastDisplayedEventId NOTIFY lastDisplayedEventChanged)
    Q_PROPERTY(QString lastFullyReadEventId READ lastFullyReadEventId WRITE
                   markMessagesAsRead NOTIFY fullyReadMarkerMoved)
    Q_PROPERTY(qsizetype highlightCount READ highlightCount
                   NOTIFY highlightCountChanged)
    Q_PROPERTY(qsizetype notificationCount READ notificationCount
                   NOTIFY notificationCountChanged)
    Q_PROPERTY(EventStats partiallyReadStats READ partiallyReadStats NOTIFY partiallyReadStatsChanged)
    Q_PROPERTY(EventStats unreadStats READ unreadStats NOTIFY unreadStatsChanged)
    Q_PROPERTY(bool allHistoryLoaded READ allHistoryLoaded NOTIFY allHistoryLoadedChanged
                   STORED false)
    Q_PROPERTY(QStringList tagNames READ tagNames NOTIFY tagsChanged)
    Q_PROPERTY(bool isFavourite READ isFavourite NOTIFY tagsChanged STORED false)
    Q_PROPERTY(bool isLowPriority READ isLowPriority NOTIFY tagsChanged STORED false)

    Q_PROPERTY(GetRoomEventsJob* eventsHistoryJob READ eventsHistoryJob NOTIFY eventsHistoryJobChanged)
    Q_PROPERTY(int requestedHistorySize READ requestedHistorySize NOTIFY eventsHistoryJobChanged)

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
        // NotInUse = 0x2,
        Topic = 0x4, //!< \sa topicChanged
        PartiallyReadStats = 0x8, //!< \sa partiallyReadStatsChanged
        Avatar = 0x10, //!< \sa avatarChanged
        JoinState = 0x20, //!< \sa joinStateChanged
        Tags = 0x40, //!< \sa tagsChanged
        //! \sa userAdded, userRemoved, memberRenamed, memberListChanged,
        //!     displaynameChanged
        Members = 0x80,
        UnreadStats = 0x100, //!< \sa unreadStatsChanged
        // AccountData pre-0.9 = 0x200,
        Summary = 0x400, //!< \sa summaryChanged, displaynameChanged
        // ReadMarker pre-0.9 = 0x800,
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

    //! Get a RoomMember object for the local user.
    RoomMember localMember() const;
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

    int timelineSize() const;
    bool usesEncryption() const;
    RoomEventPtr decryptMessage(const EncryptedEvent& encryptedEvent);
    void handleRoomKeyEvent(const RoomKeyEvent& roomKeyEvent,
                            const QString& senderId,
                            const QByteArray& olmSessionId,
                            const QByteArray& senderKey,
                            const QByteArray& senderEdKey);
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

    //! \brief Get a RoomMember object for the given user Matrix ID
    //!
    //! Will return a nullptr if there is no m.room.member event for the user in
    //! the room so needs to be null checked.
    //!
    //! \note This can return a member in any state that is known to the room so
    //!       check the state (using RoomMember::membershipState()) before use.
    Q_INVOKABLE RoomMember member(const QString& userId) const;

    //! Get a list of room members who have joined the room.
    QList<RoomMember> joinedMembers() const;

    //! Get a list of all members known to the room.
    QList<RoomMember> members() const;

    //! Get a list of all members known to have left the room.
    QList<RoomMember> membersLeft() const;

    //! Get a list of room members who are currently sending a typing indicator.
    QList<RoomMember> membersTyping() const;

    //! \brief Get a list of room members who are currently sending a typing indicator.
    //!
    //! The local member is excluded from this list.
    QList<RoomMember> otherMembersTyping() const;

    //! Get a list of room member Matrix IDs who have joined the room.
    QStringList joinedMemberIds() const;

    //! Get a list of all member Matrix IDs known to the room.
    QStringList memberIds() const;

    //! Whether the name for the given member should be disambiguated
    bool needsDisambiguation(const QString& userId) const;

    //! \brief Check the join state of a given user in this room
    //!
    //! \return the given user's state with respect to the room
    Q_INVOKABLE Quotient::Membership memberState(const QString& userId) const;

    //! Check whether a user with the given id is a member of the room
    Q_INVOKABLE bool isMember(const QString& userId) const;

    const Avatar& memberAvatarObject(const QString& memberId) const;

    //! \brief Get a avatar of the specified dimensions
    //!
    //! This always returns immediately; if there's no avatar cached yet, the call triggers
    //! a network request, that will emit Room::memberAvatarUpdated() once completed.
    //! \return a pixmap with the avatar or a placeholder if there's none available yet
    Q_INVOKABLE QImage memberAvatar(const QString& memberId, int width, int height);

    //! \brief Get a square avatar of the specified size
    //!
    //! This is an overload for the case when the needed width and height are equal.
    Q_INVOKABLE QImage memberAvatar(const QString& memberId, int dimension);

    const Timeline& messageEvents() const;
    const PendingEvents& pendingEvents() const;

    //! \brief Get the number of requested historical events
    //! \return The number of requested events if there's a pending request; 0 otherwise
    int requestedHistorySize() const;

    //! Check whether all historical messages are already loaded
    //! \return true if the "oldest" event in the timeline is a room creation event and there's
    //!         no further history to load; false otherwise
    bool allHistoryLoaded() const;

    //! \brief Get a reverse iterator at the position before the "oldest" event
    //!
    //! Same as messageEvents().crend()
    rev_iter_t historyEdge() const;
    //! \brief Get an iterator for the position beyond the latest arrived event
    //!
    //! Same as messageEvents().cend()
    Timeline::const_iterator syncEdge() const;
    Q_INVOKABLE Quotient::TimelineItem::index_t minTimelineIndex() const;
    Q_INVOKABLE Quotient::TimelineItem::index_t maxTimelineIndex() const;
    Q_INVOKABLE bool isValidIndex(Quotient::TimelineItem::index_t timelineIndex) const;

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
    Tag tag(const QString& name) const;

    /** Add a new tag to this room
     * If this room already has this tag, nothing happens. If it's a new
     * tag for the room, the respective tag record is added to the set
     * of tags and the new set is sent to the server to update other
     * clients.
     */
    void addTag(const QString& name, const Tag& tagData = {});
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

    /// Get the list of members this room is a direct chat with
    QList<RoomMember> directChatMembers() const;

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

#if Quotient_VERSION_MAJOR == 0 && Quotient_VERSION_MINOR < 10
    [[deprecated("Create MemberSorter objects directly instead")]]
    MemberSorter memberSorter() const;
#endif

    Q_INVOKABLE bool supportsCalls() const;

    /// Whether the current user is allowed to upgrade the room
    Q_INVOKABLE bool canSwitchVersions() const;

    /// \brief Get the current room state
    RoomStateView currentState() const;

    //! \brief The effective power level of the given member in the room
    //!
    //! This is normally the same as calling `RoomPowerLevelEvent::powerLevelForUser(userId)` but
    //! takes into account the room context and works even if the room state has no power levels
    //! event. It is THE recommended way to get a room member's power level to display in the UI.
    //! \param memberId The room member ID to check; if empty, the local user will be checked
    //! \sa RoomPowerLevelsEvent, https://spec.matrix.org/v1.11/client-server-api/#mroompower_levels
    Q_INVOKABLE int memberEffectivePowerLevel(const QString& memberId = {}) const;

    //! \brief Get the power level required to send events of the given type
    //!
    //! \note This is a generic method that only gets the power level to send events with a given
    //!       type. Some operations have additional restrictions or enablers though: e.g.,
    //!       room member changes (kicks, invites) have special power levels; on the other hand,
    //!       redactions of one's own messages are allowed regardless of the power level. To check
    //!       effective ability to perform an operation, use Room's can*() methods instead of
    //!       comparing the power levels (those are also slightly more efficient).
    //! \note Unlike the template version below, this method determines at runtime whether an event
    //!       type is that of a state event, assuming unknown event types to be non-state; pass
    //!       `true` as the second parameter to override that.
    //! \sa canSend, canRedact, canSwitchVersions
    Q_INVOKABLE int powerLevelFor(const QString& eventTypeId, bool forceStateEvent = false) const;

    //! \brief Get the power level required to send events of the given type
    //!
    //! This is an optimised version of non-template powerLevelFor() (with the same caveat about
    //! operations based on some event types) for cases when the event type is known at build time.
    //! \tparam EvT the event type to get the power level for
    template <EventClass EvT>
    int powerLevelFor() const
    {
        return currentState().get<RoomPowerLevelsEvent>()->powerLevelForEventType<EvT>();
    }

    //! \brief Post a pre-created room message event
    //!
    //! Takes ownership of the event, deleting it once the matching one arrives with the sync.
    //! \note Do not assume that the event is already on the road to the homeserver when this (or
    //!       any other `post*`) method returns; it can be queued internally.
    //! \sa PendingEventItem::deliveryStatus()
    //! \return a reference to the pending event item
    const PendingEventItem& post(RoomEventPtr event);

    template <typename EvT, typename... ArgTs>
    const PendingEventItem& post(ArgTs&&... args)
    {
        return post(makeEvent<EvT>(std::forward<ArgTs>(args)...));
    }

    PendingEventItem::future_type whenMessageMerged(QString txnId) const;

    //! Send a request to update the room state with the given event
    SetRoomStateWithKeyJob* setState(const StateEvent& evt);

    //! \brief Set a state event of the given type with the given arguments
    //!
    //! This type-safe overload attempts to send a state event of the type \p EvT constructed from
    //! \p args.
    template <typename EvT, typename... ArgTs>
    auto setState(ArgTs&&... args)
    {
        return setState(EvT(std::forward<ArgTs>(args)...));
    }

    void addMegolmSessionFromBackup(const QByteArray &sessionId, const QByteArray &sessionKey, uint32_t index, const QByteArray& senderKey, const QByteArray& senderEdKey);

    Q_INVOKABLE void startVerification();

    QJsonArray exportMegolmSessions();

public Q_SLOTS:
    /** Check whether the room should be upgraded */
    void checkVersion();

    QString postMessage(const QString& plainText, MessageEventType type, std::optional<EventRelation> relatesTo = std::nullopt);
    QString postPlainText(const QString& plainText, std::optional<EventRelation> relatesTo = std::nullopt);
    QString postHtmlMessage(const QString& plainText, const QString& html,
                            MessageEventType type = MessageEventType::Text,
                            std::optional<EventRelation> relatesTo = std::nullopt);
    QString postHtmlText(const QString& plainText, const QString& html, std::optional<EventRelation> relatesTo = std::nullopt);
    /// Send a reaction on a given event with a given key
    QString postReaction(const QString& eventId, const QString& key);

    QString postFile(const QString& plainText, EventContent::TypedBase* content, std::optional<EventRelation> relatesTo = std::nullopt);

    /** Post a pre-created room message event
     *
     * Takes ownership of the event, deleting it once the matching one
     * arrives with the sync
     * \return transaction id associated with the event.
     */
    [[deprecated("Use post() instead")]]
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

    JobHandle<GetRoomEventsJob> getPreviousContent(int limit = 10, const QString &filter = {});

    void inviteToRoom(const QString& memberId);
    JobHandle<LeaveRoomJob> leaveRoom();
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

    //! \brief A new member has joined the room
    //!
    //! This can be from any previous state or a member previously unknown to
    //! the room.
    void memberJoined(RoomMember member);

    //! \brief A member who previously joined has left
    //!
    //! The member will still be known to the room their membership state has changed
    //! from Membership::Join to anything else.
    void memberLeft(RoomMember member);

    //! A known joined member is about to update their display name
    void memberNameAboutToUpdate(RoomMember member, QString newName);

    //! A known joined member has updated their display name
    void memberNameUpdated(RoomMember member);

    //! A known joined member has updated their avatar
    void memberAvatarUpdated(RoomMember member);

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

    //! The list of members sending typing indicators has changed.
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
    void partiallyReadStatsChanged();
    void unreadStatsChanged();
    void allHistoryLoadedChanged();

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

template <template <class> class ContT>
inline typename ContT<RoomMember>::size_type lowerBoundMemberIndex(const ContT<RoomMember>& c,
                                                                   const auto& v,
                                                                   MemberSorter ms = {})
{
    return std::ranges::lower_bound(c, v, ms) - c.begin();
}

template <template <class> class ContT>
inline typename ContT<QString>::size_type lowerBoundMemberIndex(const ContT<QString>& c,
                                                                const auto& v, const Room* r,
                                                                MemberSorter ms = {})
{
    return std::ranges::lower_bound(c, v, ms, std::bind_front(&Room::member, r)) - c.begin();
}

} // namespace Quotient
Q_DECLARE_METATYPE(Quotient::FileTransferInfo)
Q_DECLARE_METATYPE(Quotient::ReadReceipt)
Q_DECLARE_OPERATORS_FOR_FLAGS(Quotient::Room::Changes)
