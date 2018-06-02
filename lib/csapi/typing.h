/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"

#include "converters.h"

namespace QMatrixClient
{
    // Operations

    class SetTypingJob : public BaseJob
    {
        public:
            explicit SetTypingJob(const QString& userId, const QString& roomId, bool typing, Omittable<int> timeout = none);
    };
} // namespace QMatrixClient
