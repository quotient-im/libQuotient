/******************************************************************************
 * Copyright (C) 2018 Kitsune Ral <kitsune-ral@users.sf.net>
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

#include "joinstate.h"
#include "events/stateevent.h"

namespace QMatrixClient {
    /// Room summary, as defined in MSC688
    /**
     * Every member of this structure is an Omittable; as per the MSC, only
     * changed values are sent from the server so if nothing is in the payload
     * the respective member will be omitted. In particular, `heroes.omitted()`
     * means that nothing has come from the server; heroes.value().isEmpty()
     * means a peculiar case of a room with the only member - the current user.
     */
    struct RoomSummary
    {
        Omittable<int> joinedMemberCount;
        Omittable<int> invitedMemberCount;
        Omittable<QStringList> heroes; //< mxids of users to take part in the room name

        bool isEmpty() const;
        /// Merge the contents of another RoomSummary object into this one
        /// \return true, if the current object has changed; false otherwise
        bool merge(const RoomSummary& other);

        friend QDebug operator<<(QDebug dbg, const RoomSummary& rs);
    };

    template <>
    struct JsonObjectConverter<RoomSummary>
    {
        static void dumpTo(QJsonObject& jo, const RoomSummary& rs);
        static void fillFrom(const QJsonObject& jo, RoomSummary& rs);
    };

    class SyncRoomData
    {
        public:
            QString roomId;
            JoinState joinState;
            RoomSummary summary;
            StateEvents state;
            RoomEvents timeline;
            Events ephemeral;
            Events accountData;

            bool timelineLimited;
            QString timelinePrevBatch;
            int unreadCount;
            int highlightCount;
            int notificationCount;

            SyncRoomData(const QString& roomId, JoinState joinState_,
                         const QJsonObject& room_);
            SyncRoomData(SyncRoomData&&) = default;
            SyncRoomData& operator=(SyncRoomData&&) = default;

            static const QString UnreadCountKey;
    };

    // QVector cannot work with non-copiable objects, std::vector can.
    using SyncDataList = std::vector<SyncRoomData>;

    class SyncData
    {
        public:
            SyncData() = default;
            explicit SyncData(const QString& cacheFileName);
            /** Parse sync response into room events
             * \param json response from /sync or a room state cache
             * \return the list of rooms with missing cache files; always
             *         empty when parsing response from /sync
             */
            void parseJson(const QJsonObject& json, const QString& baseDir = {});

            Events&& takePresenceData();
            Events&& takeAccountData();
            Events&& takeToDeviceEvents();
            SyncDataList&& takeRoomData();

            QString nextBatch() const { return nextBatch_; }

            QStringList unresolvedRooms() const { return unresolvedRoomIds; }

            static std::pair<int, int> cacheVersion() { return { 10, 0 }; }
            static QString fileNameForRoom(QString roomId);

        private:
            QString nextBatch_;
            Events presenceData;
            Events accountData;
            Events toDeviceEvents;
            SyncDataList roomData;
            QStringList unresolvedRoomIds;

            static QJsonObject loadJson(const QString& fileName);
    };
}  // namespace QMatrixClient
