/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "../basejob.h"

#include <QtCore/QJsonObject>

#include "converters.h"

namespace QMatrixClient
{
    // Operations

    class JoinRoomByIdJob : public BaseJob
    {
        public:
            // Inner data structures

            struct ThirdPartySigned
            {
                QString sender;
                QString mxid;
                QString token;
                QJsonObject signatures;
            };

            // End of inner data structures

            explicit JoinRoomByIdJob(const QString& roomId, const ThirdPartySigned& thirdPartySigned = {});
            ~JoinRoomByIdJob() override;

            const QString& roomId() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };

    class JoinRoomJob : public BaseJob
    {
        public:
            // Inner data structures

            struct Signed
            {
                QString sender;
                QString mxid;
                QString token;
                QJsonObject signatures;
            };

            struct ThirdPartySigned
            {
                Signed signedData;
            };

            // End of inner data structures

            explicit JoinRoomJob(const QString& roomIdOrAlias, const ThirdPartySigned& thirdPartySigned = {});
            ~JoinRoomJob() override;

            const QString& roomId() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };
} // namespace QMatrixClient
