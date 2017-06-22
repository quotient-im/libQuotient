/******************************************************************************
 * Copyright (C) 2015 Felix Rohrbach <kde@fxrh.de>
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

#include "event.h"

#include <QtCore/QUrl>
#include <QtCore/QMimeType>
#include <QtCore/QSize>

namespace QMatrixClient
{
    namespace MessageEventContent
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

                QMimeType mimeType;

            protected:
                Base() = default;
                explicit Base(const QMimeType& type) : mimeType(type) { }

                virtual void fillJson(QJsonObject* o) const = 0;
        };

        /**
         * A base class for content types that have an "info" object in their
         * JSON representation
         *
         * These include most multimedia types currently in the CS API spec.
         * Derived classes should override fillInfoJson() to fill the "info"
         * subobject, BUT NOT the main JSON object. Most but not all "info"
         * classes (specifically, those deriving from UrlInfo) should also
         * have a constructor that accepts two parameters, QUrl and QJsonObject,
         * in order to load the URL+info part from JSON.
         */
        class InfoBase: public Base
        {
            public:
                QJsonObject toInfoJson() const;

            protected:
                using Base::Base;

                virtual void fillInfoJson(QJsonObject* infoJson) const { }
        };
    }  // namespace MessageEventContent

    /**
     * The event class corresponding to m.room.message events
     */
    class RoomMessageEvent: public RoomEvent
    {
            Q_GADGET
        public:
            enum class MsgType
            {
                Text, Emote, Notice, Image, File, Location, Video, Audio, Unknown
            };

            RoomMessageEvent(const QString& roomId, const QString& fromUserId,
                             const QString& plainMessage,
                             MsgType msgType = MsgType::Text)
                : RoomEvent(Type::RoomMessage, roomId, fromUserId)
                , _msgtype(msgType), _plainBody(plainMessage), _content(nullptr)
            { }
            RoomMessageEvent(const QString& roomId, const QString& fromUserId,
                             const QString& plainBody,
                             MessageEventContent::Base* content,
                             MsgType msgType)
                : RoomEvent(Type::RoomMessage, roomId, fromUserId)
                , _msgtype(msgType), _plainBody(plainBody), _content(content)
            { }
            explicit RoomMessageEvent(const QJsonObject& obj);

            MsgType msgtype() const          { return _msgtype; }
            const QString& plainBody() const { return _plainBody; }
            const MessageEventContent::Base* content() const { return _content.data(); }
            QMimeType mimeType() const;

            QJsonObject toJson() const;

            static constexpr const char* TypeId = "m.room.message";

        private:
            MsgType _msgtype;
            QString _plainBody;
            QScopedPointer<MessageEventContent::Base> _content;

            REGISTER_ENUM(MsgType)
    };
    using MessageEventType = RoomMessageEvent::MsgType;

    namespace MessageEventContent
    {
        // The below structures fairly follow CS spec 11.2.1.6. The overall
        // set of attributes for each content types is a superset of the spec
        // but specific aggregation structure is altered. See doc comments to
        // each type for the list of available attributes.

        /**
         * Rich text content for m.text, m.emote, m.notice
         *
         * Available fields: mimeType, body. The body can be either rich text
         * or plain text, depending on what mimeType specifies.
         */
        class TextContent: public Base
        {
            public:
                TextContent(const QString& text, const QString& contentType);
                explicit TextContent(const QJsonObject& json);

                void fillJson(QJsonObject* json) const override;

                QString body;
        };

        /**
         * Base class for content types that consist of a URL along with
         * additional information
         *
         * All message types except the (hyper)text mentioned above and
         * m.location fall under this category.
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
                void fillJson(QJsonObject* json) const override;
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

                void fillInfoJson(QJsonObject* infoJson) const /* override */
                {
                    InfoT::fillInfoJson(infoJson);
                    infoJson->insert("w", imageSize.width());
                    infoJson->insert("h", imageSize.height());
                }

                QSize imageSize;
        };

        /**
         * A base class for an info type that carries a thumbnail
         *
         * This class provides a means to save/load a thumbnail to/from "info"
         * subobject of the JSON representation of a message; namely,
         * "info/thumbnail_url" and "info/thumbnail_info" fields are used.
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

                void fillInfoJson(QJsonObject* infoJson) const /* override */
                {
                    InfoT::fillInfoJson(infoJson);
                    infoJson->insert("thumbnail_url", thumbnail.url.toString());
                    infoJson->insert("thumbnail_info", thumbnail.toInfoJson());
                }

                ImageInfo<> thumbnail;
        };

        /**
         * One more facility base class for content types that have a URL and
         * additional info
         *
         * The assumed layout for types enabled by a combination of UrlInfo and
         * UrlWith<> is the following: "url" and, optionally, "filename" in the
         * top-level JSON and the rest of information inside the "info" subobject.
         *
         * \tparam InfoT base info class; should derive from \p UrlInfo or
         * provide a constructor with a compatible signature
         */
        template <class InfoT> // InfoT : public FileInfo
        class UrlWith : public InfoT
        {
            public:
                using InfoT::InfoT;
                explicit UrlWith(const QJsonObject& json)
                    : InfoT(json["url"].toString(), json["info"].toObject(),
                            json["filename"].toString())
                { }
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

        /**
         * Content class for m.location
         *
         * Available fields:
         * - corresponding to the top-level JSON:
         *   - geoUri ("geo_uri" in JSON)
         * - corresponding to the "info" subobject:
         *   - thumbnail.url ("thumbnail_url" in JSON)
         * - corresponding to the "info/thumbnail_info" subobject:
         *   - thumbnail.payloadSize
         *   - thumbnail.mimeType
         *   - thumbnail.imageSize
         */
        class LocationContent: public Thumbnailed<>
        {
            public:
                LocationContent(const QString& geoUri,
                                const ImageInfo<>& thumbnail);
                explicit LocationContent(const QJsonObject& json);

                void fillJson(QJsonObject* o) const override;

                QString geoUri;
        };

        /**
         * A base class for "playable" info types: audio and video
         */
        class PlayableInfo : public FileInfo
        {
            public:
                explicit PlayableInfo(const QUrl& u, int fileSize,
                                      const QMimeType& mimeType, int duration,
                                      const QString& originalFilename = {});
                PlayableInfo(const QUrl& u, const QJsonObject& infoJson,
                             const QString& originalFilename = {});

                void fillInfoJson(QJsonObject* infoJson) const override;

                int duration;
        };

        /**
         * Content class for m.video
         *
         * Available fields:
         * - corresponding to the top-level JSON:
         *   - url
         *   - filename (extension to the CS API spec)
         * - corresponding to the "info" subobject:
         *   - payloadSize ("size" in JSON)
         *   - mimeType ("mimetype" in JSON)
         *   - duration
         *   - imageSize (QSize for a combination of "h" and "w" in JSON)
         *   - thumbnail.url ("thumbnail_url" in JSON)
         * - corresponding to the "info/thumbnail_info" subobject: contents of
         *   thumbnail field, in the same vein as for "info":
         *   - payloadSize
         *   - mimeType
         *   - imageSize
         */
        using VideoContent = UrlWith<Thumbnailed<ImageInfo<PlayableInfo>>>;

        /**
         * Content class for m.audio
         *
         * Available fields:
         * - corresponding to the top-level JSON:
         *   - url
         *   - filename (extension to the CS API spec)
         * - corresponding to the "info" subobject:
         *   - payloadSize ("size" in JSON)
         *   - mimeType ("mimetype" in JSON)
         *   - duration
         */
        using AudioContent = UrlWith<PlayableInfo>;
    }  // namespace MessageEventContent
}  // namespace QMatrixClient
