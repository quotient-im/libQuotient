/******************************************************************************
 * Copyright (C) 2015 Felix Rohrbach <kde@fxrh.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#pragma once

#include "csapi/message_pagination.h"
#include "events/roommessageevent.h"
#include "events/accountdataevents.h"
#include "eventitem.h"
#include "joinstate.h"

#include <QtGui/QImage>

#include <memory>
#include <deque>
#include <utility>

namespace QMatrixClient
{
    class Event;
    class SyncRoomData;
    class RoomMemberEvent;
    class Connection;
    class User;
    class MemberSorter;
    class LeaveRoomJob;
    class SetRoomStateWithKeyJob;
    class RedactEventJob;

    class FileTransferInfo
    {
            Q_GADGET
            Q_PROPERTY(bool active READ active CONSTANT)
            Q_PROPERTY(bool completed READ completed CONSTANT)
            Q_PROPERTY(bool failed READ failed CONSTANT)
            Q_PROPERTY(int progress MEMBER progress CONSTANT)
            Q_PROPERTY(int total MEMBER total CONSTANT)
            Q_PROPERTY(QUrl localDir MEMBER localDir CONSTANT)
            Q_PROPERTY(QUrl localPath MEMBER localPath CONSTANT)
        public:
            enum Status { None, Started, Completed, Failed };
            Status status = None;
            int progress = 0;
            int total = -1;
            QUrl localDir { };
            QUrl localPath { };

            bool active() const
            { return status == Started || status == Completed; }
            bool completed() const { return status == Completed; }
            bool failed() const { return status == Failed; }
    };

    class Room: public QObject
    {
            Q_OBJECT
            Q_PROPERTY(Connection* connection READ connection CONSTANT)
            Q_PROPERTY(User* localUser READ localUser CONSTANT)
            Q_PROPERTY(QString id READ id CONSTANT)
            Q_PROPERTY(QString name READ name NOTIFY namesChanged)
            Q_PROPERTY(QStringList aliases READ aliases NOTIFY namesChanged)
            Q_PROPERTY(QString canonicalAlias READ canonicalAlias NOTIFY namesChanged)
            Q_PROPERTY(QString displayName READ displayName NOTIFY namesChanged)
            Q_PROPERTY(QString topic READ topic NOTIFY topicChanged)
            Q_PROPERTY(QString avatarMediaId READ avatarMediaId NOTIFY avatarChanged STORED false)
            Q_PROPERTY(QUrl avatarUrl READ avatarUrl NOTIFY avatarChanged)
            Q_PROPERTY(bool usesEncryption READ usesEncryption NOTIFY encryption)

            Q_PROPERTY(int timelineSize READ timelineSize NOTIFY addedMessages)
            Q_PROPERTY(QStringList memberNames READ memberNames NOTIFY memberListChanged)
            Q_PROPERTY(int memberCount READ memberCount NOTIFY memberListChanged)
            Q_PROPERTY(int joinedCount READ joinedCount NOTIFY memberListChanged)
            Q_PROPERTY(int invitedCount READ invitedCount NOTIFY memberListChanged)
            Q_PROPERTY(int totalMemberCount READ totalMemberCount NOTIFY memberListChanged)

            Q_PROPERTY(bool displayed READ displayed WRITE setDisplayed NOTIFY displayedChanged)
            Q_PROPERTY(QString firstDisplayedEventId READ firstDisplayedEventId WRITE setFirstDisplayedEventId NOTIFY firstDisplayedEventChanged)
            Q_PROPERTY(QString lastDisplayedEventId READ lastDisplayedEventId WRITE setLastDisplayedEventId NOTIFY lastDisplayedEventChanged)

            Q_PROPERTY(QString readMarkerEventId READ readMarkerEventId WRITE markMessagesAsRead NOTIFY readMarkerMoved)
            Q_PROPERTY(bool hasUnreadMessages READ hasUnreadMessages NOTIFY unreadMessagesChanged)
            Q_PROPERTY(int unreadCount READ unreadCount NOTIFY unreadMessagesChanged)
            Q_PROPERTY(QStringList tagNames READ tagNames NOTIFY tagsChanged)
            Q_PROPERTY(bool isFavourite READ isFavourite NOTIFY tagsChanged)
            Q_PROPERTY(bool isLowPriority READ isLowPriority NOTIFY tagsChanged)

            Q_PROPERTY(GetRoomEventsJob* eventsHistoryJob READ eventsHistoryJob NOTIFY eventsHistoryJobChanged)

        public:
            using Timeline = std::deque<TimelineItem>;
            using PendingEvents = std::vector<PendingEventItem>;
            using rev_iter_t = Timeline::const_reverse_iterator;
            using timeline_iter_t = Timeline::const_iterator;

            enum Change : uint {
                NoChange = 0x0,
                NameChange = 0x1,
                CanonicalAliasChange = 0x2,
                TopicChange = 0x4,
                UnreadNotifsChange = 0x8,
                AvatarChange = 0x10,
                JoinStateChange = 0x20,
                TagsChange = 0x40,
                MembersChange = 0x80,
                EncryptionOn = 0x100,
                AccountDataChange = 0x200,
                SummaryChange = 0x400,
                ReadMarkerChange = 0x800,
                OtherChange = 0x8000,
                AnyChange = 0xFFFF
            };
            Q_DECLARE_FLAGS(Changes, Change)
            Q_FLAG(Changes)

            Room(Connection* connection, QString id, JoinState initialJoinState);
            ~Room() override;

            // Property accessors

            Connection* connection() const;
            User* localUser() const;
            const QString& id() const;
            QString name() const;
            QStringList aliases() const;
            QString canonicalAlias() const;
            QString displayName() const;
            QString topic() const;
            QString avatarMediaId() const;
            QUrl avatarUrl() const;
            Q_INVOKABLE JoinState joinState() const;
            Q_INVOKABLE QList<User*> usersTyping() const;
            QList<User*> membersLeft() const;

            Q_INVOKABLE QList<User*> users() const;
            QStringList memberNames() const;
            [[deprecated("Use joinedCount(), invitedCount(), totalMemberCount()")]]
            int memberCount() const;
            int timelineSize() const;
            bool usesEncryption() const;
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
            Q_INVOKABLE User* user(const QString& userId) const;

            /**
             * \brief Check the join state of a given user in this room
             *
             * \note Banned and invited users are not tracked for now (Leave
             *       will be returned for them).
             *
             * \return either of Join, Leave, depending on the given
             *         user's state in the room
             */
            Q_INVOKABLE JoinState memberJoinState(User* user) const;

            /**
             * Get a disambiguated name for a given user in
             * the context of the room
             */
            Q_INVOKABLE QString roomMembername(const User* u) const;
            /**
             * Get a disambiguated name for a user with this id in
             * the context of the room
             */
            Q_INVOKABLE QString roomMembername(const QString& userId) const;

            const Timeline& messageEvents() const;
            const PendingEvents& pendingEvents() const;
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
            /// \deprecated Use historyEdge instead
            rev_iter_t timelineEdge() const;
            Q_INVOKABLE TimelineItem::index_t minTimelineIndex() const;
            Q_INVOKABLE TimelineItem::index_t maxTimelineIndex() const;
            Q_INVOKABLE bool isValidIndex(TimelineItem::index_t timelineIndex) const;

            rev_iter_t findInTimeline(TimelineItem::index_t index) const;
            rev_iter_t findInTimeline(const QString& evtId) const;

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

            rev_iter_t readMarker(const User* user) const;
            rev_iter_t readMarker() const;
            QString readMarkerEventId() const;
            QList<User*> usersAtEventId(const QString& eventId);
            /**
             * \brief Mark the event with uptoEventId as read
             *
             * Finds in the timeline and marks as read the event with
             * the specified id; also posts a read receipt to the server either
             * for this message or, if it's from the local user, for
             * the nearest non-local message before. uptoEventId must be non-empty.
             */
            void markMessagesAsRead(QString uptoEventId);

            /// Check whether there are unread messages in the room
            bool hasUnreadMessages() const;

            /** Get the number of unread messages in the room
             * Depending on the read marker state, this call may return either
             * a precise or an estimate number of unread events. Only "notable"
             * events (non-redacted message events from users other than local)
             * are counted.
             *
             * In a case when readMarker() == timelineEdge() (the local read
             * marker is beyond the local timeline) only the bottom limit of
             * the unread messages number can be estimated (and even that may
             * be slightly off due to, e.g., redactions of events not loaded
             * to the local timeline).
             *
             * If all messages are read, this function will return -1 (_not_ 0,
             * as zero may mean "zero or more unread messages" in a situation
             * when the read marker is outside the local timeline.
             */
            int unreadCount() const;

            Q_INVOKABLE int notificationCount() const;
            Q_INVOKABLE void resetNotificationCount();
            Q_INVOKABLE int highlightCount() const;
            Q_INVOKABLE void resetHighlightCount();

            /** Check whether the room has account data of the given type
             * Tags and read markers are not supported by this method _yet_.
             */
            bool hasAccountData(const QString& type) const;

            /** Get a generic account data event of the given type
             * This returns a generic hashmap for any room account data event
             * stored on the server. Tags and read markers cannot be retrieved
             * using this method _yet_.
             */
            const EventPtr& accountData(const QString& type) const;

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

            /** Overwrite the room's tags
             * This completely replaces the existing room's tags with a set
             * of new ones and updates the new set on the server. Unlike
             * most other methods in Room, this one sends a signal about changes
             * immediately, not waiting for confirmation from the server
             * (because tags are saved in account data rather than in shared
             * room state).
             */
            void setTags(TagsMap newTags);

            /// Check whether the list of tags has m.favourite
            bool isFavourite() const;
            /// Check whether the list of tags has m.lowpriority
            bool isLowPriority() const;

            /// Check whether this room is a direct chat
            Q_INVOKABLE bool isDirectChat() const;

            /// Get the list of users this room is a direct chat with
            QList<User*> directChatUsers() const;

            Q_INVOKABLE QUrl urlToThumbnail(const QString& eventId);
            Q_INVOKABLE QUrl urlToDownload(const QString& eventId);
            Q_INVOKABLE QString fileNameToDownload(const QString& eventId);
            Q_INVOKABLE FileTransferInfo fileTransferInfo(const QString& id) const;

            /** Pretty-prints plain text into HTML
             * As of now, it's exactly the same as QMatrixClient::prettyPrint();
             * in the future, it will also linkify room aliases, mxids etc.
             * using the room context.
             */
            QString prettyPrint(const QString& plainText) const;

            MemberSorter memberSorter() const;

            Q_INVOKABLE void inviteCall(const QString& callId,
                                        const int lifetime, const QString& sdp);
            Q_INVOKABLE void sendCallCandidates(const QString& callId,
                                            const QJsonArray& candidates);
            Q_INVOKABLE void answerCall(const QString& callId, const int lifetime,
                                        const QString& sdp);
            Q_INVOKABLE void answerCall(const QString& callId,
                                        const QString& sdp);
            Q_INVOKABLE void hangupCall(const QString& callId);
            Q_INVOKABLE bool supportsCalls() const;

        public slots:
            QString postMessage(const QString& plainText, MessageEventType type);
            QString postPlainText(const QString& plainText);
            QString postHtmlMessage(const QString& plainText,
                                    const QString& html, MessageEventType type);
            QString postHtmlText(const QString& plainText, const QString& html);
            /** Post a pre-created room message event
             *
             * Takes ownership of the event, deleting it once the matching one
             * arrives with the sync
             * \return transaction id associated with the event.
             */
            QString postEvent(RoomEvent* event);
            QString postJson(const QString& matrixType,
                             const QJsonObject& eventContent);
            QString retryMessage(const QString& txnId);
            void discardMessage(const QString& txnId);
            void setName(const QString& newName);
            void setCanonicalAlias(const QString& newAlias);
            void setTopic(const QString& newTopic);

            void getPreviousContent(int limit = 10);

            void inviteToRoom(const QString& memberId);
            LeaveRoomJob* leaveRoom();
            SetRoomStateWithKeyJob* setMemberState(
                    const QString& memberId, const RoomMemberEvent& event) const;
            void kickMember(const QString& memberId, const QString& reason = {});
            void ban(const QString& userId, const QString& reason = {});
            void unban(const QString& userId);
            void redactEvent(const QString& eventId,
                             const QString& reason = {});

            void uploadFile(const QString& id, const QUrl& localFilename,
                            const QString& overrideContentType = {});
            // If localFilename is empty a temporary file is created
            void downloadFile(const QString& eventId,
                              const QUrl& localFilename = {});
            void cancelFileTransfer(const QString& id);

            /// Mark all messages in the room as read
            void markAllMessagesAsRead();

        signals:
            void eventsHistoryJobChanged();
            void aboutToAddHistoricalMessages(RoomEventsRange events);
            void aboutToAddNewMessages(RoomEventsRange events);
            void addedMessages(int fromIndex, int toIndex);
            void pendingEventAboutToAdd();
            void pendingEventAdded();
            void pendingEventAboutToMerge(RoomEvent* serverEvent,
                                          int pendingEventIndex);
            void pendingEventMerged();
            void pendingEventAboutToDiscard(int pendingEventIndex);
            void pendingEventDiscarded();
            void pendingEventChanged(int pendingEventIndex);

            /** A common signal for various kinds of changes in the room
             * Aside from all changes in the room state
             * @param changes a set of flags describing what changes occured
             *                upon the last sync
             * \sa StateChange
             */
            void changed(Changes changes);
            /**
             * \brief The room name, the canonical alias or other aliases changed
             *
             * Not triggered when displayname changes.
             */
            void namesChanged(Room* room);
            void displaynameAboutToChange(Room* room);
            void displaynameChanged(Room* room, QString oldName);
            void topicChanged();
            void avatarChanged();
            void userAdded(User* user);
            void userRemoved(User* user);
            void memberAboutToRename(User* user, QString newName);
            void memberRenamed(User* user);
            void memberListChanged();
            /// The previously lazy-loaded members list is now loaded entirely
            /// \sa setDisplayed
            void allMembersLoaded();
            void encryption();

            void joinStateChanged(JoinState oldState, JoinState newState);
            void typingChanged();

            void highlightCountChanged(Room* room);
            void notificationCountChanged(Room* room);

            void displayedChanged(bool displayed);
            void firstDisplayedEventChanged();
            void lastDisplayedEventChanged();
            void lastReadEventChanged(User* user);
            void readMarkerMoved(QString fromEventId, QString toEventId);
            void readMarkerForUserMoved(User* user, QString fromEventId, QString toEventId);
            void unreadMessagesChanged(Room* room);

            void accountDataAboutToChange(QString type);
            void accountDataChanged(QString type);
            void tagsAboutToChange();
            void tagsChanged();

            void replacedEvent(const RoomEvent* newEvent,
                               const RoomEvent* oldEvent);

            void newFileTransfer(QString id, QUrl localFile);
            void fileTransferProgress(QString id, qint64 progress, qint64 total);
            void fileTransferCompleted(QString id, QUrl localFile, QUrl mxcUrl);
            void fileTransferFailed(QString id, QString errorMessage = {});
            void fileTransferCancelled(QString id);

            void callEvent(Room* room, const RoomEvent* event);
            /// The room is about to be deleted
            void beforeDestruction(Room*);

        protected:
            /// Returns true if any of room names/aliases has changed
            virtual Changes processStateEvent(const RoomEvent& e);
            virtual Changes processEphemeralEvent(EventPtr&& event);
            virtual Changes processAccountDataEvent(EventPtr&& event);
            virtual void onAddNewTimelineEvents(timeline_iter_t /*from*/) { }
            virtual void onAddHistoricalTimelineEvents(rev_iter_t /*from*/) { }
            virtual void onRedaction(const RoomEvent& /*prevEvent*/,
                                     const RoomEvent& /*after*/) { }
            virtual QJsonObject toJson() const;
            virtual void updateData(SyncRoomData&& data, bool fromCache = false);

        private:
            friend class Connection;

            class Private;
            Private* d;

            // This is called from Connection, reflecting a state change that
            // arrived from the server. Clients should use
            // Connection::joinRoom() and Room::leaveRoom() to change the state.
            void setJoinState(JoinState state);
    };

    class MemberSorter
    {
        public:
            explicit MemberSorter(const Room* r) : room(r) { }

            bool operator()(User* u1, User* u2) const;
            bool operator()(User* u1, const QString& u2name) const;

            template <typename ContT, typename ValT>
            typename ContT::size_type lowerBoundIndex(const ContT& c,
                                                      const ValT& v) const
            {
                return std::lower_bound(c.begin(), c.end(), v, *this) - c.begin();
            }

        private:
            const Room* room;
    };
}  // namespace QMatrixClient
Q_DECLARE_METATYPE(QMatrixClient::FileTransferInfo)
Q_DECLARE_OPERATORS_FOR_FLAGS(QMatrixClient::Room::Changes)
