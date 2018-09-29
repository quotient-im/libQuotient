/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"


namespace QMatrixClient
{
    // Operations

    /// Gets the versions of the specification supported by the server.
    ///
    /// Gets the versions of the specification supported by the server.
    /// 
    /// Values will take the form ``rX.Y.Z``.
    /// 
    /// Only the latest ``Z`` value will be reported for each supported ``X.Y`` value.
    /// i.e. if the server implements ``r0.0.0``, ``r0.0.1``, and ``r1.2.0``, it will report ``r0.0.1`` and ``r1.2.0``.
    class GetVersionsJob : public BaseJob
    {
        public:
            explicit GetVersionsJob();

            /*! Construct a URL without creating a full-fledged job object
             *
             * This function can be used when a URL for
             * GetVersionsJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl);

            ~GetVersionsJob() override;

            // Result properties

            /// The supported versions.
            const QStringList& versions() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };
} // namespace QMatrixClient
