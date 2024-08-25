// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#pragma once

#include <Quotient/jobs/basejob.h>

#include <QtCore/QIODevice>
#include <QtNetwork/QNetworkReply>

namespace Quotient {

//! \brief Upload some content to the content repository.
class QUOTIENT_API UploadContentJob : public BaseJob {
public:
    //!
    //! \param filename
    //!   The name of the file being uploaded
    //!
    //! \param contentType
    //!   The content type of the file being uploaded
    explicit UploadContentJob(QIODevice* content, const QString& filename = {},
                              const QString& contentType = {});

    // Result properties

    //! The [`mxc://` URI](/client-server-api/#matrix-content-mxc-uris) to the uploaded content.
    QUrl contentUri() const { return loadFromJson<QUrl>("content_uri"_L1); }
};

inline auto collectResponse(const UploadContentJob* job) { return job->contentUri(); }

//! \brief Upload content to an `mxc://` URI that was created earlier.
//!
//! This endpoint permits uploading content to an `mxc://` URI that was created
//! earlier via [POST /_matrix/media/v1/create](/client-server-api/#post_matrixmediav1create).
class QUOTIENT_API UploadContentToMXCJob : public BaseJob {
public:
    //! \param serverName
    //!   The server name from the `mxc://` URI (the authority component).
    //!
    //! \param mediaId
    //!   The media ID from the `mxc://` URI (the path component).
    //!
    //!
    //! \param filename
    //!   The name of the file being uploaded
    //!
    //! \param contentType
    //!   The content type of the file being uploaded
    explicit UploadContentToMXCJob(const QString& serverName, const QString& mediaId,
                                   QIODevice* content, const QString& filename = {},
                                   const QString& contentType = {});
};

//! \brief Create a new `mxc://` URI without uploading the content.
//!
//! Creates a new `mxc://` URI, independently of the content being uploaded. The content must be
//! provided later via [`PUT
//! /_matrix/media/v3/upload/{serverName}/{mediaId}`](/client-server-api/#put_matrixmediav3uploadservernamemediaid).
//!
//! The server may optionally enforce a maximum age for unused IDs,
//! and delete media IDs when the client doesn't start the upload in time,
//! or when the upload was interrupted and not resumed in time. The server
//! should include the maximum POSIX millisecond timestamp to complete the
//! upload in the `unused_expires_at` field in the response JSON. The
//! recommended default expiration is 24 hours which should be enough time
//! to accommodate users on poor connection who find a better connection to
//! complete the upload.
//!
//! As well as limiting the rate of requests to create `mxc://` URIs, the server
//! should limit the number of concurrent *pending media uploads* a given
//! user can have. A pending media upload is a created `mxc://` URI where (a)
//! the media has not yet been uploaded, and (b) has not yet expired (the
//! `unused_expires_at` timestamp has not yet passed). In both cases, the
//! server should respond with an HTTP 429 error with an errcode of
//! `M_LIMIT_EXCEEDED`.
class QUOTIENT_API CreateContentJob : public BaseJob {
public:
    explicit CreateContentJob();

    //! \brief Construct a URL without creating a full-fledged job object
    //!
    //! This function can be used when a URL for CreateContentJob
    //! is necessary but the job itself isn't.
    static QUrl makeRequestUrl(const HomeserverData& hsData);

    // Result properties

    //! The [`mxc://` URI](/client-server-api/#matrix-content-mxc-uris) at
    //! which the content will be available, once it is uploaded.
    QUrl contentUri() const { return loadFromJson<QUrl>("content_uri"_L1); }

    //! The timestamp (in milliseconds since the unix epoch) when the
    //! generated media id will expire, if media is not uploaded.
    std::optional<qint64> unusedExpiresAt() const
    {
        return loadFromJson<std::optional<qint64>>("unused_expires_at"_L1);
    }

    struct Response {
        //! The [`mxc://` URI](/client-server-api/#matrix-content-mxc-uris) at
        //! which the content will be available, once it is uploaded.
        QUrl contentUri{};

        //! The timestamp (in milliseconds since the unix epoch) when the
        //! generated media id will expire, if media is not uploaded.
        std::optional<qint64> unusedExpiresAt{};
    };
};

template <std::derived_from<CreateContentJob> JobT>
constexpr inline auto doCollectResponse<JobT> =
    [](JobT* j) -> CreateContentJob::Response { return { j->contentUri(), j->unusedExpiresAt() }; };

//! \brief Download content from the content repository.
//!
//! \note
//! Replaced by [`GET
//! /_matrix/client/v1/media/download/{serverName}/{mediaId}`](/client-server-api/#get_matrixclientv1mediadownloadservernamemediaid)
//! (requires authentication).
//!
//! \warning
//! <strong>[Changed in v1.11]</strong> This endpoint MAY return `404 M_NOT_FOUND`
//! for media which exists, but is after the server froze unauthenticated
//! media access. See [Client Behaviour](/client-server-api/#content-repo-client-behaviour) for more
//! information.
class [[deprecated("Check the documentation for details")]] QUOTIENT_API GetContentJob
    : public BaseJob {
public:
    //! \param serverName
    //!   The server name from the `mxc://` URI (the authority component).
    //!
    //! \param mediaId
    //!   The media ID from the `mxc://` URI (the path component).
    //!
    //! \param allowRemote
    //!   Indicates to the server that it should not attempt to fetch the media if
    //!   it is deemed remote. This is to prevent routing loops where the server
    //!   contacts itself.
    //!
    //!   Defaults to `true` if not provided.
    //!
    //! \param timeoutMs
    //!   The maximum number of milliseconds that the client is willing to wait to
    //!   start receiving data, in the case that the content has not yet been
    //!   uploaded. The default value is 20000 (20 seconds). The content
    //!   repository SHOULD impose a maximum value for this parameter. The
    //!   content repository MAY respond before the timeout.
    //!
    //! \param allowRedirect
    //!   Indicates to the server that it may return a 307 or 308 redirect
    //!   response that points at the relevant media content. When not explicitly
    //!   set to `true` the server must return the media content itself.
    explicit GetContentJob(const QString& serverName, const QString& mediaId,
                           bool allowRemote = true, qint64 timeoutMs = 20000,
                           bool allowRedirect = false);

    //! \brief Construct a URL without creating a full-fledged job object
    //!
    //! This function can be used when a URL for GetContentJob
    //! is necessary but the job itself isn't.
    static QUrl makeRequestUrl(const HomeserverData& hsData, const QString& serverName,
                               const QString& mediaId, bool allowRemote = true,
                               qint64 timeoutMs = 20000, bool allowRedirect = false);

    // Result properties

    //! The content type of the file that was previously uploaded.
    QString contentType() const { return QString::fromUtf8(reply()->rawHeader("Content-Type")); }

    //! The name of the file that was previously uploaded, if set.
    QString contentDisposition() const
    {
        return QString::fromUtf8(reply()->rawHeader("Content-Disposition"));
    }

    //! The content that was previously uploaded.
    QIODevice* data() { return reply(); }
};

//! \brief Download content from the content repository overriding the file name
//!
//! \note
//! Replaced by [`GET
//! /_matrix/client/v1/media/download/{serverName}/{mediaId}/{fileName}`](/client-server-api/#get_matrixclientv1mediadownloadservernamemediaidfilename)
//! (requires authentication).
//!
//! This will download content from the content repository (same as
//! the previous endpoint) but replace the target file name with the one
//! provided by the caller.
//!
//! \warning
//! <strong>[Changed in v1.11]</strong> This endpoint MAY return `404 M_NOT_FOUND`
//! for media which exists, but is after the server froze unauthenticated
//! media access. See [Client Behaviour](/client-server-api/#content-repo-client-behaviour) for more
//! information.
class [[deprecated("Check the documentation for details")]] QUOTIENT_API GetContentOverrideNameJob
    : public BaseJob {
public:
    //! \param serverName
    //!   The server name from the `mxc://` URI (the authority component).
    //!
    //! \param mediaId
    //!   The media ID from the `mxc://` URI (the path component).
    //!
    //! \param fileName
    //!   A filename to give in the `Content-Disposition` header.
    //!
    //! \param allowRemote
    //!   Indicates to the server that it should not attempt to fetch the media if
    //!   it is deemed remote. This is to prevent routing loops where the server
    //!   contacts itself.
    //!
    //!   Defaults to `true` if not provided.
    //!
    //! \param timeoutMs
    //!   The maximum number of milliseconds that the client is willing to wait to
    //!   start receiving data, in the case that the content has not yet been
    //!   uploaded. The default value is 20000 (20 seconds). The content
    //!   repository SHOULD impose a maximum value for this parameter. The
    //!   content repository MAY respond before the timeout.
    //!
    //! \param allowRedirect
    //!   Indicates to the server that it may return a 307 or 308 redirect
    //!   response that points at the relevant media content. When not explicitly
    //!   set to `true` the server must return the media content itself.
    explicit GetContentOverrideNameJob(const QString& serverName, const QString& mediaId,
                                       const QString& fileName, bool allowRemote = true,
                                       qint64 timeoutMs = 20000, bool allowRedirect = false);

    //! \brief Construct a URL without creating a full-fledged job object
    //!
    //! This function can be used when a URL for GetContentOverrideNameJob
    //! is necessary but the job itself isn't.
    static QUrl makeRequestUrl(const HomeserverData& hsData, const QString& serverName,
                               const QString& mediaId, const QString& fileName,
                               bool allowRemote = true, qint64 timeoutMs = 20000,
                               bool allowRedirect = false);

    // Result properties

    //! The content type of the file that was previously uploaded.
    QString contentType() const { return QString::fromUtf8(reply()->rawHeader("Content-Type")); }

    //! The `fileName` requested or the name of the file that was previously
    //! uploaded, if set.
    QString contentDisposition() const
    {
        return QString::fromUtf8(reply()->rawHeader("Content-Disposition"));
    }

    //! The content that was previously uploaded.
    QIODevice* data() { return reply(); }
};

//! \brief Download a thumbnail of content from the content repository
//!
//! \note
//! Replaced by [`GET
//! /_matrix/client/v1/media/thumbnail/{serverName}/{mediaId}`](/client-server-api/#get_matrixclientv1mediathumbnailservernamemediaid)
//! (requires authentication).
//!
//! Download a thumbnail of content from the content repository.
//! See the [Thumbnails](/client-server-api/#thumbnails) section for more information.
//!
//! \warning
//! <strong>[Changed in v1.11]</strong> This endpoint MAY return `404 M_NOT_FOUND`
//! for media which exists, but is after the server froze unauthenticated
//! media access. See [Client Behaviour](/client-server-api/#content-repo-client-behaviour) for more
//! information.
class [[deprecated("Check the documentation for details")]] QUOTIENT_API GetContentThumbnailJob
    : public BaseJob {
public:
    //! \param serverName
    //!   The server name from the `mxc://` URI (the authority component).
    //!
    //! \param mediaId
    //!   The media ID from the `mxc://` URI (the path component).
    //!
    //! \param width
    //!   The *desired* width of the thumbnail. The actual thumbnail may be
    //!   larger than the size specified.
    //!
    //! \param height
    //!   The *desired* height of the thumbnail. The actual thumbnail may be
    //!   larger than the size specified.
    //!
    //! \param method
    //!   The desired resizing method. See the [Thumbnails](/client-server-api/#thumbnails)
    //!   section for more information.
    //!
    //! \param allowRemote
    //!   Indicates to the server that it should not attempt to fetch the media if
    //!   it is deemed remote. This is to prevent routing loops where the server
    //!   contacts itself.
    //!
    //!   Defaults to `true` if not provided.
    //!
    //! \param timeoutMs
    //!   The maximum number of milliseconds that the client is willing to wait to
    //!   start receiving data, in the case that the content has not yet been
    //!   uploaded. The default value is 20000 (20 seconds). The content
    //!   repository SHOULD impose a maximum value for this parameter. The
    //!   content repository MAY respond before the timeout.
    //!
    //! \param allowRedirect
    //!   Indicates to the server that it may return a 307 or 308 redirect
    //!   response that points at the relevant media content. When not explicitly
    //!   set to `true` the server must return the media content itself.
    //!
    //! \param animated
    //!   Indicates preference for an animated thumbnail from the server, if possible. Animated
    //!   thumbnails typically use the content types `image/gif`, `image/png` (with APNG format),
    //!   `image/apng`, and `image/webp` instead of the common static `image/png` or `image/jpeg`
    //!   content types.
    //!
    //!   When `true`, the server SHOULD return an animated thumbnail if possible and supported.
    //!   When `false`, the server MUST NOT return an animated thumbnail. For example, returning a
    //!   static `image/png` or `image/jpeg` thumbnail. When not provided, the server SHOULD NOT
    //!   return an animated thumbnail.
    //!
    //!   Servers SHOULD prefer to return `image/webp` thumbnails when supporting animation.
    //!
    //!   When `true` and the media cannot be animated, such as in the case of a JPEG or PDF, the
    //!   server SHOULD behave as though `animated` is `false`.
    explicit GetContentThumbnailJob(const QString& serverName, const QString& mediaId, int width,
                                    int height, const QString& method = {}, bool allowRemote = true,
                                    qint64 timeoutMs = 20000, bool allowRedirect = false,
                                    std::optional<bool> animated = std::nullopt);

    //! \brief Construct a URL without creating a full-fledged job object
    //!
    //! This function can be used when a URL for GetContentThumbnailJob
    //! is necessary but the job itself isn't.
    static QUrl makeRequestUrl(const HomeserverData& hsData, const QString& serverName,
                               const QString& mediaId, int width, int height,
                               const QString& method = {}, bool allowRemote = true,
                               qint64 timeoutMs = 20000, bool allowRedirect = false,
                               std::optional<bool> animated = std::nullopt);

    // Result properties

    //! The content type of the thumbnail.
    QString contentType() const { return QString::fromUtf8(reply()->rawHeader("Content-Type")); }

    //! A thumbnail of the requested content.
    QIODevice* data() { return reply(); }
};

//! \brief Get information about a URL for a client
//!
//! \note
//! Replaced by [`GET
//! /_matrix/client/v1/media/preview_url`](/client-server-api/#get_matrixclientv1mediapreview_url).
//!
//! Get information about a URL for the client. Typically this is called when a
//! client sees a URL in a message and wants to render a preview for the user.
//!
//! **Note:**
//! Clients should consider avoiding this endpoint for URLs posted in encrypted
//! rooms. Encrypted rooms often contain more sensitive information the users
//! do not want to share with the homeserver, and this can mean that the URLs
//! being shared should also not be shared with the homeserver.
class [[deprecated("Check the documentation for details")]] QUOTIENT_API GetUrlPreviewJob
    : public BaseJob {
public:
    //! \param url
    //!   The URL to get a preview of.
    //!
    //! \param ts
    //!   The preferred point in time to return a preview for. The server may
    //!   return a newer version if it does not have the requested version
    //!   available.
    explicit GetUrlPreviewJob(const QUrl& url, std::optional<qint64> ts = std::nullopt);

    //! \brief Construct a URL without creating a full-fledged job object
    //!
    //! This function can be used when a URL for GetUrlPreviewJob
    //! is necessary but the job itself isn't.
    static QUrl makeRequestUrl(const HomeserverData& hsData, const QUrl& url,
                               std::optional<qint64> ts = std::nullopt);

    // Result properties

    //! The byte-size of the image. Omitted if there is no image attached.
    std::optional<qint64> matrixImageSize() const
    {
        return loadFromJson<std::optional<qint64>>("matrix:image:size"_L1);
    }

    //! An [`mxc://` URI](/client-server-api/#matrix-content-mxc-uris) to the image. Omitted if
    //! there is no image.
    QUrl ogImage() const { return loadFromJson<QUrl>("og:image"_L1); }

    struct Response {
        //! The byte-size of the image. Omitted if there is no image attached.
        std::optional<qint64> matrixImageSize{};

        //! An [`mxc://` URI](/client-server-api/#matrix-content-mxc-uris) to the image. Omitted if
        //! there is no image.
        QUrl ogImage{};
    };
};

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
template <std::derived_from<GetUrlPreviewJob> JobT>
constexpr inline auto doCollectResponse<JobT> =
    [](JobT* j) -> GetUrlPreviewJob::Response { return { j->matrixImageSize(), j->ogImage() }; };
QT_WARNING_POP

//! \brief Get the configuration for the content repository.
//!
//! \note
//! Replaced by [`GET
//! /_matrix/client/v1/media/config`](/client-server-api/#get_matrixclientv1mediaconfig).
//!
//! This endpoint allows clients to retrieve the configuration of the content
//! repository, such as upload limitations.
//! Clients SHOULD use this as a guide when using content repository endpoints.
//! All values are intentionally left optional. Clients SHOULD follow
//! the advice given in the field description when the field is not available.
//!
//! **NOTE:** Both clients and server administrators should be aware that proxies
//! between the client and the server may affect the apparent behaviour of content
//! repository APIs, for example, proxies may enforce a lower upload size limit
//! than is advertised by the server on this endpoint.
class [[deprecated("Check the documentation for details")]] QUOTIENT_API GetConfigJob
    : public BaseJob {
public:
    explicit GetConfigJob();

    //! \brief Construct a URL without creating a full-fledged job object
    //!
    //! This function can be used when a URL for GetConfigJob
    //! is necessary but the job itself isn't.
    static QUrl makeRequestUrl(const HomeserverData& hsData);

    // Result properties

    //! The maximum size an upload can be in bytes.
    //! Clients SHOULD use this as a guide when uploading content.
    //! If not listed or null, the size limit should be treated as unknown.
    std::optional<qint64> uploadSize() const
    {
        return loadFromJson<std::optional<qint64>>("m.upload.size"_L1);
    }
};

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
inline auto collectResponse(const GetConfigJob* job) { return job->uploadSize(); }
QT_WARNING_POP

} // namespace Quotient
