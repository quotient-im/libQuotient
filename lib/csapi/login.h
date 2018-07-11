/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"

#include "converters.h"
#include <QtCore/QVector>

namespace QMatrixClient
{
    // Operations

    class GetLoginFlowsJob : public BaseJob
    {
        public:
            // Inner data structures

            struct LoginFlow
            {
                QString type;
            };

            // Construction/destruction

            explicit GetLoginFlowsJob();

            /** Construct a URL out of baseUrl and usual parameters passed to
             * GetLoginFlowsJob. This function can be used when
             * a URL for GetLoginFlowsJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl);

            ~GetLoginFlowsJob() override;

            // Result properties

            const QVector<LoginFlow>& flows() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };

    class LoginJob : public BaseJob
    {
        public:
            explicit LoginJob(const QString& type, const QString& user = {}, const QString& medium = {}, const QString& address = {}, const QString& password = {}, const QString& token = {}, const QString& deviceId = {}, const QString& initialDeviceDisplayName = {});
            ~LoginJob() override;

            // Result properties

            const QString& userId() const;
            const QString& accessToken() const;
            const QString& homeServer() const;
            const QString& deviceId() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };
} // namespace QMatrixClient
