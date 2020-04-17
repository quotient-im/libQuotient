/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "converters.h"

#include "jobs/basejob.h"

#include <QtCore/QHash>
#include <QtCore/QJsonObject>

namespace Quotient {

// Operations

/*! \brief Gets information about the server's capabilities.
 *
 * Gets information about the server's supported feature set
 * and other relevant capabilities.
 */
class GetCapabilitiesJob : public BaseJob {
public:
    // Inner data structures

    /// Capability to indicate if the user can change their password.
    struct ChangePasswordCapability {
        /// True if the user can change their password, false otherwise.
        bool enabled;
    };

    /// The room versions the server supports.
    struct RoomVersionsCapability {
        /// The default room version the server is using for new rooms.
        QString defaultVersion;
        /// A detailed description of the room versions the server supports.
        QHash<QString, QString> available;
    };

    /// The custom capabilities the server supports, using the
    /// Java package naming convention.
    struct Capabilities {
        /// Capability to indicate if the user can change their password.
        Omittable<ChangePasswordCapability> changePassword;
        /// The room versions the server supports.
        Omittable<RoomVersionsCapability> roomVersions;
        /// The custom capabilities the server supports, using the
        /// Java package naming convention.
        QHash<QString, QJsonObject> additionalProperties;
    };

    // Construction/destruction

    /// Gets information about the server's capabilities.
    explicit GetCapabilitiesJob();

    /*! \brief Construct a URL without creating a full-fledged job object
     *
     * This function can be used when a URL for GetCapabilitiesJob
     * is necessary but the job itself isn't.
     */
    static QUrl makeRequestUrl(QUrl baseUrl);
    ~GetCapabilitiesJob() override;

    // Result properties

    /// The custom capabilities the server supports, using the
    /// Java package naming convention.
    const Capabilities& capabilities() const;

protected:
    Status parseJson(const QJsonDocument& data) override;

private:
    class Private;
    QScopedPointer<Private> d;
};

} // namespace Quotient
