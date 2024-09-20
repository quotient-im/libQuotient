// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#pragma once

#include <Quotient/jobs/basejob.h>

namespace Quotient {

struct QUOTIENT_API BooleanCapability {
    //! True if the user can perform the action, false otherwise.
    bool enabled;
};

template <>
struct JsonObjectConverter<BooleanCapability> {
    static void dumpTo(QJsonObject& jo, const BooleanCapability& pod)
    {
        addParam<>(jo, "enabled"_L1, pod.enabled);
    }
    static void fillFrom(const QJsonObject& jo, BooleanCapability& pod)
    {
        fillFromJson(jo.value("enabled"_L1), pod.enabled);
    }
};

//! \brief Gets information about the server's capabilities.
//!
//! Gets information about the server's supported feature set
//! and other relevant capabilities.
class QUOTIENT_API GetCapabilitiesJob : public BaseJob {
public:
    // Inner data structures

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
        std::optional<BooleanCapability> changePassword{};

        //! The room versions the server supports.
        std::optional<RoomVersionsCapability> roomVersions{};

        //! Capability to indicate if the user can change their display name.
        std::optional<BooleanCapability> setDisplayname{};

        //! Capability to indicate if the user can change their avatar.
        std::optional<BooleanCapability> setAvatarUrl{};

        //! Capability to indicate if the user can change 3PID associations on their account.
        std::optional<BooleanCapability> thirdPartyIdChanges{};

        //! Capability to indicate if the user can generate tokens to log further clients into their
        //! account.
        std::optional<BooleanCapability> getLoginToken{};

        //! Application-dependent keys using the
        //! [Common Namespaced Identifier
        //! Grammar](/appendices/#common-namespaced-identifier-grammar).
        QVariantHash additionalProperties{};
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
        fillFromJson(jo.take("m.set_displayname"_L1), result.setDisplayname);
        fillFromJson(jo.take("m.set_avatar_url"_L1), result.setAvatarUrl);
        fillFromJson(jo.take("m.3pid_changes"_L1), result.thirdPartyIdChanges);
        fillFromJson(jo.take("m.get_login_token"_L1), result.getLoginToken);
        fromJson(jo, result.additionalProperties);
    }
};

} // namespace Quotient
