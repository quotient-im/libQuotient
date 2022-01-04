// SPDX-FileCopyrightText: 2017 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

// This file contains generic event content definitions, applicable to room
// message events as well as other events (e.g., avatars).

#include "encryptedfile.h"
#include "quotient_export.h"

#include <QtCore/QJsonObject>
#include <QtCore/QMimeType>
#include <QtCore/QSize>
#include <QtCore/QUrl>
#include <QtCore/QMetaType>

class QFileInfo;

namespace Quotient {
namespace EventContent {
    //! \brief Base for all content types that can be stored in RoomMessageEvent
    //!
    //! Each content type class should have a constructor taking
    //! a QJsonObject and override fillJson() with an implementation
    //! that will fill the target QJsonObject with stored values. It is
    //! assumed but not required that a content object can also be created
    //! from plain data.
    class QUOTIENT_API Base {
    public:
        explicit Base(QJsonObject o = {}) : originalJson(std::move(o)) {}
        virtual ~Base() = default;

        QJsonObject toJson() const;

    public:
        QJsonObject originalJson;

    protected:
        Base(const Base&) = default;
        Base(Base&&) = default;

        virtual void fillJson(QJsonObject* o) const = 0;
    };

    // The below structures fairly follow CS spec 11.2.1.6. The overall
    // set of attributes for each content types is a superset of the spec
    // but specific aggregation structure is altered. See doc comments to
    // each type for the list of available attributes.

    // A quick classes inheritance structure follows (the definitions are
    // spread across eventcontent.h and roommessageevent.h):
    // FileInfo
    //   FileContent : UrlWithThumbnailContent<FileInfo>
    //   AudioContent : PlayableContent<UrlBasedContent<FileInfo>>
    //   ImageInfo : FileInfo + imageSize attribute
    //     ImageContent : UrlWithThumbnailContent<ImageInfo>
    //     VideoContent : PlayableContent<UrlWithThumbnailContent<ImageInfo>>

    //! \brief Mix-in class representing `info` subobject in content JSON
    //!
    //! This is one of base classes for content types that deal with files or
    //! URLs. It stores the file metadata attributes, such as size, MIME type
    //! etc. found in the `content/info` subobject of event JSON payloads.
    //! Actual content classes derive from this class _and_ TypedBase that
    //! provides a polymorphic interface to access data in the mix-in. FileInfo
    //! (as well as ImageInfo, that adds image size to the metadata) is NOT
    //! polymorphic and is used in a non-polymorphic way to store thumbnail
    //! metadata (in a separate instance), next to the metadata on the file
    //! itself.
    //!
    //! If you need to make a new _content_ (not info) class based on files/URLs
    //! take UrlBasedContent as the example, i.e.:
    //! 1. Double-inherit from this class (or ImageInfo) and TypedBase.
    //! 2. Provide a constructor from QJsonObject that will pass the `info`
    //!    subobject (not the whole content JSON) down to FileInfo/ImageInfo.
    //! 3. Override fillJson() to customise the JSON export logic. Make sure
    //!    to call toInfoJson() from it to produce the payload for the `info`
    //!    subobject in the JSON payload.
    //!
    //! \sa ImageInfo, FileContent, ImageContent, AudioContent, VideoContent,
    //!     UrlBasedContent
    class QUOTIENT_API FileInfo {
    public:
        FileInfo() = default;
        //! \brief Construct from a QFileInfo object
        //!
        //! \param fi a QFileInfo object referring to an existing file
        explicit FileInfo(const QFileInfo& fi);
        explicit FileInfo(QUrl mxcUrl, qint64 payloadSize = -1,
                          const QMimeType& mimeType = {},
                          Omittable<EncryptedFile> encryptedFile = none,
                          QString originalFilename = {});
        //! \brief Construct from a JSON `info` payload
        //!
        //! Make sure to pass the `info` subobject of content JSON, not the
        //! whole JSON content.
        FileInfo(QUrl mxcUrl, const QJsonObject& infoJson,
                 Omittable<EncryptedFile> encryptedFile,
                 QString originalFilename = {});

        bool isValid() const;

        //! \brief Extract media id from the URL
        //!
        //! This can be used, e.g., to construct a QML-facing image://
        //! URI as follows:
        //! \code "image://provider/" + info.mediaId() \endcode
        QString mediaId() const { return url.authority() + url.path(); }

    public:
        QJsonObject originalInfoJson;
        QMimeType mimeType;
        QUrl url;
        qint64 payloadSize = 0;
        QString originalName;
        Omittable<EncryptedFile> file = none;
    };

    template <typename InfoT>
    QJsonObject toInfoJson(const InfoT& info)
    {
        QJsonObject infoJson;
        info.fillInfoJson(&infoJson);
        return infoJson;
    }

    //! \brief A content info class for image/video content types and thumbnails
    class QUOTIENT_API ImageInfo : public FileInfo {
    public:
        ImageInfo() = default;
        explicit ImageInfo(const QFileInfo& fi, QSize imageSize = {});
        explicit ImageInfo(const QUrl& mxcUrl, qint64 fileSize = -1,
                           const QMimeType& type = {}, QSize imageSize = {},
                           Omittable<EncryptedFile> encryptedFile = none,
                           const QString& originalFilename = {});
        ImageInfo(const QUrl& mxcUrl, const QJsonObject& infoJson,
                  Omittable<EncryptedFile> encryptedFile,
                  const QString& originalFilename = {});

        void fillInfoJson(QJsonObject* infoJson) const;

    public:
        QSize imageSize;
    };

    //! \brief An auxiliary class for an info type that carries a thumbnail
    //!
    //! This class saves/loads a thumbnail to/from `info` subobject of
    //! the JSON representation of event content; namely, `info/thumbnail_url`
    //! and `info/thumbnail_info` fields are used.
    class QUOTIENT_API Thumbnail : public ImageInfo {
    public:
        using ImageInfo::ImageInfo;
        Thumbnail(const QJsonObject& infoJson,
                  Omittable<EncryptedFile> encryptedFile = none);

        //! \brief Add thumbnail information to the passed `info` JSON object
        void fillInfoJson(QJsonObject* infoJson) const;
    };

    class QUOTIENT_API TypedBase : public Base {
    public:
        virtual QMimeType type() const = 0;
        virtual const FileInfo* fileInfo() const { return nullptr; }
        virtual FileInfo* fileInfo() { return nullptr; }
        virtual const Thumbnail* thumbnailInfo() const { return nullptr; }

    protected:
        explicit TypedBase(QJsonObject o = {}) : Base(std::move(o)) {}
        using Base::Base;
    };

    //! \brief A template class for content types with a URL and additional info
    //!
    //! Types that derive from this class template take `url` and,
    //! optionally, `filename` values from the top-level JSON object and
    //! the rest of information from the `info` subobject, as defined by
    //! the parameter type.
    //! \tparam InfoT base info class - FileInfo or ImageInfo
    template <class InfoT>
    class UrlBasedContent : public TypedBase, public InfoT {
    public:
        using InfoT::InfoT;
        explicit UrlBasedContent(const QJsonObject& json)
            : TypedBase(json)
            , InfoT(QUrl(json["url"].toString()), json["info"].toObject(),
                    fromJson<Omittable<EncryptedFile>>(json["file"]), json["filename"].toString())
        {
            // A small hack to facilitate links creation in QML.
            originalJson.insert("mediaId", InfoT::mediaId());
        }

        QMimeType type() const override { return InfoT::mimeType; }
        const FileInfo* fileInfo() const override { return this; }
        FileInfo* fileInfo() override { return this; }

    protected:
        void fillJson(QJsonObject* json) const override
        {
            Q_ASSERT(json);
            if (!InfoT::file.has_value()) {
                json->insert("url", InfoT::url.toString());
            } else {
                json->insert("file", Quotient::toJson(*InfoT::file));
            }
            if (!InfoT::originalName.isEmpty())
                json->insert("filename", InfoT::originalName);
            json->insert("info", toInfoJson<InfoT>(*this));
        }
    };

    template <typename InfoT>
    class QUOTIENT_API UrlWithThumbnailContent : public UrlBasedContent<InfoT> {
    public:
        // NB: when using inherited constructors, thumbnail has to be
        // initialised separately
        using UrlBasedContent<InfoT>::UrlBasedContent;
        explicit UrlWithThumbnailContent(const QJsonObject& json)
            : UrlBasedContent<InfoT>(json), thumbnail(InfoT::originalInfoJson)
        {
            // Another small hack, to simplify making a thumbnail link
            UrlBasedContent<InfoT>::originalJson.insert("thumbnailMediaId",
                                                        thumbnail.mediaId());
        }

        const Thumbnail* thumbnailInfo() const override { return &thumbnail; }

    public:
        Thumbnail thumbnail;

    protected:
        void fillJson(QJsonObject* json) const override
        {
            UrlBasedContent<InfoT>::fillJson(json);
            auto infoJson = json->take("info").toObject();
            thumbnail.fillInfoJson(&infoJson);
            json->insert("info", infoJson);
        }
    };

    //! \brief Content class for m.image
    //!
    //! Available fields:
    //! - corresponding to the top-level JSON:
    //!   - url
    //!   - filename (extension to the spec)
    //! - corresponding to the `info` subobject:
    //!   - payloadSize (`size` in JSON)
    //!   - mimeType (`mimetype` in JSON)
    //!   - imageSize (QSize for a combination of `h` and `w` in JSON)
    //!   - thumbnail.url (`thumbnail_url` in JSON)
    //! - corresponding to the `info/thumbnail_info` subobject: contents of
    //!   thumbnail field, in the same vein as for the main image:
    //!   - payloadSize
    //!   - mimeType
    //!   - imageSize
    using ImageContent = UrlBasedContent<ImageInfo>;

    //! \brief Content class for m.file
    //!
    //! Available fields:
    //! - corresponding to the top-level JSON:
    //!   - url
    //!   - filename
    //! - corresponding to the `info` subobject:
    //!   - payloadSize (`size` in JSON)
    //!   - mimeType (`mimetype` in JSON)
    //!   - thumbnail.url (`thumbnail_url` in JSON)
    //! - corresponding to the `info/thumbnail_info` subobject:
    //!   - thumbnail.payloadSize
    //!   - thumbnail.mimeType
    //!   - thumbnail.imageSize (QSize for `h` and `w` in JSON)
    using FileContent = UrlBasedContent<FileInfo>;
} // namespace EventContent
} // namespace Quotient
Q_DECLARE_METATYPE(const Quotient::EventContent::TypedBase*)
