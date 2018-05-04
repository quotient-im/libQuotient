/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"



namespace QMatrixClient
{
    // Operations

    class InviteUserJob : public BaseJob
    {
        public:
            explicit InviteUserJob(const QString& roomId, const QString& userId);
    };
} // namespace QMatrixClient
