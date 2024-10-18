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
                     std::unique_ptr<EventContent::Base> content = nullptr,
                     const std::optional<EventRelation>& relatesTo = std::nullopt);
    explicit RoomMessageEvent(const QString& plainBody, MsgType msgType = MsgType::Text,
                              std::unique_ptr<EventContent::Base> content = nullptr,
                              const std::optional<EventRelation>& relatesTo = std::nullopt);

    explicit RoomMessageEvent(const QJsonObject& obj);

    MsgType msgtype() const;
    QString rawMsgtype() const;
    QString plainBody() const;

    //! \brief Load event content from the event JSON
    //!
    //! \warning The result must be checked for nullptr as an event with just a plainBody
    //!          will not have a content object.
    //! \warning Since libQuotient 0.9, the returned value has changed from a C pointer (TypedBase*)
    //!          to `std::unique_ptr<>` because the deserialised content object is no more stored
    //!          inside the event. The calling code must either store the entire returned value
    //!          in a variable or copy/move away the needed field from the returned value;
    //!          a reference or a pointer to a field will become dangling at the statement end.
    //!
    //! \return an event content object if the event has content, nullptr otherwise.
    std::unique_ptr<EventContent::Base> content() const;

    //! Update the message JSON with the given content
    void setContent(std::unique_ptr<EventContent::Base> content);

    //! \brief Determine whether the message has content/attachment of a specified type
    //!
    //! \return true, if the message has type and content corresponding to \p ContentT (e.g.
    //!         `m.file` or `m.audio` for FileContent); false otherwise
    template <std::derived_from<EventContent::Base> ContentT>
    bool has() const { return false; }

    //! \brief Get the message content and try to cast it to the specified type
    //!
    //! \return A pointer to the object of the requested type if the event has content of this type;
    //!         nullptr, if the event has no content or the content cannot be cast to this type
    template <std::derived_from<EventContent::Base> ContentT>
    std::unique_ptr<ContentT> get() const
    {
        return has<ContentT>()
                   ? std::unique_ptr<ContentT>(static_cast<ContentT*>(content().release()))
                   : nullptr;
    }

    QMimeType mimeType() const;

    //! \brief Determine whether the message has a thumbnail
    //!
    //! \return true, if the message has a data structure corresponding to
    //!         a thumbnail (the message type may be one for visual content,
    //!         such as m.image, or non-visual, i.e. m.file or m.location);
    //!         false otherwise
    bool hasThumbnail() const;

    //! Retrieve a thumbnail from the message event
    EventContent::Thumbnail getThumbnail() const;

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
    // FIXME: should it really be static?
    static QJsonObject assembleContentJson(const QString& plainBody, const QString& jsonMsgType,
                                           std::unique_ptr<EventContent::Base> content,
                                           const std::optional<EventRelation>& relatesTo);

    Q_ENUM(MsgType)
};

template <> QUOTIENT_API bool RoomMessageEvent::has<EventContent::TextContent>() const;
template <> QUOTIENT_API bool RoomMessageEvent::has<EventContent::LocationContent>() const;
template <> QUOTIENT_API bool RoomMessageEvent::has<EventContent::FileContentBase>() const;
template <> QUOTIENT_API bool RoomMessageEvent::has<EventContent::FileContent>() const;
template <> QUOTIENT_API bool RoomMessageEvent::has<EventContent::ImageContent>() const;
template <> QUOTIENT_API bool RoomMessageEvent::has<EventContent::AudioContent>() const;
template <> QUOTIENT_API bool RoomMessageEvent::has<EventContent::VideoContent>() const;

using MessageEventType = RoomMessageEvent::MsgType;
} // namespace Quotient
