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

#ifndef QMATRIXCLIENT_SYNCJOB_H
#define QMATRIXCLIENT_SYNCJOB_H

#include "basejob.h"

#include "../joinstate.h"
#include "../events/event.h"

namespace QMatrixClient
{
    /**
     * @brief A crude wrapper around a container of pointers that owns pointers
     * to contained objects
     *
     * Similar to vector<unique_ptr<>>, upon deletion, EventsHolder
     * will delete all events contained in it.
     */
    template <typename ContainerT>
    class Owning : public ContainerT
    {
        public:
            Owning() = default;
            Owning(Owning&) = delete;
            Owning(Owning&& other) : ContainerT(std::move(other)) { }
            ~Owning() { for (auto e: *this) delete e; }

            /**
             * @brief returns the underlying events and releases the ownership
             *
             * Acts similar to unique_ptr::release.
             */
            ContainerT release() { return std::move(*this); }
    };

    class SyncRoomData
    {
    public:
        class EventList : public Owning<Events>
        {
            private:
                QString jsonKey;
            public:
                explicit EventList(QString k) : jsonKey(k) { }
                void fromJson(const QJsonObject& roomContents);
        };

        QString roomId;
        JoinState joinState;
        EventList state;
        EventList timeline;
        EventList ephemeral;
        EventList accountData;
        EventList inviteState;

        bool timelineLimited;
        QString timelinePrevBatch;
        int highlightCount;
        int notificationCount;

        SyncRoomData(QString roomId_ = QString(),
                     JoinState joinState_ = JoinState::Join,
                     const QJsonObject& room_ = QJsonObject());
    };
}
Q_DECLARE_TYPEINFO(QMatrixClient::SyncRoomData, Q_MOVABLE_TYPE);

namespace QMatrixClient
{
    // QVector cannot work with non-copiable objects, std::vector can.
    using SyncData = std::vector<SyncRoomData>;

    class ConnectionData;
    class SyncJob: public BaseJob
    {
        public:
            SyncJob(ConnectionData* connection, QString since=QString());
            virtual ~SyncJob();
            
            void setFilter(QString filter);
            void setFullState(bool full);
            void setPresence(QString presence);
            void setTimeout(int timeout);

            SyncData& roomData();
            QString nextBatch() const;

        protected:
            QString apiPath() const override;
            QUrlQuery query() const override;
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            Private* d;
    };
}

#endif // QMATRIXCLIENT_SYNCJOB_H
