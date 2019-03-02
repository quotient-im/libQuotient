/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"

#include "converters.h"
#include "csapi/definitions/wellknown/full.h"

namespace QMatrixClient {
    // Operations

    /// Gets Matrix server discovery information about the domain.
    ///
    /// Gets discovery information about the domain. The file may include
    /// additional keys, which MUST follow the Java package naming convention,
    /// e.g. ``com.example.myapp.property``. This ensures property names are
    /// suitably namespaced for each application and reduces the risk of
    /// clashes.
    ///
    /// Note that this endpoint is not necessarily handled by the homeserver,
    /// but by another webserver, to be used for discovering the homeserver URL.
    class GetWellknownJob : public BaseJob
    {
        public:
        explicit GetWellknownJob();

        /*! Construct a URL without creating a full-fledged job object
         *
         * This function can be used when a URL for
         * GetWellknownJob is necessary but the job
         * itself isn't.
         */
        static QUrl makeRequestUrl(QUrl baseUrl);

        ~GetWellknownJob() override;

        // Result properties

        /// Server discovery information.
        const DiscoveryInformation& data() const;

        protected:
        Status parseJson(const QJsonDocument& data) override;

        private:
        class Private;
        QScopedPointer<Private> d;
    };
} // namespace QMatrixClient
