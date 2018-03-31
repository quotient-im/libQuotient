/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "../basejob.h"

#include <QtCore/QIODevice>


namespace QMatrixClient
{
    // Operations

    class UploadContentJob : public BaseJob
    {
        public:
            explicit UploadContentJob(QIODevice* content, const QString& filename = {}, const QString& contentType = {});
            ~UploadContentJob() override;

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
            /** Construct a URL out of baseUrl and usual parameters passed to
             * GetContentJob. This function can be used when
             * a URL for GetContentJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl, const QString& serverName, const QString& mediaId);

            explicit GetContentJob(const QString& serverName, const QString& mediaId);
            ~GetContentJob() override;

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
            /** Construct a URL out of baseUrl and usual parameters passed to
             * GetContentOverrideNameJob. This function can be used when
             * a URL for GetContentOverrideNameJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl, const QString& serverName, const QString& mediaId, const QString& fileName);

            explicit GetContentOverrideNameJob(const QString& serverName, const QString& mediaId, const QString& fileName);
            ~GetContentOverrideNameJob() override;

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
            /** Construct a URL out of baseUrl and usual parameters passed to
             * GetContentThumbnailJob. This function can be used when
             * a URL for GetContentThumbnailJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl, const QString& serverName, const QString& mediaId, int width = {}, int height = {}, const QString& method = {});

            explicit GetContentThumbnailJob(const QString& serverName, const QString& mediaId, int width = {}, int height = {}, const QString& method = {});
            ~GetContentThumbnailJob() override;

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
            /** Construct a URL out of baseUrl and usual parameters passed to
             * GetUrlPreviewJob. This function can be used when
             * a URL for GetUrlPreviewJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl, const QString& url, double ts = {});

            explicit GetUrlPreviewJob(const QString& url, double ts = {});
            ~GetUrlPreviewJob() override;

            double matrixImageSize() const;
            const QString& ogImage() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };
} // namespace QMatrixClient
