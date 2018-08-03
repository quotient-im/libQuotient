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

#include "event.h"

#include <QtCore/QDateTime>

namespace QMatrixClient
{
    class RedactionEvent;

    /** This class corresponds to m.room.* events */
    class RoomEvent : public Event
    {
            Q_GADGET
            Q_PROPERTY(QString id READ id)
            Q_PROPERTY(QDateTime timestamp READ timestamp CONSTANT)
            Q_PROPERTY(QString roomId READ roomId CONSTANT)
            Q_PROPERTY(QString senderId READ senderId CONSTANT)
            Q_PROPERTY(QString redactionReason READ redactionReason)
            Q_PROPERTY(bool isRedacted READ isRedacted)
            Q_PROPERTY(QString transactionId READ transactionId WRITE setTransactionId)
        public:
            using factory_t = EventFactory<RoomEvent>;

            // RedactionEvent is an incomplete type here so we cannot inline
            // constructors and destructors and we cannot use 'using'.
            RoomEvent(Type type, event_mtype_t matrixType,
                      const QJsonObject& contentJson = {});
            RoomEvent(Type type, const QJsonObject& json);
            ~RoomEvent() override;

            QString id() const;
            QDateTime timestamp() const;
            QString roomId() const;
            QString senderId() const;
            bool isRedacted() const { return bool(_redactedBecause); }
            const event_ptr_tt<RedactionEvent>& redactedBecause() const
            {
                return _redactedBecause;
            }
            QString redactionReason() const;
            QString transactionId() const;
            QString stateKey() const;

            /**
             * Sets the transaction id for locally created events. This should be
             * done before the event is exposed to any code using the respective
             * Q_PROPERTY.
             *
             * \param txnId - transaction id, normally obtained from
             * Connection::generateTxnId()
             */
            void setTransactionId(const QString& txnId);

            /**
             * Sets event id for locally created events
             *
             * When a new event is created locally, it has no server id yet.
             * This function allows to add the id once the confirmation from
             * the server is received. There should be no id set previously
             * in the event. It's the responsibility of the code calling addId()
             * to notify clients that use Q_PROPERTY(id) about its change
             */
            void addId(const QString& newId);

        private:
            event_ptr_tt<RedactionEvent> _redactedBecause;
    };
    using RoomEventPtr = event_ptr_tt<RoomEvent>;
    using RoomEvents = EventsArray<RoomEvent>;
    using RoomEventsRange = Range<RoomEvents>;
} // namespace QMatrixClient
Q_DECLARE_METATYPE(QMatrixClient::RoomEvent*)
Q_DECLARE_METATYPE(const QMatrixClient::RoomEvent*)
