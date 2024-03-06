// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#pragma once

#include <Quotient/csapi/definitions/auth_data.h>
#include <Quotient/csapi/definitions/client_device.h>

#include <Quotient/jobs/basejob.h>

namespace Quotient {

//! \brief List registered devices for the current user
//!
//! Gets information about all devices for the current user.
class QUOTIENT_API GetDevicesJob : public BaseJob {
public:
    explicit GetDevicesJob();

    //! \brief Construct a URL without creating a full-fledged job object
    //!
    //! This function can be used when a URL for GetDevicesJob
    //! is necessary but the job itself isn't.
    static QUrl makeRequestUrl(QUrl baseUrl);

    // Result properties

    //! A list of all registered devices for this user.
    QVector<Device> devices() const { return loadFromJson<QVector<Device>>("devices"_ls); }
};

//! \brief Get a single device
//!
//! Gets information on a single device, by device id.
class QUOTIENT_API GetDeviceJob : public BaseJob {
public:
    //! \param deviceId
    //!   The device to retrieve.
    explicit GetDeviceJob(const QString& deviceId);

    //! \brief Construct a URL without creating a full-fledged job object
    //!
    //! This function can be used when a URL for GetDeviceJob
    //! is necessary but the job itself isn't.
    static QUrl makeRequestUrl(QUrl baseUrl, const QString& deviceId);

    // Result properties

    //! Device information
    Device device() const { return fromJson<Device>(jsonData()); }
};

//! \brief Update a device
//!
//! Updates the metadata on the given device.
class QUOTIENT_API UpdateDeviceJob : public BaseJob {
public:
    //! \param deviceId
    //!   The device to update.
    //!
    //! \param displayName
    //!   The new display name for this device. If not given, the
    //!   display name is unchanged.
    explicit UpdateDeviceJob(const QString& deviceId, const QString& displayName = {});
};

//! \brief Delete a device
//!
//! This API endpoint uses the [User-Interactive Authentication
//! API](/client-server-api/#user-interactive-authentication-api).
//!
//! Deletes the given device, and invalidates any access token associated with it.
class QUOTIENT_API DeleteDeviceJob : public BaseJob {
public:
    //! \param deviceId
    //!   The device to delete.
    //!
    //! \param auth
    //!   Additional authentication information for the
    //!   user-interactive authentication API.
    explicit DeleteDeviceJob(const QString& deviceId,
                             const Omittable<AuthenticationData>& auth = none);
};

//! \brief Bulk deletion of devices
//!
//! This API endpoint uses the [User-Interactive Authentication
//! API](/client-server-api/#user-interactive-authentication-api).
//!
//! Deletes the given devices, and invalidates any access token associated with them.
class QUOTIENT_API DeleteDevicesJob : public BaseJob {
public:
    //! \param devices
    //!   The list of device IDs to delete.
    //!
    //! \param auth
    //!   Additional authentication information for the
    //!   user-interactive authentication API.
    explicit DeleteDevicesJob(const QStringList& devices,
                              const Omittable<AuthenticationData>& auth = none);
};

} // namespace Quotient
