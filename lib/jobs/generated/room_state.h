/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "../basejob.h"

#include <QtCore/QJsonObject>


namespace QMatrixClient
{
    // Operations

    class SetRoomStateWithKeyJob : public BaseJob
    {
        public:
            explicit SetRoomStateWithKeyJob(const QString& roomId, const QString& eventType, const QString& stateKey, const QJsonObject& body = {});
            ~SetRoomStateWithKeyJob() override;

            const QString& eventId() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };

    class SetRoomStateJob : public BaseJob
    {
        public:
            explicit SetRoomStateJob(const QString& roomId, const QString& eventType, const QJsonObject& body = {});
            ~SetRoomStateJob() override;

            const QString& eventId() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };
} // namespace QMatrixClient
