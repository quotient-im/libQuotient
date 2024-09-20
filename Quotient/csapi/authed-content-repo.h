// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#pragma once

#include <Quotient/jobs/basejob.h>

#include <QtCore/QIODevice>
#include <QtNetwork/QNetworkReply>

namespace Quotient {

//! \brief Download content from the content repository.
//!
//! \note
//! Clients SHOULD NOT generate or use URLs which supply the access token in
//! the query string. These URLs may be copied by users verbatim and provided
//! in a chat message to another user, disclosing the sender's access token.
//!
//! Clients MAY be redirected using the 307/308 responses below to download
//! the request object. This is typical when the homeserver uses a Content
//! Delivery Network (CDN).
class QUOTIENT_API GetContentAuthedJob : public BaseJob {
public:
    //! \param serverName
    //!   The server name from the `mxc://` URI (the authority component).
    //!
    //! \param mediaId
    //!   The media ID from the `mxc://` URI (the path component).
    //!
    //! \param timeoutMs
    //!   The maximum number of milliseconds that the client is willing to wait to
    //!   start receiving data, in the case that the content has not yet been
    //!   uploaded. The default value is 20000 (20 seconds). The content
    //!   repository SHOULD impose a maximum value for this parameter. The
    //!   content repository MAY respond before the timeout.
    explicit GetContentAuthedJob(const QString& serverName, const QString& mediaId,
                                 qint64 timeoutMs = 20000);

    //! \brief Construct a URL without creating a full-fledged job object
    //!
    //! This function can be used when a URL for GetContentAuthedJob
    //! is necessary but the job itself isn't.
    static QUrl makeRequestUrl(const HomeserverData& hsData, const QString& serverName,
                               const QString& mediaId, qint64 timeoutMs = 20000);

    // Result properties

    //! The content type of the file that was previously uploaded.
    //!
    //! The server MUST return a `Content-Type` which is either exactly the same
    //! as the original upload, or reasonably close. The bounds of "reasonable"
    //! are:
    //!
    //! * Adding a charset to `text/*` content types.
    //! * Detecting HTML and using `text/html` instead of `text/plain`.
    //! * Using `application/octet-stream` when the server determines the
    //!   content type is obviously wrong. For example, an encrypted file being
    //!   claimed as `image/png`.
    //! * Returning `application/octet-stream` when the media has an
    //!   unknown/unprovided `Content-Type`. For example, being uploaded before
    //!   the server tracked content types or when the remote server is
    //!   non-compliantly omitting the header entirely.
    //!
    //! Actions not in the spirit of the above are not considered "reasonable".
    QString contentType() const { return QString::fromUtf8(reply()->rawHeader("Content-Type")); }

    //! The
    //! [disposition](https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Content-Disposition)
    //! of the returned content. MUST be one of `inline` or `attachment`,
    //! and SHOULD contain a file name.
    //!
    //! If the `Content-Type` is allowed in the [restrictions for serving
    //! inline content](/client-server-api/#serving-inline-content),
    //! servers SHOULD use `inline`, otherwise they SHOULD use
    //! `attachment`.
    //!
    //! If the upload was made with a `filename`, this header MUST
    //! contain the same `filename`. Otherwise, `filename` is excluded
    //! from the header. If the media being downloaded is remote, the
    //! remote server's `filename` in the `Content-Disposition` header
    //! is used as the `filename` instead. When the header is not
    //! supplied, or does not supply a `filename`, the local download
    //! response does not include a `filename`.
    QString contentDisposition() const
    {
        return QString::fromUtf8(reply()->rawHeader("Content-Disposition"));
    }

    //! The content that was previously uploaded.
    QIODevice* data() { return reply(); }
};

//! \brief Download content from the content repository overriding the file name.
//!
//! This will download content from the content repository (same as
//! the previous endpoint) but replaces the target file name with the one
//! provided by the caller.
//!
//! \note
//! Clients SHOULD NOT generate or use URLs which supply the access token in
//! the query string. These URLs may be copied by users verbatim and provided
//! in a chat message to another user, disclosing the sender's access token.
//!
//! Clients MAY be redirected using the 307/308 responses below to download
//! the request object. This is typical when the homeserver uses a Content
//! Delivery Network (CDN).
class QUOTIENT_API GetContentOverrideNameAuthedJob : public BaseJob {
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
    //! \param timeoutMs
    //!   The maximum number of milliseconds that the client is willing to wait to
    //!   start receiving data, in the case that the content has not yet been
    //!   uploaded. The default value is 20000 (20 seconds). The content
    //!   repository SHOULD impose a maximum value for this parameter. The
    //!   content repository MAY respond before the timeout.
    explicit GetContentOverrideNameAuthedJob(const QString& serverName, const QString& mediaId,
                                             const QString& fileName, qint64 timeoutMs = 20000);

    //! \brief Construct a URL without creating a full-fledged job object
    //!
    //! This function can be used when a URL for GetContentOverrideNameAuthedJob
    //! is necessary but the job itself isn't.
    static QUrl makeRequestUrl(const HomeserverData& hsData, const QString& serverName,
                               const QString& mediaId, const QString& fileName,
                               qint64 timeoutMs = 20000);

    // Result properties

    //! The content type of the file that was previously uploaded.
    //!
    //! The server MUST return a `Content-Type` which is either exactly the same
    //! as the original upload, or reasonably close. The bounds of "reasonable"
    //! are:
    //!
    //! * Adding a charset to `text/*` content types.
    //! * Detecting HTML and using `text/html` instead of `text/plain`.
    //! * Using `application/octet-stream` when the server determines the
    //!   content type is obviously wrong. For example, an encrypted file being
    //!   claimed as `image/png`.
    //! * Returning `application/octet-stream` when the media has an
    //!   unknown/unprovided `Content-Type`. For example, being uploaded before
    //!   the server tracked content types or when the remote server is
    //!   non-compliantly omitting the header entirely.
    //!
    //! Actions not in the spirit of the above are not considered "reasonable".
    QString contentType() const { return QString::fromUtf8(reply()->rawHeader("Content-Type")); }

    //! The
    //! [disposition](https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Content-Disposition)
    //! of the returned content. MUST be one of `inline` or `attachment`,
    //! and MUST contain the file name requested in the path.
    //!
    //! If the `Content-Type` is allowed in the [restrictions for serving
    //! inline content](/client-server-api/#serving-inline-content),
    //! servers SHOULD use `inline`, otherwise they SHOULD use
    //! `attachment`.
    QString contentDisposition() const
    {
        return QString::fromUtf8(reply()->rawHeader("Content-Disposition"));
    }

    //! The content that was previously uploaded.
    QIODevice* data() { return reply(); }
};

//! \brief Download a thumbnail of content from the content repository
//!
//! Download a thumbnail of content from the content repository.
//! See the [Thumbnails](/client-server-api/#thumbnails) section for more information.
//!
//! \note
//! Clients SHOULD NOT generate or use URLs which supply the access token in
//! the query string. These URLs may be copied by users verbatim and provided
//! in a chat message to another user, disclosing the sender's access token.
//!
//! Clients MAY be redirected using the 307/308 responses below to download
//! the request object. This is typical when the homeserver uses a Content
//! Delivery Network (CDN).
class QUOTIENT_API GetContentThumbnailAuthedJob : public BaseJob {
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
    //! \param timeoutMs
    //!   The maximum number of milliseconds that the client is willing to wait to
    //!   start receiving data, in the case that the content has not yet been
    //!   uploaded. The default value is 20000 (20 seconds). The content
    //!   repository SHOULD impose a maximum value for this parameter. The
    //!   content repository MAY respond before the timeout.
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
    explicit GetContentThumbnailAuthedJob(const QString& serverName, const QString& mediaId,
                                          int width, int height, const QString& method = {},
                                          qint64 timeoutMs = 20000,
                                          std::optional<bool> animated = std::nullopt);

    //! \brief Construct a URL without creating a full-fledged job object
    //!
    //! This function can be used when a URL for GetContentThumbnailAuthedJob
    //! is necessary but the job itself isn't.
    static QUrl makeRequestUrl(const HomeserverData& hsData, const QString& serverName,
                               const QString& mediaId, int width, int height,
                               const QString& method = {}, qint64 timeoutMs = 20000,
                               std::optional<bool> animated = std::nullopt);

    // Result properties

    //! The
    //! [disposition](https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Content-Disposition)
    //! of the returned content. MUST be `inline`, and SHOULD contain a file name (e.g.
    //! `thumbnail.png`).
    //!
    //! Servers should note the [Content-Type restrictions for serving inline
    //! content](/client-server-api/#serving-inline-content), as these limitations imply which
    //! formats should be used for thumbnail generation.
    QString contentDisposition() const
    {
        return QString::fromUtf8(reply()->rawHeader("Content-Disposition"));
    }

    //! The content type of the thumbnail.
    QString contentType() const { return QString::fromUtf8(reply()->rawHeader("Content-Type")); }

    //! A thumbnail of the requested content.
    QIODevice* data() { return reply(); }
};

//! \brief Get information about a URL for a client
//!
//! Get information about a URL for the client. Typically this is called when a
//! client sees a URL in a message and wants to render a preview for the user.
//!
//! \note
//! Clients should consider avoiding this endpoint for URLs posted in encrypted
//! rooms. Encrypted rooms often contain more sensitive information the users
//! do not want to share with the homeserver, and this can mean that the URLs
//! being shared should also not be shared with the homeserver.
class QUOTIENT_API GetUrlPreviewAuthedJob : public BaseJob {
public:
    //! \param url
    //!   The URL to get a preview of.
    //!
    //! \param ts
    //!   The preferred point in time to return a preview for. The server may
    //!   return a newer version if it does not have the requested version
    //!   available.
    explicit GetUrlPreviewAuthedJob(const QUrl& url, std::optional<qint64> ts = std::nullopt);

    //! \brief Construct a URL without creating a full-fledged job object
    //!
    //! This function can be used when a URL for GetUrlPreviewAuthedJob
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

template <std::derived_from<GetUrlPreviewAuthedJob> JobT>
constexpr inline auto doCollectResponse<JobT> = [](JobT* j) -> GetUrlPreviewAuthedJob::Response {
    return { j->matrixImageSize(), j->ogImage() };
};

//! \brief Get the configuration for the content repository.
//!
//! This endpoint allows clients to retrieve the configuration of the content
//! repository, such as upload limitations.
//! Clients SHOULD use this as a guide when using content repository endpoints.
//! All values are intentionally left optional. Clients SHOULD follow
//! the advice given in the field description when the field is not available.
//!
//! \note
//! Both clients and server administrators should be aware that proxies
//! between the client and the server may affect the apparent behaviour of content
//! repository APIs, for example, proxies may enforce a lower upload size limit
//! than is advertised by the server on this endpoint.
class QUOTIENT_API GetConfigAuthedJob : public BaseJob {
public:
    explicit GetConfigAuthedJob();

    //! \brief Construct a URL without creating a full-fledged job object
    //!
    //! This function can be used when a URL for GetConfigAuthedJob
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

inline auto collectResponse(const GetConfigAuthedJob* job) { return job->uploadSize(); }

} // namespace Quotient
