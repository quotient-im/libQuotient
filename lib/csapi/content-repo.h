/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"

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
            explicit GetContentJob(const QString& serverName, const QString& mediaId);

            /** Construct a URL out of baseUrl and usual parameters passed to
             * GetContentJob. This function can be used when
             * a URL for GetContentJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl, const QString& serverName, const QString& mediaId);

            ~GetContentJob() override;

            // Result properties

            const QString& contentType() const;
            const QString& contentDisposition() const;
            QIODevice* content() const;

        protected:
            Status parseReply(QNetworkReply* reply) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };

    class GetContentOverrideNameJob : public BaseJob
    {
        public:
            explicit GetContentOverrideNameJob(const QString& serverName, const QString& mediaId, const QString& fileName);

            /** Construct a URL out of baseUrl and usual parameters passed to
             * GetContentOverrideNameJob. This function can be used when
             * a URL for GetContentOverrideNameJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl, const QString& serverName, const QString& mediaId, const QString& fileName);

            ~GetContentOverrideNameJob() override;

            // Result properties

            const QString& contentType() const;
            const QString& contentDisposition() const;
            QIODevice* content() const;

        protected:
            Status parseReply(QNetworkReply* reply) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };

    class GetContentThumbnailJob : public BaseJob
    {
        public:
            explicit GetContentThumbnailJob(const QString& serverName, const QString& mediaId, int width = {}, int height = {}, const QString& method = {});

            /** Construct a URL out of baseUrl and usual parameters passed to
             * GetContentThumbnailJob. This function can be used when
             * a URL for GetContentThumbnailJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl, const QString& serverName, const QString& mediaId, int width = {}, int height = {}, const QString& method = {});

            ~GetContentThumbnailJob() override;

            // Result properties

            const QString& contentType() const;
            QIODevice* content() const;

        protected:
            Status parseReply(QNetworkReply* reply) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };

    class GetUrlPreviewJob : public BaseJob
    {
        public:
            explicit GetUrlPreviewJob(const QString& url, qint64 ts = {});

            /** Construct a URL out of baseUrl and usual parameters passed to
             * GetUrlPreviewJob. This function can be used when
             * a URL for GetUrlPreviewJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl, const QString& url, qint64 ts = {});

            ~GetUrlPreviewJob() override;

            // Result properties

            qint64 matrixImageSize() const;
            const QString& ogImage() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };
} // namespace QMatrixClient
