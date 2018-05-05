/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"

#include <QtCore/QJsonObject>


namespace QMatrixClient
{
    // Operations

    class SetAccountDataJob : public BaseJob
    {
        public:
            explicit SetAccountDataJob(const QString& userId, const QString& type, const QJsonObject& content = {});
    };

    class SetAccountDataPerRoomJob : public BaseJob
    {
        public:
            explicit SetAccountDataPerRoomJob(const QString& userId, const QString& roomId, const QString& type, const QJsonObject& content = {});
    };
} // namespace QMatrixClient
