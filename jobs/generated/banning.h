/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "../basejob.h"

#include <QtCore/QString>


namespace QMatrixClient
{
    // Operations

    class BanJob : public BaseJob
    {
        public:
            explicit BanJob(const QString& roomId, const QString& userId, const QString& reason = {});
    };

    class UnbanJob : public BaseJob
    {
        public:
            explicit UnbanJob(const QString& roomId, const QString& userId);
    };
} // namespace QMatrixClient
