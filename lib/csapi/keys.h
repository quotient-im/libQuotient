/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"

#include "csapi/definitions/device_keys.h"
#include <QtCore/QHash>
#include "converters.h"
#include <QtCore/QVariant>
#include <QtCore/QJsonObject>

namespace QMatrixClient
{
    // Operations

    class UploadKeysJob : public BaseJob
    {
        public:
            explicit UploadKeysJob(const Omittable<DeviceKeys>& deviceKeys = none, const QHash<QString, QVariant>& oneTimeKeys = {});
            ~UploadKeysJob() override;

            // Result properties

            const QHash<QString, int>& oneTimeKeyCounts() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };

    class QueryKeysJob : public BaseJob
    {
        public:
            // Inner data structures

            struct UnsignedDeviceInfo
            {
                QString deviceDisplayName;
            };

            struct DeviceInformation : DeviceKeys
            {
                Omittable<UnsignedDeviceInfo> unsignedData;
            };

            // Construction/destruction

            explicit QueryKeysJob(const QHash<QString, QStringList>& deviceKeys, Omittable<int> timeout = none, const QString& token = {});
            ~QueryKeysJob() override;

            // Result properties

            const QHash<QString, QJsonObject>& failures() const;
            const QHash<QString, QHash<QString, DeviceInformation>>& deviceKeys() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };

    class ClaimKeysJob : public BaseJob
    {
        public:
            explicit ClaimKeysJob(const QHash<QString, QHash<QString, QString>>& oneTimeKeys, Omittable<int> timeout = none);
            ~ClaimKeysJob() override;

            // Result properties

            const QHash<QString, QJsonObject>& failures() const;
            const QHash<QString, QHash<QString, QVariant>>& oneTimeKeys() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };

    class GetKeysChangesJob : public BaseJob
    {
        public:
            explicit GetKeysChangesJob(const QString& from, const QString& to);

            /** Construct a URL out of baseUrl and usual parameters passed to
             * GetKeysChangesJob. This function can be used when
             * a URL for GetKeysChangesJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl, const QString& from, const QString& to);

            ~GetKeysChangesJob() override;

            // Result properties

            const QStringList& changed() const;
            const QStringList& left() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };
} // namespace QMatrixClient
