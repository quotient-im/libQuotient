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
                     EventContent::TypedBase* content = nullptr,
                     std::optional<EventRelation> relatesTo = std:: nullopt);
    explicit RoomMessageEvent(const QString& plainBody,
                              MsgType msgType = MsgType::Text,
                              EventContent::TypedBase* content = nullptr,
                              std::optional<EventRelation> relatesTo = std:: nullopt);

    explicit RoomMessageEvent(const QJsonObject& obj);

    MsgType msgtype() const;
    QString rawMsgtype() const;
    QString plainBody() const;
    const EventContent::TypedBase* content() const { return _content.get(); }
    void editContent(auto visitor)
    {
        visitor(*_content);
        editJson()[ContentKey] = assembleContentJson(plainBody(), rawMsgtype(), _content.get(), _relatesTo);
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

    //! \brief The EventRelation for this event.
    //!
    //! \return an EventRelation object which can be checked for type if it exists,
    //!         std::nullopt otherwise.
    std::optional<EventRelation> relatesTo() const;

    //! \brief The upstream event ID for the relation.
    //!
    //! \warning If your client is not thread aware use replyEventId() as this will
    //!          return the fallback reply ID so you can treat a threaded reply like a normal one.
    //!
    //! \warning If your client is thread aware use threadRootEventId() to get the
    //!          thread root ID as this will return an empty string on the root event.
    //!          threadRootEventId() will return the root messages ID on itself.
    QString upstreamEventId() const;

    //! \brief Obtain id of an event replaced by the current one
    //! \sa RoomEvent::isReplaced, RoomEvent::replacedBy
    QString replacedEvent() const;

    //! \brief Determine whether the event has been replaced
    //!
    //! \return true if this event has been overridden by another event
    //!         with `"rel_type": "m.replace"`; false otherwise
    bool isReplaced() const;

    QString replacedBy() const;

    //! \brief Determine whether the event is a reply to another message.
    //!
    //! \param includeFallbacks include thread fallback replies for non-threaded clients.
    //!
    //! \return true if this event is a reply, i.e. it has `"m.in_reply_to"`
    //!         event ID and is not a thread fallback (except where \p includeFallbacks is true);
    //!         false otherwise.
    //!
    //! \note It's possible to reply to another message in a thread so this function
    //!       will return true for a `"rel_type"` of `"m.thread"` if `"is_falling_back"`
    //!       is false.
    bool isReply(bool includeFallbacks = false) const;

    //! \brief The ID for the event being replied to.
    //!
    //! \param includeFallbacks include thread fallback replies for non-threaded clients.
    //!
    //!
    //! \return The event ID for a reply, this includes threaded replies where `"rel_type"`
    //!         is `"m.thread"` and `"is_falling_back"` is false (except where \p includeFallbacks is true).
    QString replyEventId(bool includeFallbacks = false)const;

    //! \brief Determine whether the event is part of a thread.
    //!
    //! \return true if this event is part of a thread, i.e. it has
    //!         `"rel_type": "m.thread"` or  `"m.relations": { "m.thread": {}}`;
    //!         false otherwise.
    bool isThreaded() const;

    //! \brief The event ID for the thread root event.
    //!
    //! \note This will return the ID of the event if it is the thread root.
    //!
    //! \return The event ID of the thread root if threaded, an empty string otherwise.
    QString threadRootEventId()const;

    QString fileNameToDownload() const;

    static QString rawMsgTypeForUrl(const QUrl& url);
    static QString rawMsgTypeForFile(const QFileInfo& fi);

private:
    std::unique_ptr<EventContent::TypedBase> _content;
    std::optional<EventRelation> _relatesTo;

    // FIXME: should it really be static?
    static QJsonObject assembleContentJson(const QString& plainBody,
                                           const QString& jsonMsgType,
                                           EventContent::TypedBase* content,
                                           std::optional<EventRelation> relatesTo);

    Q_ENUM(MsgType)
};

using MessageEventType = RoomMessageEvent::MsgType;

namespace EventContent {

    // Additional event content types

    /**
     * Rich text content for m.text, m.emote, m.notice
     *
     * Available fields: mimeType, body. The body can be either rich text
     * or plain text, depending on what mimeType specifies.
     */
    class QUOTIENT_API TextContent : public TypedBase {
    public:
        TextContent(QString text, const QString& contentType);
        explicit TextContent(const QJsonObject& json);

        QMimeType type() const override { return mimeType; }

        QMimeType mimeType;
        QString body;

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
            , duration(FileInfo::originalInfoJson["duration"_L1].toInt())
        {}

    protected:
        void fillInfoJson(QJsonObject& infoJson) const override
        {
            infoJson.insert("duration"_L1, duration);
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
