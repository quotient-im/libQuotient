/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"

#include <QtCore/QJsonObject>
#include <QtCore/QVector>
#include "converters.h"
#include "csapi/definitions/client_device.h"

namespace QMatrixClient
{
    // Operations

    class GetDevicesJob : public BaseJob
    {
        public:
            explicit GetDevicesJob();

            /** Construct a URL out of baseUrl and usual parameters passed to
             * GetDevicesJob. This function can be used when
             * a URL for GetDevicesJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl);

            ~GetDevicesJob() override;

            // Result properties

            const QVector<Device>& devices() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };

    class GetDeviceJob : public BaseJob
    {
        public:
            explicit GetDeviceJob(const QString& deviceId);

            /** Construct a URL out of baseUrl and usual parameters passed to
             * GetDeviceJob. This function can be used when
             * a URL for GetDeviceJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl, const QString& deviceId);

            ~GetDeviceJob() override;

            // Result properties

            const Device& data() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };

    class UpdateDeviceJob : public BaseJob
    {
        public:
            explicit UpdateDeviceJob(const QString& deviceId, const QString& displayName = {});
    };

    class DeleteDeviceJob : public BaseJob
    {
        public:
            explicit DeleteDeviceJob(const QString& deviceId, const QJsonObject& auth = {});
    };

    class DeleteDevicesJob : public BaseJob
    {
        public:
            explicit DeleteDevicesJob(const QStringList& devices, const QJsonObject& auth = {});
    };
} // namespace QMatrixClient
