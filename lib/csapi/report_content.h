/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"

#include "converters.h"

namespace QMatrixClient
{
    // Operations

    class ReportContentJob : public BaseJob
    {
        public:
            explicit ReportContentJob(const QString& roomId, const QString& eventId, int score, const QString& reason);
    };
} // namespace QMatrixClient
