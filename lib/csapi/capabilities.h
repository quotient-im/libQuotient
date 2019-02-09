/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"

#include "converters.h"
#include <QtCore/QHash>

namespace QMatrixClient
{
    // Operations

    /// Gets information about the server's capabilities.
    ///
    /// Gets information about the server's supported feature set
    /// and other relevant capabilities.
    class GetCapabilitiesJob : public BaseJob
    {
        public:
            // Inner data structures

            /// Capability to indicate if the user can change their password.
            struct ChangePasswordCapability
            {
                /// True if the user can change their password, false otherwise.
                bool enabled;
            };

            /// The room versions the server supports.
            struct RoomVersionsCapability
            {
                /// The default room version the server is using for new rooms.
                QString isDefault;
                /// A detailed description of the room versions the server supports.
                QHash<QString, QString> available;
            };

            // Construction/destruction

            explicit GetCapabilitiesJob();

            /*! Construct a URL without creating a full-fledged job object
             *
             * This function can be used when a URL for
             * GetCapabilitiesJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl);

            ~GetCapabilitiesJob() override;

            // Result properties

            /// Capability to indicate if the user can change their password.
            const Omittable<ChangePasswordCapability>& changePassword() const;
            /// The room versions the server supports.
            const Omittable<RoomVersionsCapability>& roomVersions() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };
} // namespace QMatrixClient
