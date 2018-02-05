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
#include "joinstate.h"

#include <QtCore/QList>
#include <QtCore/QStringList>
#include <QtCore/QObject>
#include <QtCore/QJsonObject>
#include <QtGui/QPixmap>

#include <memory>
#include <deque>
#include <utility>

namespace QMatrixClient
{
    class Event;
    class Connection;
    class User;
    class MemberSorter;
    class LeaveRoomJob;
    class RedactEventJob;

    class TimelineItem
    {
        public:
            // For compatibility with Qt containers, even though we use
            // a std:: container now for the room timeline
            using index_t = int;

            TimelineItem(RoomEventPtr&& e, index_t number)
                : evt(move(e)), idx(number) { }

            RoomEvent* event() const { return evt.get(); }
            RoomEvent* operator->() const { return evt.operator->(); }
            index_t index() const { return idx; }

            // Used for event redaction
            RoomEventPtr replaceEvent(RoomEventPtr&& other);

        private:
            RoomEventPtr evt;
            index_t idx;
    };
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

            Q_PROPERTY(int timelineSize READ timelineSize NOTIFY addedMessages)
            Q_PROPERTY(QStringList memberNames READ memberNames NOTIFY memberListChanged)
            Q_PROPERTY(int memberCount READ memberCount NOTIFY memberListChanged)

            Q_PROPERTY(bool displayed READ displayed WRITE setDisplayed NOTIFY displayedChanged)
            Q_PROPERTY(QString firstDisplayedEventId READ firstDisplayedEventId WRITE setFirstDisplayedEventId NOTIFY firstDisplayedEventChanged)
            Q_PROPERTY(QString lastDisplayedEventId READ lastDisplayedEventId WRITE setLastDisplayedEventId NOTIFY lastDisplayedEventChanged)

            Q_PROPERTY(QString readMarkerEventId READ readMarkerEventId WRITE markMessagesAsRead NOTIFY readMarkerMoved)
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

            Q_INVOKABLE bool hasUnreadMessages();

            Q_INVOKABLE int notificationCount() const;
            Q_INVOKABLE void resetNotificationCount();
            Q_INVOKABLE int highlightCount() const;
            Q_INVOKABLE void resetHighlightCount();

            Q_INVOKABLE QUrl urlToThumbnail(const QString& eventId);
            Q_INVOKABLE QUrl urlToDownload(const QString& eventId);
            Q_INVOKABLE QString fileNameToDownload(const QString& eventId);
            Q_INVOKABLE FileTransferInfo fileTransferInfo(const QString& id) const;

            /** Pretty-prints plain text into HTML
             * This includes HTML escaping of <,>,",& and URLs linkification.
             */
            QString prettyPrint(const QString& plainText) const;

            MemberSorter memberSorter() const;

            QJsonObject toJson() const;
            void updateData(SyncRoomData&& data );
            void setJoinState( JoinState state );

        public slots:
            void postMessage(const QString& plainText,
                             MessageEventType type = MessageEventType::Text);
            void postMessage(const RoomMessageEvent& event);
            /** @deprecated If you have a custom event type, construct the event
             * and pass it as a whole to postMessage() */
            void postMessage(const QString& type, const QString& plainText);
            void setName(const QString& newName);
            void setCanonicalAlias(const QString& newAlias);
            void setTopic(const QString& newTopic);

            void getPreviousContent(int limit = 10);

            void inviteToRoom(const QString& memberId);
            LeaveRoomJob* leaveRoom();
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
            void memberRenamed(User* user);
            void memberListChanged();

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

            void replacedEvent(const RoomEvent* newEvent,
                               const RoomEvent* oldEvent);

            void newFileTransfer(QString id, QUrl localFile);
            void fileTransferProgress(QString id, qint64 progress, qint64 total);
            void fileTransferCompleted(QString id, QUrl localFile, QUrl mxcUrl);
            void fileTransferFailed(QString id, QString errorMessage = {});
            void fileTransferCancelled(QString id);

        protected:
            virtual void processStateEvents(const RoomEvents& events);
            virtual void processEphemeralEvent(EventPtr event);
            virtual void onAddNewTimelineEvents(timeline_iter_t from) { }
            virtual void onAddHistoricalTimelineEvents(rev_iter_t from) { }
            virtual void onRedaction(const RoomEvent* prevEvent,
                                     const RoomEvent* after) { }

        private:
            class Private;
            Private* d;
    };

    class MemberSorter
    {
        public:
            explicit MemberSorter(const Room* r) : room(r) { }

            bool operator()(User* u1, User* u2) const;

            template <typename ContT>
            typename ContT::size_type lowerBoundIndex(const ContT& c,
                                                      typename ContT::value_type v) const
            {
                return  std::lower_bound(c.begin(), c.end(), v, *this) - c.begin();
            }

        private:
            const Room* room;
    };
}  // namespace QMatrixClient
Q_DECLARE_METATYPE(QMatrixClient::FileTransferInfo)
