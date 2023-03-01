// SPDX-FileCopyrightText: 2015 Felix Rohrbach <kde@fxrh.de>
// SPDX-FileCopyrightText: 2016 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-FileCopyrightText: 2017 Roman Plášil <me@rplasil.name>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "eventcontent.h"
#include "eventrelation.h"
#include "roomevent.h"

class QFileInfo;

namespace Quotient {
namespace MessageEventContent = EventContent; // Back-compatibility

/**
 * The event class corresponding to m.room.message events
 */
class QUOTIENT_API RoomMessageEvent : public RoomEvent {
    Q_GADGET
public:
    QUO_EVENT(RoomMessageEvent, "m.room.message")

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

    //! \brief Determine whether the event has been replaced
    //!
    //! \return true if this event has been overridden by another event
    //!         with `"rel_type": "m.replace"`; false otherwise
    bool isReplaced() const;

    QString replacedBy() const;

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

using MessageEventType = RoomMessageEvent::MsgType;

namespace EventContent {

    struct [[deprecated("Use Quotient::EventRelation instead")]] RelatesTo
        : EventRelation {
        static constexpr auto ReplyTypeId() { return ReplyType; }
        static constexpr auto ReplacementTypeId() { return ReplacementType; }
    };
    [[deprecated("Use EventRelation::replyTo() instead")]]
    inline auto replyTo(QString eventId)
    {
        return EventRelation::replyTo(std::move(eventId));
    }
    [[deprecated("Use EventRelation::replace() instead")]]
    inline auto replacementOf(QString eventId)
    {
        return EventRelation::replace(std::move(eventId));
    }

    // Additional event content types

    /**
     * Rich text content for m.text, m.emote, m.notice
     *
     * Available fields: mimeType, body. The body can be either rich text
     * or plain text, depending on what mimeType specifies.
     */
    class QUOTIENT_API TextContent : public TypedBase {
    public:
        TextContent(QString text, const QString& contentType,
                    Omittable<EventRelation> relatesTo = none);
        explicit TextContent(const QJsonObject& json);

        QMimeType type() const override { return mimeType; }

        QMimeType mimeType;
        QString body;
        Omittable<EventRelation> relatesTo;

    protected:
        void fillJson(QJsonObject& json) const override;
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
    class QUOTIENT_API LocationContent : public TypedBase {
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

    /**
     * A base class for info types that include duration: audio and video
     */
    template <typename InfoT>
    class PlayableContent : public UrlBasedContent<InfoT> {
    public:
        using UrlBasedContent<InfoT>::UrlBasedContent;
        PlayableContent(const QJsonObject& json)
            : UrlBasedContent<InfoT>(json)
            , duration(FileInfo::originalInfoJson["duration"_ls].toInt())
        {}

    protected:
        void fillInfoJson(QJsonObject& infoJson) const override
        {
            infoJson.insert(QStringLiteral("duration"), duration);
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
    using VideoContent = PlayableContent<ImageInfo>;

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
     *   - thumbnail.url ("thumbnail_url" in JSON - extension to the spec)
     * - corresponding to the "info/thumbnail_info" subobject: contents of
     *   thumbnail field (extension to the spec):
     *   - payloadSize
     *   - mimeType
     *   - imageSize
     */
    using AudioContent = PlayableContent<FileInfo>;
} // namespace EventContent
} // namespace Quotient
