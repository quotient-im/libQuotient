/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"

#include <QtCore/QHash>
#include "converters.h"
#include <QtCore/QVector>

namespace QMatrixClient
{
    // Operations

    class GetWhoIsJob : public BaseJob
    {
        public:
            // Inner data structures

            struct ConnectionInfo
            {
                QString ip;
                qint64 lastSeen;
                QString userAgent;
            };

            struct SessionInfo
            {
                QVector<ConnectionInfo> connections;
            };

            struct DeviceInfo
            {
                QVector<SessionInfo> sessions;
            };

            // Construction/destruction

            explicit GetWhoIsJob(const QString& userId);

            /** Construct a URL out of baseUrl and usual parameters passed to
             * GetWhoIsJob. This function can be used when
             * a URL for GetWhoIsJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl, const QString& userId);

            ~GetWhoIsJob() override;

            // Result properties

            const QString& userId() const;
            const QHash<QString, DeviceInfo>& devices() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };
} // namespace QMatrixClient
