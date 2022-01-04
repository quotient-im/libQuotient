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
    /**
     * A base class for all content types that can be stored
     * in a RoomMessageEvent
     *
     * Each content type class should have a constructor taking
     * a QJsonObject and override fillJson() with an implementation
     * that will fill the target QJsonObject with stored values. It is
     * assumed but not required that a content object can also be created
     * from plain data.
     */
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

    /**
     * A base/mixin class for structures representing an "info" object for
     * some content types. These include most attachment types currently in
     * the CS API spec.
     *
     * In order to use it in a content class, derive both from TypedBase
     * (or Base) and from FileInfo (or its derivative, such as \p ImageInfo)
     * and call fillInfoJson() to fill the "info" subobject. Make sure
     * to pass an "info" part of JSON to FileInfo constructor, not the whole
     * JSON content, as well as contents of "url" (or a similar key) and
     * optionally "filename" node from the main JSON content. Assuming you
     * don't do unusual things, you should use \p UrlBasedContent<> instead
     * of doing multiple inheritance and overriding Base::fillJson() by hand.
     *
     * This class is not polymorphic.
     */
    class QUOTIENT_API FileInfo {
    public:
        FileInfo() = default;
        explicit FileInfo(const QFileInfo& fi);
        explicit FileInfo(QUrl mxcUrl, qint64 payloadSize = -1,
                          const QMimeType& mimeType = {},
                          Omittable<EncryptedFile> file = none,
                          QString originalFilename = {});
        FileInfo(QUrl mxcUrl, const QJsonObject& infoJson,
                 const Omittable<EncryptedFile> &file,
                 QString originalFilename = {});

        bool isValid() const;

        void fillInfoJson(QJsonObject* infoJson) const;

        /**
         * \brief Extract media id from the URL
         *
         * This can be used, e.g., to construct a QML-facing image://
         * URI as follows:
         * \code "image://provider/" + info.mediaId() \endcode
         */
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

    /**
     * A content info class for image content types: image, thumbnail, video
     */
    class QUOTIENT_API ImageInfo : public FileInfo {
    public:
        ImageInfo() = default;
        explicit ImageInfo(const QFileInfo& fi, QSize imageSize = {});
        explicit ImageInfo(const QUrl& mxcUrl, qint64 fileSize = -1,
                           const QMimeType& type = {}, QSize imageSize = {},
                           const Omittable<EncryptedFile> &file = none,
                           const QString& originalFilename = {});
        ImageInfo(const QUrl& mxcUrl, const QJsonObject& infoJson,
                  const Omittable<EncryptedFile> &encryptedFile,
                  const QString& originalFilename = {});

        void fillInfoJson(QJsonObject* infoJson) const;

    public:
        QSize imageSize;
    };

    /**
     * An auxiliary class for an info type that carries a thumbnail
     *
     * This class saves/loads a thumbnail to/from "info" subobject of
     * the JSON representation of event content; namely,
     * "info/thumbnail_url" and "info/thumbnail_info" fields are used.
     */
    class QUOTIENT_API Thumbnail : public ImageInfo {
    public:
        using ImageInfo::ImageInfo;
        Thumbnail(const QJsonObject& infoJson, const Omittable<EncryptedFile> &file = none);

        /**
         * Writes thumbnail information to "thumbnail_info" subobject
         * and thumbnail URL to "thumbnail_url" node inside "info".
         */
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

    /**
     * A base class for content types that have a URL and additional info
     *
     * Types that derive from this class template take "url" and,
     * optionally, "filename" values from the top-level JSON object and
     * the rest of information from the "info" subobject, as defined by
     * the parameter type.
     *
     * \tparam InfoT base info class
     */
    template <class InfoT>
    class QUOTIENT_API UrlBasedContent : public TypedBase, public InfoT {
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

    /**
     * Content class for m.image
     *
     * Available fields:
     * - corresponding to the top-level JSON:
     *   - url
     *   - filename (extension to the spec)
     * - corresponding to the "info" subobject:
     *   - payloadSize ("size" in JSON)
     *   - mimeType ("mimetype" in JSON)
     *   - imageSize (QSize for a combination of "h" and "w" in JSON)
     *   - thumbnail.url ("thumbnail_url" in JSON)
     * - corresponding to the "info/thumbnail_info" subobject: contents of
     *   thumbnail field, in the same vein as for the main image:
     *   - payloadSize
     *   - mimeType
     *   - imageSize
     */
    using ImageContent = UrlWithThumbnailContent<ImageInfo>;

    /**
     * Content class for m.file
     *
     * Available fields:
     * - corresponding to the top-level JSON:
     *   - url
     *   - filename
     * - corresponding to the "info" subobject:
     *   - payloadSize ("size" in JSON)
     *   - mimeType ("mimetype" in JSON)
     *   - thumbnail.url ("thumbnail_url" in JSON)
     * - corresponding to the "info/thumbnail_info" subobject:
     *   - thumbnail.payloadSize
     *   - thumbnail.mimeType
     *   - thumbnail.imageSize (QSize for "h" and "w" in JSON)
     */
    using FileContent = UrlWithThumbnailContent<FileInfo>;
} // namespace EventContent
} // namespace Quotient
Q_DECLARE_METATYPE(const Quotient::EventContent::TypedBase*)
