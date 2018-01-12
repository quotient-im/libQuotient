/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "../basejob.h"



namespace QMatrixClient
{
    // Operations

    class KickJob : public BaseJob
    {
        public:
            explicit KickJob(const QString& roomId, const QString& userId, const QString& reason = {});
    };
} // namespace QMatrixClient
