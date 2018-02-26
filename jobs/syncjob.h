/******************************************************************************
 * Copyright (C) 2016 Felix Rohrbach <kde@fxrh.de>
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

#include "basejob.h"

#include "joinstate.h"
#include "events/event.h"
#include "util.h"

namespace QMatrixClient
{
    template <typename EventT>
    class SyncBatch : public EventsBatch<EventT>
    {
        public:
            explicit SyncBatch(QString k) : jsonKey(std::move(k)) { }
            void fromJson(const QJsonObject& roomContents)
            {
                EventsBatch<EventT>::fromJson(
                            roomContents[jsonKey].toObject(), "events");
            }

        private:
            QString jsonKey;
    };

    class SyncRoomData
    {
        public:
            QString roomId;
            JoinState joinState;
            SyncBatch<RoomEvent> state;
            SyncBatch<RoomEvent> timeline;
            SyncBatch<Event> ephemeral;
            SyncBatch<Event> accountData;

            bool timelineLimited;
            QString timelinePrevBatch;
            int highlightCount;
            int notificationCount;

            SyncRoomData(const QString& roomId, JoinState joinState_,
                         const QJsonObject& room_);
            SyncRoomData(SyncRoomData&&) = default;
            SyncRoomData& operator=(SyncRoomData&&) = default;
    };
    // QVector cannot work with non-copiable objects, std::vector can.
    using SyncDataList = std::vector<SyncRoomData>;

    class SyncData
    {
        public:
            BaseJob::Status parseJson(const QJsonDocument &data);
            SyncBatch<Event>&& takeAccountData();
            SyncDataList&& takeRoomData();
            QString nextBatch() const;

        private:
            QString nextBatch_;
            SyncBatch<Event> accountData { "account_data" };
            SyncDataList roomData;
    };

    class SyncJob: public BaseJob
    {
        public:
            explicit SyncJob(const QString& since = {},
                             const QString& filter = {},
                             int timeout = -1, const QString& presence = {});

            SyncData &&takeData() { return std::move(d); }

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            SyncData d;
    };
}  // namespace QMatrixClient
