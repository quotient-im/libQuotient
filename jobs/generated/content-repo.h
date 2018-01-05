/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "../basejob.h"

#include <QtCore/QByteArray>
#include <QtCore/QString>


namespace QMatrixClient
{
    // Operations

    class UploadContentJob : public BaseJob
    {
        public:
            explicit UploadContentJob(QByteArray content, const QString& filename = {}, const QString& contentType = {});
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
            explicit GetContentJob(const QString& serverName, const QString& mediaId);
            ~GetContentJob() override;

            const QString& contentType() const;
            const QString& contentDisposition() const;
            QByteArray content() const;

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
            ~GetContentOverrideNameJob() override;

            const QString& contentType() const;
            const QString& contentDisposition() const;
            QByteArray content() const;

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
            ~GetContentThumbnailJob() override;

            const QString& contentType() const;
            QByteArray content() const;

        protected:
            Status parseReply(QNetworkReply* reply) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };

    class GetUrlPreviewJob : public BaseJob
    {
        public:
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
