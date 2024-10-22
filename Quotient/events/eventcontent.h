// SPDX-FileCopyrightText: 2017 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

// This file contains generic event content definitions, applicable to room
// message events as well as other events (e.g., avatars).

#include "filesourceinfo.h"

#include <QtCore/QJsonObject>
#include <QtCore/QMetaType>
#include <QtCore/QMimeType>
#include <QtCore/QSize>
#include <QtCore/QUrl>

class QFileInfo;

namespace Quotient {
constexpr inline auto InfoKey = "info"_L1;
}

namespace Quotient::EventContent {
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

    virtual QMimeType type() const = 0;

public:
    QJsonObject originalJson;

    // You can't assign those classes
    Base& operator=(const Base&) = delete;
    Base& operator=(Base&&) = delete;

protected:
    Base(const Base&) = default;
    Base(Base&&) noexcept = default;

    virtual void fillJson(QJsonObject&) const = 0;
};

// The below structures fairly follow CS spec 11.2.1.6. The overall
// set of attributes for each content types is a superset of the spec
// but specific aggregation structure is altered. See doc comments to
// each type for the list of available attributes.

// A quick class inheritance structure follows:
// UrlBasedContent<InfoT> : InfoT + thumbnail data
//   PlayableContent<InfoT> : + duration attribute
// FileInfo
//   FileContent = UrlBasedContent<FileInfo>
//   AudioContent = PlayableContent<FileInfo>
//   ImageInfo : FileInfo + imageSize attribute
//     ImageContent = UrlBasedContent<ImageInfo>
//     VideoContent = PlayableContent<ImageInfo>

//! \brief Mix-in class representing `info` subobject in content JSON
//!
//! This is one of base classes for content types that deal with files or URLs. It stores
//! file metadata attributes, such as size, MIME type etc. found in the `content/info` subobject of
//! event JSON payloads. Actual content classes derive from this class _and_ Base that provides
//! a polymorphic interface to access data in the mix-in. FileInfo (as well as ImageInfo, that adds
//! image size to the metadata) is NOT polymorphic and is used in a non-polymorphic way to store
//! thumbnail metadata (in a separate instance), next to the metadata on the file itself.
//!
//! If you need to make a new _content_ (not info) class based on files/URLs take UrlBasedContent
//! as the example, i.e.:
//! 1. Double-inherit from this class (or ImageInfo) and Base.
//! 2. Provide a constructor from QJsonObject that will pass the `info` subobject (not the whole
//!    content JSON) down to FileInfo/ImageInfo.
//! 3. Override fillJson() to customise the JSON export logic. Make sure to call toInfoJson()
//!    from it to produce the payload for the `info` subobject in the JSON payload.
//!
//! \sa ImageInfo, FileContent, ImageContent, AudioContent, VideoContent, UrlBasedContent
struct QUOTIENT_API FileInfo {
    FileInfo() = default;
    //! \brief Construct from a QFileInfo object
    //!
    //! \param fi a QFileInfo object referring to an existing file
    explicit FileInfo(const QFileInfo& fi);
    explicit FileInfo(FileSourceInfo sourceInfo, qint64 payloadSize = -1,
                      const QMimeType& mimeType = {},
                      QString originalFilename = {});
    //! \brief Construct from a JSON `info` payload
    //!
    //! Make sure to pass the `info` subobject of content JSON, not the
    //! whole JSON content.
    FileInfo(FileSourceInfo sourceInfo, const QJsonObject& infoJson,
             QString originalFilename = {});

    bool isValid() const;
    QUrl url() const;

    //! \brief Extract media id from the URL
    //!
    //! This can be used, e.g., to construct a QML-facing image://
    //! URI as follows:
    //! \code "image://provider/" + info.mediaId() \endcode
    QString mediaId() const { return url().authority() + url().path(); }

    FileSourceInfo source;
    QJsonObject originalInfoJson;
    QMimeType mimeType;
    qint64 payloadSize = 0;
    QString originalName;
};

QUOTIENT_API QJsonObject toInfoJson(const FileInfo& info);

//! \brief A content info class for image/video content types and thumbnails
struct QUOTIENT_API ImageInfo : public FileInfo {
    ImageInfo() = default;
    explicit ImageInfo(const QFileInfo& fi, QSize imageSize = {});
    explicit ImageInfo(FileSourceInfo sourceInfo, qint64 fileSize = -1,
                       const QMimeType& type = {}, QSize imageSize = {},
                       const QString& originalFilename = {});
    ImageInfo(FileSourceInfo sourceInfo, const QJsonObject& infoJson,
              const QString& originalFilename = {});

    QSize imageSize;
};

QUOTIENT_API QJsonObject toInfoJson(const ImageInfo& info);

//! \brief An auxiliary class for an info type that carries a thumbnail
//!
//! This class saves/loads a thumbnail to/from `info` subobject of
//! the JSON representation of event content; namely, `info/thumbnail_url`
//! (or, in case of an encrypted thumbnail, `info/thumbnail_file`) and
//! `info/thumbnail_info` fields are used.
struct QUOTIENT_API Thumbnail : public ImageInfo {
    using ImageInfo::ImageInfo;
    explicit Thumbnail(const QJsonObject& infoJson);

    //! \brief Add thumbnail information to the passed `info` JSON object
    void dumpTo(QJsonObject& infoJson) const;
};

//! The base for all file-based content classes
class QUOTIENT_API FileContentBase : public Base {
public:
    using Base::Base;
    virtual QUrl url() const = 0;
};

//! \brief Rich text content for m.text, m.emote, m.notice
//!
//! Available fields: mimeType, body. The body can be either rich text
//! or plain text, depending on what mimeType specifies.
class QUOTIENT_API TextContent : public Base {
public:
    TextContent(QString text, const QString& contentType);
    explicit TextContent(const QJsonObject& json);

    QMimeType type() const override { return mimeType; }

    QMimeType mimeType;
    QString body;

protected:
    void fillJson(QJsonObject& json) const override;
};

//! \brief Content class for m.location
//!
//! Available fields:
//!     - corresponding to the top-level JSON:
//!         - geoUri ("geo_uri" in JSON)
//!     - corresponding to the "info" subobject:
//!         - thumbnail.url ("thumbnail_url" in JSON)
//!     - corresponding to the "info/thumbnail_info" subobject:
//!         - thumbnail.payloadSize
//!         - thumbnail.mimeType
//!         - thumbnail.imageSize
class QUOTIENT_API LocationContent : public Base {
public:
    LocationContent(const QString& geoUri, const Thumbnail& thumbnail = {});
    explicit LocationContent(const QJsonObject& json);

    QMimeType type() const override;

public:
    QString geoUri;
    Thumbnail thumbnail;

protected:
    void fillJson(QJsonObject& o) const override;
};

//! \brief A template class for content types with a URL and additional info
//!
//! Types that derive from this class template take `url` (or, if the file
//! is encrypted, `file`) and, optionally, `filename` values from
//! the top-level JSON object and the rest of information from the `info`
//! subobject, as defined by the parameter type.
//! \tparam InfoT base info class - FileInfo or ImageInfo
template <std::derived_from<FileInfo> InfoT>
class UrlBasedContent : public FileContentBase, public InfoT {
public:
    using InfoT::InfoT;
    explicit UrlBasedContent(const QJsonObject& json)
        : FileContentBase(json)
        , InfoT(fileSourceInfoFromJson(json, { "url"_L1, "file"_L1 }), json[InfoKey].toObject(),
                json["filename"_L1].toString())
        , thumbnail(FileInfo::originalInfoJson)
    {
        // Two small hacks on originalJson to expose mediaIds to QML
        originalJson.insert("mediaId"_L1, InfoT::mediaId());
        originalJson.insert("thumbnailMediaId"_L1, thumbnail.mediaId());
    }

    QMimeType type() const override { return InfoT::mimeType; }
    QUrl url() const override { return InfoT::url(); }

public:
    Thumbnail thumbnail;

protected:
    virtual void fillInfoJson(QJsonObject& infoJson [[maybe_unused]]) const
    {}

    void fillJson(QJsonObject& json) const override
    {
        Quotient::fillJson(json, { "url"_L1, "file"_L1 }, InfoT::source);
        if (!InfoT::originalName.isEmpty())
            json.insert("filename"_L1, InfoT::originalName);
        auto infoJson = toInfoJson(*this);
        if (thumbnail.isValid())
            thumbnail.dumpTo(infoJson);
        fillInfoJson(infoJson);
        json.insert(InfoKey, infoJson);
    }
};

//! \brief Content class for m.image
//!
//! Available fields:
//! - corresponding to the top-level JSON:
//!   - source (corresponding to `url` or `file` in JSON)
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
//!   - source (corresponding to `url` or `file` in JSON)
//!   - filename
//! - corresponding to the `info` subobject:
//!   - payloadSize (`size` in JSON)
//!   - mimeType (`mimetype` in JSON)
//!   - thumbnail.source (`thumbnail_url` or `thumbnail_file` in JSON)
//! - corresponding to the `info/thumbnail_info` subobject:
//!   - thumbnail.payloadSize
//!   - thumbnail.mimeType
//!   - thumbnail.imageSize (QSize for `h` and `w` in JSON)
using FileContent = UrlBasedContent<FileInfo>;

//! A base class for info types that include duration: audio and video
template <typename InfoT>
class PlayableContent : public UrlBasedContent<InfoT> {
public:
    using UrlBasedContent<InfoT>::UrlBasedContent;
    explicit PlayableContent(const QJsonObject& json)
        : UrlBasedContent<InfoT>(json), duration(FileInfo::originalInfoJson["duration"_L1].toInt())
    {}

protected:
    void fillInfoJson(QJsonObject& infoJson) const override
    {
        infoJson.insert("duration"_L1, duration);
}

public:
    int duration;
};

//! \brief Content class for m.video
//!
//! Available fields:
//!     - corresponding to the top-level JSON:
//!         - url
//!         - filename (extension to the CS API spec)
//!     - corresponding to the "info" subobject:
//!         - payloadSize ("size" in JSON)
//!         - mimeType ("mimetype" in JSON)
//!         - duration
//!         - imageSize (QSize for a combination of "h" and "w" in JSON)
//!         - thumbnail.url ("thumbnail_url" in JSON)
//!     - corresponding to the "info/thumbnail_info" subobject: contents of
//!       thumbnail field, in the same vein as for "info":
//!         - payloadSize
//!         - mimeType
//!         - imageSize
using VideoContent = PlayableContent<ImageInfo>;

//! \brief Content class for m.audio
//!
//! Available fields:
//!     - corresponding to the top-level JSON:
//!         - url
//!         - filename (extension to the CS API spec)
//!     - corresponding to the "info" subobject:
//!         - payloadSize ("size" in JSON)
//!         - mimeType ("mimetype" in JSON)
//!         - duration
//!         - thumbnail.url ("thumbnail_url" in JSON)
//!     - corresponding to the "info/thumbnail_info" subobject: contents of
//!       thumbnail field, in the same vein as for "info":
//!         - payloadSize
//!         - mimeType
//!         - imageSize
using AudioContent = PlayableContent<FileInfo>;
} // namespace Quotient::EventContent
Q_DECLARE_METATYPE(const Quotient::EventContent::Base*)
