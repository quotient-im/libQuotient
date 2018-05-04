/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "../basejob.h"

#include <QtCore/QJsonObject>
#include <QtCore/QVector>

#include "converters.h"

namespace QMatrixClient
{
    // Operations

    class CreateRoomJob : public BaseJob
    {
        public:
            // Inner data structures

            struct Invite3pid
            {
                QString idServer;
                QString medium;
                QString address;
            };

            struct StateEvent
            {
                QString type;
                QString stateKey;
                QJsonObject content;
            };

            // Construction/destruction

            explicit CreateRoomJob(const QString& visibility = {}, const QString& roomAliasName = {}, const QString& name = {}, const QString& topic = {}, const QVector<QString>& invite = {}, const QVector<Invite3pid>& invite3pid = {}, const QJsonObject& creationContent = {}, const QVector<StateEvent>& initialState = {}, const QString& preset = {}, bool isDirect = {}, bool guestCanJoin = {});
            ~CreateRoomJob() override;

            // Result properties

            const QString& roomId() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };
} // namespace QMatrixClient
