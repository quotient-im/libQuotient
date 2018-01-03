/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "../basejob.h"

#include <QtCore/QString>


namespace QMatrixClient
{
    // Operations

    class SetTypingJob : public BaseJob
    {
        public:
            explicit SetTypingJob(const QString& userId, const QString& roomId, bool typing, int timeout = {});
    };
} // namespace QMatrixClient
