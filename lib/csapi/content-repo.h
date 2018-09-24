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

    /// Upload some content to the content repository.
    class UploadContentJob : public BaseJob
    {
        public:
            /*! Upload some content to the content repository.
             * \param content
             * \param filename
             *   The name of the file being uploaded
             * \param contentType
             *   The content type of the file being uploaded
             */
            explicit UploadContentJob(QIODevice* content, const QString& filename = {}, const QString& contentType = {});
            ~UploadContentJob() override;

            // Result properties

            /// The MXC URI to the uploaded content.
            const QString& contentUri() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };

    /// Download content from the content repository.
    class GetContentJob : public BaseJob
    {
        public:
            /*! Download content from the content repository.
             * \param serverName
             *   The server name from the ``mxc://`` URI (the authoritory component)
             * \param mediaId
             *   The media ID from the ``mxc://`` URI (the path component)
             * \param allowRemote
             *   Indicates to the server that it should not attempt to fetch the media if it is deemed
             *   remote. This is to prevent routing loops where the server contacts itself. Defaults to 
             *   true if not provided.
             */
            explicit GetContentJob(const QString& serverName, const QString& mediaId, bool allowRemote = true);

            /*! Construct a URL without creating a full-fledged job object
             *
             * This function can be used when a URL for
             * GetContentJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl, const QString& serverName, const QString& mediaId, bool allowRemote = true);

            ~GetContentJob() override;

            // Result properties

            /// The content type of the file that was previously uploaded.
            const QString& contentType() const;
            /// The name of the file that was previously uploaded, if set.
            const QString& contentDisposition() const;
            /// The content that was previously uploaded.
            QIODevice* data() const;

        protected:
            Status parseReply(QNetworkReply* reply) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };

    /// Download content from the content repository as a given filename.
    class GetContentOverrideNameJob : public BaseJob
    {
        public:
            /*! Download content from the content repository as a given filename.
             * \param serverName
             *   The server name from the ``mxc://`` URI (the authoritory component)
             * \param mediaId
             *   The media ID from the ``mxc://`` URI (the path component)
             * \param fileName
             *   The filename to give in the Content-Disposition
             * \param allowRemote
             *   Indicates to the server that it should not attempt to fetch the media if it is deemed
             *   remote. This is to prevent routing loops where the server contacts itself. Defaults to 
             *   true if not provided.
             */
            explicit GetContentOverrideNameJob(const QString& serverName, const QString& mediaId, const QString& fileName, bool allowRemote = true);

            /*! Construct a URL without creating a full-fledged job object
             *
             * This function can be used when a URL for
             * GetContentOverrideNameJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl, const QString& serverName, const QString& mediaId, const QString& fileName, bool allowRemote = true);

            ~GetContentOverrideNameJob() override;

            // Result properties

            /// The content type of the file that was previously uploaded.
            const QString& contentType() const;
            /// The name of file given in the request
            const QString& contentDisposition() const;
            /// The content that was previously uploaded.
            QIODevice* data() const;

        protected:
            Status parseReply(QNetworkReply* reply) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };

    /// Download a thumbnail of the content from the content repository.
    class GetContentThumbnailJob : public BaseJob
    {
        public:
            /*! Download a thumbnail of the content from the content repository.
             * \param serverName
             *   The server name from the ``mxc://`` URI (the authoritory component)
             * \param mediaId
             *   The media ID from the ``mxc://`` URI (the path component)
             * \param width
             *   The *desired* width of the thumbnail. The actual thumbnail may not
             *   match the size specified.
             * \param height
             *   The *desired* height of the thumbnail. The actual thumbnail may not
             *   match the size specified.
             * \param method
             *   The desired resizing method.
             * \param allowRemote
             *   Indicates to the server that it should not attempt to fetch the media if it is deemed
             *   remote. This is to prevent routing loops where the server contacts itself. Defaults to 
             *   true if not provided.
             */
            explicit GetContentThumbnailJob(const QString& serverName, const QString& mediaId, int width, int height, const QString& method = {}, bool allowRemote = true);

            /*! Construct a URL without creating a full-fledged job object
             *
             * This function can be used when a URL for
             * GetContentThumbnailJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl, const QString& serverName, const QString& mediaId, int width, int height, const QString& method = {}, bool allowRemote = true);

            ~GetContentThumbnailJob() override;

            // Result properties

            /// The content type of the thumbnail.
            const QString& contentType() const;
            /// A thumbnail of the requested content.
            QIODevice* data() const;

        protected:
            Status parseReply(QNetworkReply* reply) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };

    /// Get information about a URL for a client
    class GetUrlPreviewJob : public BaseJob
    {
        public:
            /*! Get information about a URL for a client
             * \param url
             *   The URL to get a preview of
             * \param ts
             *   The preferred point in time to return a preview for. The server may
             *   return a newer version if it does not have the requested version
             *   available.
             */
            explicit GetUrlPreviewJob(const QString& url, Omittable<qint64> ts = none);

            /*! Construct a URL without creating a full-fledged job object
             *
             * This function can be used when a URL for
             * GetUrlPreviewJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl, const QString& url, Omittable<qint64> ts = none);

            ~GetUrlPreviewJob() override;

            // Result properties

            /// The byte-size of the image. Omitted if there is no image attached.
            Omittable<qint64> matrixImageSize() const;
            /// An MXC URI to the image. Omitted if there is no image.
            const QString& ogImage() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };

    /// Get the configuration for the content repository.
    ///
    /// This endpoint allows clients to retrieve the configuration of the content
    /// repository, such as upload limitations.
    /// Clients SHOULD use this as a guide when using content repository endpoints.
    /// All values are intentionally left optional. Clients SHOULD follow
    /// the advice given in the field description when the field is not available.
    /// 
    /// **NOTE:** Both clients and server administrators should be aware that proxies
    /// between the client and the server may affect the apparent behaviour of content
    /// repository APIs, for example, proxies may enforce a lower upload size limit
    /// than is advertised by the server on this endpoint.
    class GetConfigJob : public BaseJob
    {
        public:
            explicit GetConfigJob();

            /*! Construct a URL without creating a full-fledged job object
             *
             * This function can be used when a URL for
             * GetConfigJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl);

            ~GetConfigJob() override;

            // Result properties

            /// The maximum size an upload can be in bytes.
            /// Clients SHOULD use this as a guide when uploading content.
            /// If not listed or null, the size limit should be treated as unknown.
            Omittable<double> uploadSize() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };
} // namespace QMatrixClient
