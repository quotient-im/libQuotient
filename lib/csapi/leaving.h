/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"


namespace QMatrixClient
{
    // Operations

    class LeaveRoomJob : public BaseJob
    {
        public:
            explicit LeaveRoomJob(const QString& roomId);

            /** Construct a URL out of baseUrl and usual parameters passed to
             * LeaveRoomJob. This function can be used when
             * a URL for LeaveRoomJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl, const QString& roomId);

    };

    class ForgetRoomJob : public BaseJob
    {
        public:
            explicit ForgetRoomJob(const QString& roomId);

            /** Construct a URL out of baseUrl and usual parameters passed to
             * ForgetRoomJob. This function can be used when
             * a URL for ForgetRoomJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl, const QString& roomId);

    };
} // namespace QMatrixClient
