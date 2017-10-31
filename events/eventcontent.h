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

#include <QtCore/QJsonObject>
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
         * from plain data. fillJson() should only fill the main JSON object
         * but not the "info" subobject if it exists for a certain content type;
         * use \p InfoBase to de/serialize "info" parts with an optional URL
         * on the top level.
         */
        class Base
        {
            public:
                virtual ~Base() = default;

                QJsonObject toJson() const;

            protected:
                virtual void fillJson(QJsonObject* o) const = 0;
        };

        class TypedBase: public Base
        {
            public:
                virtual QMimeType type() const = 0;
        };

        /**
         * A base class for content types that have an "info" object in their
         * JSON representation
         *
         * These include most multimedia types currently in the CS API spec.
         * Derived classes should override fillInfoJson() to fill the "info"
         * subobject, BUT NOT the main JSON object. Most but not all "info"
         * classes (specifically, those deriving from FileInfo) should also
         * have a constructor that accepts two parameters, QUrl and QJsonObject,
         * in order to load the URL+info part from JSON.
         */
        class InfoBase
        {
            public:
                virtual ~InfoBase() = default;

                QJsonObject toInfoJson() const;

                QMimeType mimeType;

            protected:
                InfoBase() = default;
                explicit InfoBase(const QMimeType& type) : mimeType(type) { }

                virtual void fillInfoJson(QJsonObject* /*infoJson*/) const = 0;
        };

        // The below structures fairly follow CS spec 11.2.1.6. The overall
        // set of attributes for each content types is a superset of the spec
        // but specific aggregation structure is altered. See doc comments to
        // each type for the list of available attributes.

        /**
         * Base class for content types that consist of a URL along with
         * additional information. Most of message types except textual fall
         * under this category.
         */
        class FileInfo: public InfoBase
        {
            public:
                explicit FileInfo(const QUrl& u, int payloadSize = -1,
                                  const QMimeType& mimeType = {},
                                  const QString& originalFilename = {});
                FileInfo(const QUrl& u, const QJsonObject& infoJson,
                         const QString& originalFilename = {});

                QUrl url;
                int payloadSize;
                QString originalName;

            protected:
                void fillInfoJson(QJsonObject* infoJson) const override;
        };

        /**
         * A base class for image info types: image, thumbnail, video
         *
         * \tparam InfoT base info class; should derive from \p InfoBase
         */
        template <class InfoT = FileInfo>
        class ImageInfo : public InfoT
        {
            public:
                explicit ImageInfo(const QUrl& u, int fileSize = -1,
                                   QMimeType mimeType = {},
                                   const QSize& imageSize = {})
                    : InfoT(u, fileSize, mimeType), imageSize(imageSize)
                { }
                ImageInfo(const QUrl& u, const QJsonObject& infoJson,
                          const QString& originalFilename = {})
                    : InfoT(u, infoJson, originalFilename)
                    , imageSize(infoJson["w"].toInt(), infoJson["h"].toInt())
                { }

                QSize imageSize;

            protected:
                void fillInfoJson(QJsonObject* infoJson) const override
                {
                    InfoT::fillInfoJson(infoJson);
                    infoJson->insert("w", imageSize.width());
                    infoJson->insert("h", imageSize.height());
                }
        };

        /**
         * A base class for an info type that carries a thumbnail
         *
         * This class decorates the underlying type, adding ability to save/load
         * a thumbnail to/from "info" subobject of the JSON representation of
         * event content; namely, "info/thumbnail_url" and "info/thumbnail_info"
         * fields are used.
         *
         * \tparam InfoT base info class; should derive from \p InfoBase
         */
        template <class InfoT = InfoBase>
        class Thumbnailed : public InfoT
        {
            public:
                template <typename... ArgTs>
                explicit Thumbnailed(const ImageInfo<>& thumbnail,
                                     ArgTs&&... infoArgs)
                    : InfoT(std::forward<ArgTs>(infoArgs)...)
                    , thumbnail(thumbnail)
                { }

                explicit Thumbnailed(const QJsonObject& infoJson)
                    : thumbnail(infoJson["thumbnail_url"].toString(),
                                infoJson["thumbnail_info"].toObject())
                { }

                Thumbnailed(const QUrl& u, const QJsonObject& infoJson,
                            const QString& originalFilename = {})
                    : InfoT(u, infoJson, originalFilename)
                    , thumbnail(infoJson["thumbnail_url"].toString(),
                                infoJson["thumbnail_info"].toObject())
                { }

                ImageInfo<> thumbnail;

            protected:
                void fillInfoJson(QJsonObject* infoJson) const override
                {
                    InfoT::fillInfoJson(infoJson);
                    infoJson->insert("thumbnail_url", thumbnail.url.toString());
                    infoJson->insert("thumbnail_info", thumbnail.toInfoJson());
                }
        };

        /**
         * One more facility base class for content types that have a URL and
         * additional info
         *
         * Types that derive from UrlWith<InfoT> take "url" and, optionally,
         * "filename" values from the top-level JSON object and the rest of
         * information from the "info" subobject.
         *
         * \tparam InfoT base info class; should derive from \p FileInfo or
         * provide a constructor with a compatible signature
         */
        template <class InfoT> // InfoT : public FileInfo
        class UrlWith : public TypedBase, public InfoT
        {
            public:
                using InfoT::InfoT;
                explicit UrlWith(const QJsonObject& json)
                    : InfoT(json["url"].toString(), json["info"].toObject(),
                            json["filename"].toString())
                { }

                QMimeType type() const override { return InfoT::mimeType; }

            protected:
                void fillJson(QJsonObject* json) const override
                {
                    Q_ASSERT(json);
                    json->insert("url", InfoT::url.toString());
                    if (!InfoT::originalName.isEmpty())
                        json->insert("filename", InfoT::originalName);
                    json->insert("info", InfoT::toInfoJson());
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
        using ImageContent = UrlWith<Thumbnailed<ImageInfo<>>>;

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
        using FileContent = UrlWith<Thumbnailed<FileInfo>>;
    }  // namespace EventContent
}  // namespace QMatrixClient
