/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"

#include <QtCore/QVector>

namespace QMatrixClient
{
    // Operations

    class GetAccount3PIDsJob : public BaseJob
    {
        public:
            // Inner data structures

            struct ThirdPartyIdentifier
            {
                QString medium;
                QString address;
            };

            // Construction/destruction

            explicit GetAccount3PIDsJob();

            /** Construct a URL out of baseUrl and usual parameters passed to
             * GetAccount3PIDsJob. This function can be used when
             * a URL for GetAccount3PIDsJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl);

            ~GetAccount3PIDsJob() override;

            // Result properties

            const QVector<ThirdPartyIdentifier>& threepids() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };

    class Post3PIDsJob : public BaseJob
    {
        public:
            // Inner data structures

            struct ThreePidCredentials
            {
                QString clientSecret;
                QString idServer;
                QString sid;
            };

            // Construction/destruction

            explicit Post3PIDsJob(const ThreePidCredentials& threePidCreds, bool bind = {});
    };

    class RequestTokenTo3PIDJob : public BaseJob
    {
        public:
            explicit RequestTokenTo3PIDJob();

            /** Construct a URL out of baseUrl and usual parameters passed to
             * RequestTokenTo3PIDJob. This function can be used when
             * a URL for RequestTokenTo3PIDJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl);

    };
} // namespace QMatrixClient
