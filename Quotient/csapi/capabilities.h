// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#pragma once

#include <Quotient/jobs/basejob.h>

namespace Quotient {

//! \brief Gets information about the server's capabilities.
//!
//! Gets information about the server's supported feature set
//! and other relevant capabilities.
class QUOTIENT_API GetCapabilitiesJob : public BaseJob {
public:
    // Inner data structures

    //! Capability to indicate if the user can change their password.
    struct QUOTIENT_API ChangePasswordCapability {
        //! True if the user can change their password, false otherwise.
        bool enabled;
    };

    //! The room versions the server supports.
    struct QUOTIENT_API RoomVersionsCapability {
        //! The default room version the server is using for new rooms.
        QString defaultVersion;

        //! A detailed description of the room versions the server supports.
        QHash<QString, QString> available;
    };

    //! The custom capabilities the server supports, using the
    //! Java package naming convention.
    struct QUOTIENT_API Capabilities {
        //! Capability to indicate if the user can change their password.
        std::optional<ChangePasswordCapability> changePassword{};

        //! The room versions the server supports.
        std::optional<RoomVersionsCapability> roomVersions{};

        QHash<QString, QJsonObject> additionalProperties{};
    };

    // Construction/destruction

    explicit GetCapabilitiesJob();

    //! \brief Construct a URL without creating a full-fledged job object
    //!
    //! This function can be used when a URL for GetCapabilitiesJob
    //! is necessary but the job itself isn't.
    static QUrl makeRequestUrl(const HomeserverData& hsData);

    // Result properties

    //! The custom capabilities the server supports, using the
    //! Java package naming convention.
    Capabilities capabilities() const { return loadFromJson<Capabilities>("capabilities"_L1); }
};

inline auto collectResponse(const GetCapabilitiesJob* job) { return job->capabilities(); }

template <>
struct QUOTIENT_API JsonObjectConverter<GetCapabilitiesJob::ChangePasswordCapability> {
    static void fillFrom(const QJsonObject& jo, GetCapabilitiesJob::ChangePasswordCapability& result)
    {
        fillFromJson(jo.value("enabled"_L1), result.enabled);
    }
};

template <>
struct QUOTIENT_API JsonObjectConverter<GetCapabilitiesJob::RoomVersionsCapability> {
    static void fillFrom(const QJsonObject& jo, GetCapabilitiesJob::RoomVersionsCapability& result)
    {
        fillFromJson(jo.value("default"_L1), result.defaultVersion);
        fillFromJson(jo.value("available"_L1), result.available);
    }
};

template <>
struct QUOTIENT_API JsonObjectConverter<GetCapabilitiesJob::Capabilities> {
    static void fillFrom(QJsonObject jo, GetCapabilitiesJob::Capabilities& result)
    {
        fillFromJson(jo.take("m.change_password"_L1), result.changePassword);
        fillFromJson(jo.take("m.room_versions"_L1), result.roomVersions);
        fromJson(jo, result.additionalProperties);
    }
};

} // namespace Quotient
