/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "../basejob.h"

#include <QtCore/QVector>

#include "converters.h"

namespace QMatrixClient
{
    // Operations

    class GetPushersJob : public BaseJob
    {
        public:
            // Inner data structures

            struct PusherData
            {
                QString url;
            };

            struct Pusher
            {
                QString pushkey;
                QString kind;
                QString appId;
                QString appDisplayName;
                QString deviceDisplayName;
                QString profileTag;
                QString lang;
                PusherData data;
            };

            // End of inner data structures

            /** Construct a URL out of baseUrl and usual parameters passed to
             * GetPushersJob. This function can be used when
             * a URL for GetPushersJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl);

            explicit GetPushersJob();
            ~GetPushersJob() override;

            const QVector<Pusher>& pushers() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };

    class PostPusherJob : public BaseJob
    {
        public:
            // Inner data structures

            struct PusherData
            {
                QString url;
            };

            // End of inner data structures

            explicit PostPusherJob(const QString& pushkey, const QString& kind, const QString& appId, const QString& appDisplayName, const QString& deviceDisplayName, const QString& lang, const PusherData& data, const QString& profileTag = {}, bool append = {});
    };
} // namespace QMatrixClient
