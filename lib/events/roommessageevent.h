// SPDX-FileCopyrightText: 2015 Felix Rohrbach <kde@fxrh.de>
// SPDX-FileCopyrightText: 2016 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-FileCopyrightText: 2017 Roman Plášil <me@rplasil.name>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "eventcontent.h"
#include "roomevent.h"

class QFileInfo;

namespace Quotient {
namespace MessageEventContent = EventContent; // Back-compatibility

/**
 * The event class corresponding to m.room.message events
 */
class RoomMessageEvent : public RoomEvent {
    Q_GADGET
    Q_PROPERTY(QString msgType READ rawMsgtype CONSTANT)
    Q_PROPERTY(QString plainBody READ plainBody CONSTANT)
    Q_PROPERTY(QMimeType mimeType READ mimeType STORED false CONSTANT)
    Q_PROPERTY(const EventContent::TypedBase* content READ content CONSTANT)
public:
    DEFINE_EVENT_TYPEID("m.room.message", RoomMessageEvent)

    enum class MsgType {
        Text,
        Emote,
        Notice,
        Image,
        File,
        Location,
        Video,
        Audio,
        Unknown
    };

    RoomMessageEvent(const QString& plainBody, const QString& jsonMsgType,
                     EventContent::TypedBase* content = nullptr);
    explicit RoomMessageEvent(const QString& plainBody,
                              MsgType msgType = MsgType::Text,
                              EventContent::TypedBase* content = nullptr);
#if QT_VERSION_MAJOR < 6
    [[deprecated("Create an EventContent object on the client side"
                 " and pass it to other constructors")]] //
    explicit RoomMessageEvent(const QString& plainBody, const QFileInfo& file,
                              bool asGenericFile = false);
#endif
    explicit RoomMessageEvent(const QJsonObject& obj);

    MsgType msgtype() const;
    QString rawMsgtype() const;
    QString plainBody() const;
    const EventContent::TypedBase* content() const { return _content.data(); }
    template <typename VisitorT>
    void editContent(VisitorT&& visitor)
    {
        visitor(*_content);
        editJson()[ContentKeyL] = assembleContentJson(plainBody(), rawMsgtype(),
                                                      _content.data());
    }
    QMimeType mimeType() const;
    //! \brief Determine whether the message has text content
    //!
    //! \return true, if the message type is one of m.text, m.notice, m.emote,
    //!         or the message type is unspecified (in which case plainBody()
    //!         can still be examined); false otherwise
    bool hasTextContent() const;
    //! \brief Determine whether the message has a file/attachment
    //!
    //! \return true, if the message has a data structure corresponding to
    //!         a file (such as m.file or m.audio); false otherwise
    bool hasFileContent() const;
    //! \brief Determine whether the message has a thumbnail
    //!
    //! \return true, if the message has a data structure corresponding to
    //!         a thumbnail (the message type may be one for visual content,
    //!         such as m.image, or generic binary content, i.e. m.file);
    //!         false otherwise
    bool hasThumbnail() const;
    //! \brief Obtain id of an event replaced by the current one
    //! \sa RoomEvent::isReplaced, RoomEvent::replacedBy
    QString replacedEvent() const;

    static QString rawMsgTypeForUrl(const QUrl& url);
    static QString rawMsgTypeForFile(const QFileInfo& fi);

private:
    QScopedPointer<EventContent::TypedBase> _content;

    // FIXME: should it really be static?
    static QJsonObject assembleContentJson(const QString& plainBody,
                                           const QString& jsonMsgType,
                                           EventContent::TypedBase* content);

    Q_ENUM(MsgType)
};
REGISTER_EVENT_TYPE(RoomMessageEvent)
using MessageEventType = RoomMessageEvent::MsgType;

namespace EventContent {
    // Additional event content types

    struct RelatesTo {
        static constexpr const char* ReplyTypeId() { return "m.in_reply_to"; }
        static constexpr const char* ReplacementTypeId() { return "m.replace"; }
        QString type; // The only supported relation so far
        QString eventId;
    };
    inline RelatesTo replyTo(QString eventId)
    {
        return { RelatesTo::ReplyTypeId(), std::move(eventId) };
    }
    inline RelatesTo replacementOf(QString eventId)
    {
        return { RelatesTo::ReplacementTypeId(), std::move(eventId) };
    }

    /**
     * Rich text content for m.text, m.emote, m.notice
     *
     * Available fields: mimeType, body. The body can be either rich text
     * or plain text, depending on what mimeType specifies.
     */
    class TextContent : public TypedBase {
    public:
        TextContent(QString text, const QString& contentType,
                    Omittable<RelatesTo> relatesTo = none);
        explicit TextContent(const QJsonObject& json);

        QMimeType type() const override { return mimeType; }

        QMimeType mimeType;
        QString body;
        Omittable<RelatesTo> relatesTo;

    protected:
        void fillJson(QJsonObject* json) const override;
    };

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
    class LocationContent : public TypedBase {
    public:
        LocationContent(const QString& geoUri, const Thumbnail& thumbnail = {});
        explicit LocationContent(const QJsonObject& json);

        QMimeType type() const override;

    public:
        QString geoUri;
        Thumbnail thumbnail;

    protected:
        void fillJson(QJsonObject* o) const override;
    };

    /**
     * A base class for info types that include duration: audio and video
     */
    template <typename ContentT>
    class PlayableContent : public ContentT {
    public:
        using ContentT::ContentT;
        PlayableContent(const QJsonObject& json)
            : ContentT(json)
            , duration(ContentT::originalInfoJson["duration"_ls].toInt())
        {}

    protected:
        void fillJson(QJsonObject* json) const override
        {
            ContentT::fillJson(json);
            auto infoJson = json->take("info"_ls).toObject();
            infoJson.insert(QStringLiteral("duration"), duration);
            json->insert(QStringLiteral("info"), infoJson);
        }

    public:
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
    using VideoContent = PlayableContent<UrlWithThumbnailContent<ImageInfo>>;

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
    using AudioContent = PlayableContent<UrlBasedContent<FileInfo>>;
} // namespace EventContent
} // namespace Quotient
