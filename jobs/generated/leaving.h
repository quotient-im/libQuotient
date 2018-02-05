/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "../basejob.h"



namespace QMatrixClient
{
    // Operations

    class LeaveRoomJob : public BaseJob
    {
        public:
            /** Construct a URL out of baseUrl and usual parameters passed to
             * LeaveRoomJob. This function can be used when
             * a URL for LeaveRoomJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl, const QString& roomId);

            explicit LeaveRoomJob(const QString& roomId);
    };

    class ForgetRoomJob : public BaseJob
    {
        public:
            /** Construct a URL out of baseUrl and usual parameters passed to
             * ForgetRoomJob. This function can be used when
             * a URL for ForgetRoomJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl, const QString& roomId);

            explicit ForgetRoomJob(const QString& roomId);
    };
} // namespace QMatrixClient
