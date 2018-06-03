/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"

#include "converters.h"
#include <QtCore/QIODevice>

namespace QMatrixClient
{
    // Operations

    class UploadContentJob : public BaseJob
    {
        public:
            explicit UploadContentJob(QIODevice* content, const QString& filename = {}, const QString& contentType = {});
            ~UploadContentJob() override;

            // Result properties

            const QString& contentUri() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };

    class GetContentJob : public BaseJob
    {
        public:
            explicit GetContentJob(const QString& serverName, const QString& mediaId, bool allowRemote = true);

            /** Construct a URL out of baseUrl and usual parameters passed to
             * GetContentJob. This function can be used when
             * a URL for GetContentJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl, const QString& serverName, const QString& mediaId, bool allowRemote = true);

            ~GetContentJob() override;

            // Result properties

            const QString& contentType() const;
            const QString& contentDisposition() const;
            QIODevice* data() const;

        protected:
            Status parseReply(QNetworkReply* reply) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };

    class GetContentOverrideNameJob : public BaseJob
    {
        public:
            explicit GetContentOverrideNameJob(const QString& serverName, const QString& mediaId, const QString& fileName, bool allowRemote = true);

            /** Construct a URL out of baseUrl and usual parameters passed to
             * GetContentOverrideNameJob. This function can be used when
             * a URL for GetContentOverrideNameJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl, const QString& serverName, const QString& mediaId, const QString& fileName, bool allowRemote = true);

            ~GetContentOverrideNameJob() override;

            // Result properties

            const QString& contentType() const;
            const QString& contentDisposition() const;
            QIODevice* data() const;

        protected:
            Status parseReply(QNetworkReply* reply) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };

    class GetContentThumbnailJob : public BaseJob
    {
        public:
            explicit GetContentThumbnailJob(const QString& serverName, const QString& mediaId, Omittable<int> width = none, Omittable<int> height = none, const QString& method = {}, bool allowRemote = true);

            /** Construct a URL out of baseUrl and usual parameters passed to
             * GetContentThumbnailJob. This function can be used when
             * a URL for GetContentThumbnailJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl, const QString& serverName, const QString& mediaId, Omittable<int> width = none, Omittable<int> height = none, const QString& method = {}, bool allowRemote = true);

            ~GetContentThumbnailJob() override;

            // Result properties

            const QString& contentType() const;
            QIODevice* data() const;

        protected:
            Status parseReply(QNetworkReply* reply) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };

    class GetUrlPreviewJob : public BaseJob
    {
        public:
            explicit GetUrlPreviewJob(const QString& url, Omittable<qint64> ts = none);

            /** Construct a URL out of baseUrl and usual parameters passed to
             * GetUrlPreviewJob. This function can be used when
             * a URL for GetUrlPreviewJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl, const QString& url, Omittable<qint64> ts = none);

            ~GetUrlPreviewJob() override;

            // Result properties

            Omittable<qint64> matrixImageSize() const;
            const QString& ogImage() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };
} // namespace QMatrixClient
