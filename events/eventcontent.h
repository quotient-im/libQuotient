/******************************************************************************
 * Copyright (C) 2017 Kitsune Ral <kitsune-ral@users.sf.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#pragma once

// This file contains generic event content definitions, applicable to room
// message events as well as other events (e.g., avatars).

#include "converters.h"

#include <QtCore/QMimeType>
#include <QtCore/QUrl>
#include <QtCore/QSize>

namespace QMatrixClient
{
    namespace EventContent
    {
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
        class Base
        {
            public:
                explicit Base (const QJsonObject& o = {}) : originalJson(o) { }
                virtual ~Base() = default;

                QJsonObject toJson() const;

            public:
                QJsonObject originalJson;

            protected:
                virtual void fillJson(QJsonObject* o) const = 0;
        };

        template <typename T = QString>
        class SimpleContent: public Base
        {
            public:
                using value_type = T;

                // The constructor is templated to enable perfect forwarding
                template <typename TT>
                SimpleContent(QString keyName, TT&& value)
                    : value(std::forward<TT>(value)), key(std::move(keyName))
                { }
                SimpleContent(const QJsonObject& json, QString keyName)
                    : Base(json)
                    , value(QMatrixClient::fromJson<T>(json[keyName]))
                    , key(std::move(keyName))
                { }

            public:
                T value;

            protected:
                QString key;

            private:
                void fillJson(QJsonObject* json) const override
                {
                    Q_ASSERT(json);
                    json->insert(key, QMatrixClient::toJson(value));
                }
        };

        // The below structures fairly follow CS spec 11.2.1.6. The overall
        // set of attributes for each content types is a superset of the spec
        // but specific aggregation structure is altered. See doc comments to
        // each type for the list of available attributes.

        // A quick classes inheritance structure follows:
        // FileInfo
        //   FileContent : UrlBasedContent<FileInfo, Thumbnail>
        //   AudioContent : UrlBasedContent<FileInfo, Duration>
        //   ImageInfo : FileInfo + imageSize attribute
        //     ImageContent : UrlBasedContent<ImageInfo, Thumbnail>
        //     VideoContent : UrlBasedContent<ImageInfo, Thumbnail, Duration>

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
        class FileInfo
        {
            public:
                explicit FileInfo(const QUrl& u, int payloadSize = -1,
                                  const QMimeType& mimeType = {},
                                  const QString& originalFilename = {});
                FileInfo(const QUrl& u, const QJsonObject& infoJson,
                         const QString& originalFilename = {});

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
                int payloadSize;
                QString originalName;
        };

        /**
         * A content info class for image content types: image, thumbnail, video
         */
        class ImageInfo : public FileInfo
        {
            public:
                explicit ImageInfo(const QUrl& u, int fileSize = -1,
                                   QMimeType mimeType = {},
                                   const QSize& imageSize = {});
                ImageInfo(const QUrl& u, const QJsonObject& infoJson,
                          const QString& originalFilename = {});

                void fillInfoJson(QJsonObject* infoJson) const;

            public:
                QSize imageSize;
        };

        /**
         * A mixin class for an info type that carries a thumbnail
         *
         * This class saves/loads a thumbnail to/from "info" subobject of
         * the JSON representation of event content; namely,
         * "info/thumbnail_url" and "info/thumbnail_info" fields are used.
         */
        class WithThumbnail
        {
            public:
                WithThumbnail(const QJsonObject& infoJson);
                WithThumbnail(const ImageInfo& info)
                    : thumbnail(info)
                { }

                /**
                 * Writes thumbnail information to "thumbnail_info" subobject
                 * and thumbnail URL to "thumbnail_url" node inside "info".
                 */
                void fillInfoJson(QJsonObject* infoJson) const;

            public:
                ImageInfo thumbnail;
        };

        class TypedBase: public Base
        {
            public:
                explicit TypedBase(const QJsonObject& o = {}) : Base(o) { }
                virtual QMimeType type() const = 0;
                virtual const FileInfo* fileInfo() const { return nullptr; }
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
         * \tparam InfoMixinTs... additional info mixin classes (e.g. WithThumbnail)
         */
        template <class InfoT, class... InfoMixinTs>
        class UrlBasedContent :
                public TypedBase, public InfoT, public InfoMixinTs...
        {
            public:
                explicit UrlBasedContent(const QJsonObject& json)
                    : TypedBase(json)
                    , InfoT(json["url"].toString(), json["info"].toObject(),
                            json["filename"].toString())
                    , InfoMixinTs(InfoT::originalInfoJson)...
                { }

                QMimeType type() const override { return InfoT::mimeType; }
                const FileInfo* fileInfo() const override { return this; }

            protected:
                void fillJson(QJsonObject* json) const override
                {
                    Q_ASSERT(json);
                    json->insert("url", InfoT::url.toString());
                    if (!InfoT::originalName.isEmpty())
                        json->insert("filename", InfoT::originalName);
                    QJsonObject infoJson;
                    InfoT::fillInfoJson(&infoJson);
                    // http://en.cppreference.com/w/cpp/language/parameter_pack#Brace-enclosed_initializers
                    // Looking forward to C++17 and its folding awesomeness.
                    int d[] = { (InfoMixinTs::fillInfoJson(&infoJson), 0)... };
                    Q_UNUSED(d);
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
        using ImageContent = UrlBasedContent<ImageInfo, WithThumbnail>;

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
        using FileContent = UrlBasedContent<FileInfo, WithThumbnail>;
    }  // namespace EventContent
}  // namespace QMatrixClient
