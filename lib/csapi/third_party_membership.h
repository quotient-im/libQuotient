/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"


namespace QMatrixClient
{
    // Operations

    class InviteBy3PIDJob : public BaseJob
    {
        public:
            explicit InviteBy3PIDJob(const QString& roomId, const QString& idServer, const QString& medium, const QString& address);
    };
} // namespace QMatrixClient
