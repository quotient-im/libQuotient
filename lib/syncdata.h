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
    class SyncRoomData
    {
        public:
            QString roomId;
            JoinState joinState;
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
            void parseJson(const QJsonObject& json);

            Events&& takePresenceData();
            Events&& takeAccountData();
            Events&& takeToDeviceEvents();
            SyncDataList&& takeRoomData();

            QString nextBatch() const { return nextBatch_; }

            QStringList unresolvedRooms() const { return unresolvedRoomIds; }

            static std::pair<int, int> cacheVersion() { return { 8, 0 }; }

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
