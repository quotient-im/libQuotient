/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"

#include "csapi/definitions/auth_data.h"
#include <QtCore/QVector>
#include "converters.h"
#include "csapi/definitions/client_device.h"

namespace QMatrixClient
{
    // Operations

    /// List registered devices for the current user
    ///
    /// Gets information about all devices for the current user.
    class GetDevicesJob : public BaseJob
    {
        public:
            explicit GetDevicesJob();

            /*! Construct a URL without creating a full-fledged job object
             *
             * This function can be used when a URL for
             * GetDevicesJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl);

            ~GetDevicesJob() override;

            // Result properties

            /// A list of all registered devices for this user.
            const QVector<Device>& devices() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };

    /// Get a single device
    ///
    /// Gets information on a single device, by device id.
    class GetDeviceJob : public BaseJob
    {
        public:
            /*! Get a single device
             * \param deviceId
             *   The device to retrieve.
             */
            explicit GetDeviceJob(const QString& deviceId);

            /*! Construct a URL without creating a full-fledged job object
             *
             * This function can be used when a URL for
             * GetDeviceJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl, const QString& deviceId);

            ~GetDeviceJob() override;

            // Result properties

            /// Device information
            const Device& data() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };

    /// Update a device
    ///
    /// Updates the metadata on the given device.
    class UpdateDeviceJob : public BaseJob
    {
        public:
            /*! Update a device
             * \param deviceId
             *   The device to update.
             * \param displayName
             *   The new display name for this device. If not given, the
             *   display name is unchanged.
             */
            explicit UpdateDeviceJob(const QString& deviceId, const QString& displayName = {});
    };

    /// Delete a device
    ///
    /// This API endpoint uses the `User-Interactive Authentication API`_.
    /// 
    /// Deletes the given device, and invalidates any access token associated with it.
    class DeleteDeviceJob : public BaseJob
    {
        public:
            /*! Delete a device
             * \param deviceId
             *   The device to delete.
             * \param auth
             *   Additional authentication information for the
             *   user-interactive authentication API.
             */
            explicit DeleteDeviceJob(const QString& deviceId, const Omittable<AuthenticationData>& auth = none);
    };

    /// Bulk deletion of devices
    ///
    /// This API endpoint uses the `User-Interactive Authentication API`_.
    /// 
    /// Deletes the given devices, and invalidates any access token associated with them.
    class DeleteDevicesJob : public BaseJob
    {
        public:
            /*! Bulk deletion of devices
             * \param devices
             *   The list of device IDs to delete.
             * \param auth
             *   Additional authentication information for the
             *   user-interactive authentication API.
             */
            explicit DeleteDevicesJob(const QStringList& devices, const Omittable<AuthenticationData>& auth = none);
    };
} // namespace QMatrixClient
