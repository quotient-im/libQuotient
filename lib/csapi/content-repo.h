/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"

#include <QtCore/QIODevice>
#include <QtNetwork/QNetworkReply>

namespace Quotient {

/*! \brief Upload some content to the content repository.
 *
 */
class UploadContentJob : public BaseJob {
public:
    /*! \brief Upload some content to the content repository.
     *
     * \param content
     *   The content to be uploaded.
     *
     * \param filename
     *   The name of the file being uploaded
     *
     * \param contentType
     *   The content type of the file being uploaded
     */
    explicit UploadContentJob(QIODevice* content, const QString& filename = {},
                              const QString& contentType = {});

    // Result properties

    /// The [MXC URI](/client-server-api/#matrix-content-mxc-uris) to the
    /// uploaded content.
    QUrl contentUri() const { return loadFromJson<QUrl>("content_uri"_ls); }
};

/*! \brief Download content from the content repository.
 *
 */
class GetContentJob : public BaseJob {
public:
    /*! \brief Download content from the content repository.
     *
     * \param serverName
     *   The server name from the `mxc://` URI (the authoritory component)
     *
     * \param mediaId
     *   The media ID from the `mxc://` URI (the path component)
     *
     * \param allowRemote
     *   Indicates to the server that it should not attempt to fetch the media
     * if it is deemed remote. This is to prevent routing loops where the server
     * contacts itself. Defaults to true if not provided.
     */
    explicit GetContentJob(const QString& serverName, const QString& mediaId,
                           bool allowRemote = true);

    /*! \brief Construct a URL without creating a full-fledged job object
     *
     * This function can be used when a URL for GetContentJob
     * is necessary but the job itself isn't.
     */
    static QUrl makeRequestUrl(QUrl baseUrl, const QString& serverName,
                               const QString& mediaId, bool allowRemote = true);

    // Result properties

    /// The content type of the file that was previously uploaded.
    QString contentType() const { return reply()->rawHeader("Content-Type"); }

    /// The name of the file that was previously uploaded, if set.
    QString contentDisposition() const
    {
        return reply()->rawHeader("Content-Disposition");
    }

    /// The content that was previously uploaded.
    QIODevice* data() { return reply(); }
};

/*! \brief Download content from the content repository overriding the file name
 *
 * This will download content from the content repository (same as
 * the previous endpoint) but replace the target file name with the one
 * provided by the caller.
 */
class GetContentOverrideNameJob : public BaseJob {
public:
    /*! \brief Download content from the content repository overriding the file
     * name
     *
     * \param serverName
     *   The server name from the `mxc://` URI (the authoritory component)
     *
     * \param mediaId
     *   The media ID from the `mxc://` URI (the path component)
     *
     * \param fileName
     *   A filename to give in the `Content-Disposition` header.
     *
     * \param allowRemote
     *   Indicates to the server that it should not attempt to fetch the media
     * if it is deemed remote. This is to prevent routing loops where the server
     * contacts itself. Defaults to true if not provided.
     */
    explicit GetContentOverrideNameJob(const QString& serverName,
                                       const QString& mediaId,
                                       const QString& fileName,
                                       bool allowRemote = true);

    /*! \brief Construct a URL without creating a full-fledged job object
     *
     * This function can be used when a URL for GetContentOverrideNameJob
     * is necessary but the job itself isn't.
     */
    static QUrl makeRequestUrl(QUrl baseUrl, const QString& serverName,
                               const QString& mediaId, const QString& fileName,
                               bool allowRemote = true);

    // Result properties

    /// The content type of the file that was previously uploaded.
    QString contentType() const { return reply()->rawHeader("Content-Type"); }

    /// The `fileName` requested or the name of the file that was previously
    /// uploaded, if set.
    QString contentDisposition() const
    {
        return reply()->rawHeader("Content-Disposition");
    }

    /// The content that was previously uploaded.
    QIODevice* data() { return reply(); }
};

/*! \brief Download a thumbnail of content from the content repository
 *
 * Download a thumbnail of content from the content repository.
 * See the [Thumbnails](/client-server-api/#thumbnails) section for more
 * information.
 */
class GetContentThumbnailJob : public BaseJob {
public:
    /*! \brief Download a thumbnail of content from the content repository
     *
     * \param serverName
     *   The server name from the `mxc://` URI (the authoritory component)
     *
     * \param mediaId
     *   The media ID from the `mxc://` URI (the path component)
     *
     * \param width
     *   The *desired* width of the thumbnail. The actual thumbnail may be
     *   larger than the size specified.
     *
     * \param height
     *   The *desired* height of the thumbnail. The actual thumbnail may be
     *   larger than the size specified.
     *
     * \param method
     *   The desired resizing method. See the
     * [Thumbnails](/client-server-api/#thumbnails) section for more information.
     *
     * \param allowRemote
     *   Indicates to the server that it should not attempt to fetch
     *   the media if it is deemed remote. This is to prevent routing loops
     *   where the server contacts itself. Defaults to true if not provided.
     */
    explicit GetContentThumbnailJob(const QString& serverName,
                                    const QString& mediaId, int width,
                                    int height, const QString& method = {},
                                    bool allowRemote = true);

    /*! \brief Construct a URL without creating a full-fledged job object
     *
     * This function can be used when a URL for GetContentThumbnailJob
     * is necessary but the job itself isn't.
     */
    static QUrl makeRequestUrl(QUrl baseUrl, const QString& serverName,
                               const QString& mediaId, int width, int height,
                               const QString& method = {},
                               bool allowRemote = true);

    // Result properties

    /// The content type of the thumbnail.
    QString contentType() const { return reply()->rawHeader("Content-Type"); }

    /// A thumbnail of the requested content.
    QIODevice* data() { return reply(); }
};

/*! \brief Get information about a URL for a client
 *
 * Get information about a URL for the client. Typically this is called when a
 * client sees a URL in a message and wants to render a preview for the user.
 *
 * **Note:**
 * Clients should consider avoiding this endpoint for URLs posted in encrypted
 * rooms. Encrypted rooms often contain more sensitive information the users
 * do not want to share with the homeserver, and this can mean that the URLs
 * being shared should also not be shared with the homeserver.
 */
class GetUrlPreviewJob : public BaseJob {
public:
    /*! \brief Get information about a URL for a client
     *
     * \param url
     *   The URL to get a preview of.
     *
     * \param ts
     *   The preferred point in time to return a preview for. The server may
     *   return a newer version if it does not have the requested version
     *   available.
     */
    explicit GetUrlPreviewJob(const QUrl& url, Omittable<qint64> ts = none);

    /*! \brief Construct a URL without creating a full-fledged job object
     *
     * This function can be used when a URL for GetUrlPreviewJob
     * is necessary but the job itself isn't.
     */
    static QUrl makeRequestUrl(QUrl baseUrl, const QUrl& url,
                               Omittable<qint64> ts = none);

    // Result properties

    /// The byte-size of the image. Omitted if there is no image attached.
    Omittable<qint64> matrixImageSize() const
    {
        return loadFromJson<Omittable<qint64>>("matrix:image:size"_ls);
    }

    /// An [MXC URI](/client-server-api/#matrix-content-mxc-uris) to the image.
    /// Omitted if there is no image.
    QUrl ogImage() const { return loadFromJson<QUrl>("og:image"_ls); }
};

/*! \brief Get the configuration for the content repository.
 *
 * This endpoint allows clients to retrieve the configuration of the content
 * repository, such as upload limitations.
 * Clients SHOULD use this as a guide when using content repository endpoints.
 * All values are intentionally left optional. Clients SHOULD follow
 * the advice given in the field description when the field is not available.
 *
 * **NOTE:** Both clients and server administrators should be aware that proxies
 * between the client and the server may affect the apparent behaviour of
 * content repository APIs, for example, proxies may enforce a lower upload size
 * limit than is advertised by the server on this endpoint.
 */
class GetConfigJob : public BaseJob {
public:
    /// Get the configuration for the content repository.
    explicit GetConfigJob();

    /*! \brief Construct a URL without creating a full-fledged job object
     *
     * This function can be used when a URL for GetConfigJob
     * is necessary but the job itself isn't.
     */
    static QUrl makeRequestUrl(QUrl baseUrl);

    // Result properties

    /// The maximum size an upload can be in bytes.
    /// Clients SHOULD use this as a guide when uploading content.
    /// If not listed or null, the size limit should be treated as unknown.
    Omittable<qint64> uploadSize() const
    {
        return loadFromJson<Omittable<qint64>>("m.upload.size"_ls);
    }
};

} // namespace Quotient
