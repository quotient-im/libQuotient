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

#include "jobs/syncjob.h"
#include "events/roommessageevent.h"
#include "events/accountdataevents.h"
#include "joinstate.h"

#include <QtGui/QPixmap>

#include <memory>
#include <deque>
#include <utility>

namespace QMatrixClient
{
    class Event;
    class RoomMemberEvent;
    class Connection;
    class User;
    class MemberSorter;
    class LeaveRoomJob;
    class SetRoomStateWithKeyJob;
    class RedactEventJob;

    class TimelineItem
    {
        public:
            // For compatibility with Qt containers, even though we use
            // a std:: container now for the room timeline
            using index_t = int;

            TimelineItem(RoomEventPtr&& e, index_t number)
                : evt(std::move(e)), idx(number)
            {
                Q_ASSERT(evt);
            }

            const RoomEvent* event() const { return rawPtr(evt); }
            const RoomEvent* get() const { return event(); }
            template <typename EventT>
            const EventT* viewAs() const { return eventCast<const EventT>(evt); }
            const RoomEventPtr& operator->() const { return evt; }
            const RoomEvent& operator*() const { return *evt; }
            index_t index() const { return idx; }

            // Used for event redaction
            RoomEventPtr replaceEvent(RoomEventPtr&& other);

        private:
            RoomEventPtr evt;
            index_t idx;
    };

    template<>
    inline const StateEventBase* TimelineItem::viewAs<StateEventBase>() const
    {
        return evt->isStateEvent() ? weakPtrCast<const StateEventBase>(evt)
                                   : nullptr;
    }

    inline QDebug& operator<<(QDebug& d, const TimelineItem& ti)
    {
        QDebugStateSaver dss(d);
        d.nospace() << "(" << ti.index() << "|" << ti->id() << ")";
        return d;
    }

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

            Q_PROPERTY(bool displayed READ displayed WRITE setDisplayed NOTIFY displayedChanged)
            Q_PROPERTY(QString firstDisplayedEventId READ firstDisplayedEventId WRITE setFirstDisplayedEventId NOTIFY firstDisplayedEventChanged)
            Q_PROPERTY(QString lastDisplayedEventId READ lastDisplayedEventId WRITE setLastDisplayedEventId NOTIFY lastDisplayedEventChanged)

            Q_PROPERTY(QString readMarkerEventId READ readMarkerEventId WRITE markMessagesAsRead NOTIFY readMarkerMoved)
            Q_PROPERTY(bool hasUnreadMessages READ hasUnreadMessages NOTIFY unreadMessagesChanged)
            Q_PROPERTY(int unreadCount READ unreadCount NOTIFY unreadMessagesChanged)
            Q_PROPERTY(QStringList tagNames READ tagNames NOTIFY tagsChanged)
            Q_PROPERTY(bool isFavourite READ isFavourite NOTIFY tagsChanged)
            Q_PROPERTY(bool isLowPriority READ isLowPriority NOTIFY tagsChanged)

        public:
            using Timeline = std::deque<TimelineItem>;
            using rev_iter_t = Timeline::const_reverse_iterator;
            using timeline_iter_t = Timeline::const_iterator;

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
            int memberCount() const;
            int timelineSize() const;
            bool usesEncryption() const;

            /**
             * Returns a square room avatar with the given size and requests it
             * from the network if needed
             * @return a pixmap with the avatar or a placeholder if there's none
             * available yet
             */
            Q_INVOKABLE QImage avatar(int dimension);
            /**
             * Returns a room avatar with the given dimensions and requests it
             * from the network if needed
             * @return a pixmap with the avatar or a placeholder if there's none
             * available yet
             */
            Q_INVOKABLE QImage avatar(int width, int height);

            /**
             * @brief Get a user object for a given user id
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
             * @brief Produces a disambiguated name for a given user in
             * the context of the room
             */
            Q_INVOKABLE QString roomMembername(const User* u) const;
            /**
             * @brief Produces a disambiguated name for a user with this id in
             * the context of the room
             */
            Q_INVOKABLE QString roomMembername(const QString& userId) const;

            const Timeline& messageEvents() const;
            const RoomEvents& pendingEvents() const;
            /**
             * A convenience method returning the read marker to the before-oldest
             * message
             */
            rev_iter_t timelineEdge() const;
            Q_INVOKABLE TimelineItem::index_t minTimelineIndex() const;
            Q_INVOKABLE TimelineItem::index_t maxTimelineIndex() const;
            Q_INVOKABLE bool isValidIndex(TimelineItem::index_t timelineIndex) const;

            rev_iter_t findInTimeline(TimelineItem::index_t index) const;
            rev_iter_t findInTimeline(const QString& evtId) const;

            bool displayed() const;
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
            /**
             * @brief Mark the event with uptoEventId as read
             *
             * Finds in the timeline and marks as read the event with
             * the specified id; also posts a read receipt to the server either
             * for this message or, if it's from the local user, for
             * the nearest non-local message before. uptoEventId must be non-empty.
             */
            void markMessagesAsRead(QString uptoEventId);

            /** Check whether there are unread messages in the room */
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
            Q_INVOKABLE void addTag(const QString& name, const QString& order);

            /** Remove a tag from the room */
            Q_INVOKABLE void removeTag(const QString& name);

            /** Overwrite the room's tags
             * This completely replaces the existing room's tags with a set
             * of new ones and updates the new set on the server. Unlike
             * most other methods in Room, this one sends a signal about changes
             * immediately, not waiting for confirmation from the server
             * (because tags are saved in account data rather than in shared
             * room state).
             */
            void setTags(const TagsMap& newTags);

            /** Check whether the list of tags has m.favourite */
            bool isFavourite() const;
            /** Check whether the list of tags has m.lowpriority */
            bool isLowPriority() const;

            /** Check whether this room is a direct chat */
            Q_INVOKABLE bool isDirectChat() const;

            /** Get the list of users this room is a direct chat with */
            QList<const User*> directChatUsers() const;

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

            QJsonObject toJson() const;
            void updateData(SyncRoomData&& data );
            void setJoinState( JoinState state );

        public slots:
            QString postMessage(const QString& plainText,
                                MessageEventType type = MessageEventType::Text);
            QString postHtmlMessage(
                        const QString& plainText, const QString& htmlText,
                        MessageEventType type = MessageEventType::Text);
            /** Post a pre-created room message event; takes ownership of the event */
            QString postMessage(RoomEvent* event);
            QString postMessage(const QString& matrixType,
                                const QJsonObject& eventContent);
            /** @deprecated If you have a custom event type, construct the event
             * and pass it as a whole to postMessage() */
            QString postMessage(const QString& type, const QString& plainText);
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

            /** Mark all messages in the room as read */
            void markAllMessagesAsRead();

        signals:
            void aboutToAddHistoricalMessages(RoomEventsRange events);
            void aboutToAddNewMessages(RoomEventsRange events);
            void addedMessages();
            void pendingEventAboutToAdd();
            void pendingEventAdded();
            void pendingEventAboutToMerge(RoomEvent* serverEvent,
                                          int pendingEventIndex);
            void pendingEventMerged();
            void pendingEventChanged(int pendingEventIndex);

            /**
             * @brief The room name, the canonical alias or other aliases changed
             *
             * Not triggered when displayname changes.
             */
            void namesChanged(Room* room);
            /** @brief The room displayname changed */
            void displaynameChanged(Room* room);
            void topicChanged();
            void avatarChanged();
            void userAdded(User* user);
            void userRemoved(User* user);
            void memberAboutToRename(User* user, QString newName);
            void memberRenamed(User* user);
            void memberListChanged();
            void encryption();

            void joinStateChanged(JoinState oldState, JoinState newState);
            void typingChanged();

            void highlightCountChanged(Room* room);
            void notificationCountChanged(Room* room);

            void displayedChanged(bool displayed);
            void firstDisplayedEventChanged();
            void lastDisplayedEventChanged();
            void lastReadEventChanged(User* user);
            void readMarkerMoved();
            void unreadMessagesChanged(Room* room);

            void accountDataChanged(QString type);
            void tagsChanged();

            void replacedEvent(const RoomEvent* newEvent,
                               const RoomEvent* oldEvent);

            void newFileTransfer(QString id, QUrl localFile);
            void fileTransferProgress(QString id, qint64 progress, qint64 total);
            void fileTransferCompleted(QString id, QUrl localFile, QUrl mxcUrl);
            void fileTransferFailed(QString id, QString errorMessage = {});
            void fileTransferCancelled(QString id);

        protected:
            /// Returns true if any of room names/aliases has changed
            virtual bool processStateEvent(const RoomEvent& e);
            virtual void processEphemeralEvent(EventPtr&& event);
            virtual void processAccountDataEvent(EventPtr&& event);
            virtual void onAddNewTimelineEvents(timeline_iter_t /*from*/) { }
            virtual void onAddHistoricalTimelineEvents(rev_iter_t /*from*/) { }
            virtual void onRedaction(const RoomEvent& /*prevEvent*/,
                                     const RoomEvent& /*after*/) { }

        private:
            class Private;
            Private* d;
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
