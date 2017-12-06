/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */


#pragma once

#include "../basejob.h"

#include <QtCore/QString>


namespace QMatrixClient
{
    // Operations

    class LeaveRoomJob : public BaseJob
    {
        public:
            explicit LeaveRoomJob(const QString& roomId);
    };

    class ForgetRoomJob : public BaseJob
    {
        public:
            explicit ForgetRoomJob(const QString& roomId);
    };
} // namespace QMatrixClient
