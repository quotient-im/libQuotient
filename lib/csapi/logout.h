/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"


namespace QMatrixClient
{
    // Operations

    class LogoutJob : public BaseJob
    {
        public:
            explicit LogoutJob();

            /** Construct a URL out of baseUrl and usual parameters passed to
             * LogoutJob. This function can be used when
             * a URL for LogoutJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl);

    };
} // namespace QMatrixClient
