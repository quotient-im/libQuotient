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

#include "basejob.h"

#include "connectiondata.h"

namespace QMatrixClient
{
    class SendEventJob: public BaseJob
    {
        public:
            /** Constructs a job that sends an arbitrary room event */
            template <typename EvT>
            SendEventJob(const QString& roomId, const EvT& event)
                : BaseJob(HttpVerb::Put, QStringLiteral("SendEventJob"),
                          QStringLiteral("_matrix/client/r0/rooms/%1/send/%2/")
                              .arg(roomId, EvT::TypeId), // See also beforeStart()
                          Query(),
                          Data(event.toJson()))
            { }

            /**
             * Constructs a plain text message job (for compatibility with
             * the old PostMessageJob API).
             */
            SendEventJob(const QString& roomId, const QString& type,
                         const QString& plainText);

            QString eventId() const { return _eventId; }

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            QString _eventId;

            void beforeStart(const ConnectionData* connData) override;
    };
}  // namespace QMatrixClient
