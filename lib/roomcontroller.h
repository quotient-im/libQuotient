/******************************************************************************
 * Copyright (C) 2019 The Quotient project
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

#pragma once

#include "eventitem.h"

#include "events/roommessageevent.h"
#include "events/accountdataevents.h"

namespace Quotient {
class Connection;
class Room;
class FileTransfer;
class SetRoomStateWithKeyJob;
class LeaveRoomJob;

/*! TODO
 */
class RoomController : public QObject {
    Q_OBJECT
    Q_PROPERTY(Room* room READ room WRITE setRoom NOTIFY roomChanged)

public:
    explicit RoomController(QObject* parent = nullptr);
    explicit RoomController(Room* initialRoom, QObject* parent = nullptr);
    ~RoomController() override;

    /// Get the room used as a context for all operations by this controller
    Room* room() const;

    /// Get the connection of the current room (=== room()->connection())
    Connection* connection() const;

    /// Get information on file upload/download
    /*!
     * \param id uploads are identified by the corresponding event's
     *           transactionId (because uploads are done before
     *           the event is even sent), while downloads are using
     *           the normal event id for identifier.
     */
    Q_INVOKABLE FileTransfer* fileTransfer(const QString& id) const;

    /// Get the URL to the actual file source in a unified way
    /*!
     * For uploads it will return a URL to a local file; for downloads
     * the URL will be taken from the corresponding room event.
     */
    Q_INVOKABLE QUrl fileSource(const QString& id) const;

    /// Set a state event of the given type with the given arguments
    /*! This typesafe overload attempts to send a state event with the type
     * \p EvT and the content defined by \p args. Specifically, the function
     * creates a temporary object of type \p EvT passing \p args to
     * the constructor, and sends a request to the homeserver using
     * the Matrix event type defined by \p EvT and the event content produced
     * via EvT::contentJson().
     */
    template <typename EvT, typename... ArgTs>
    auto setState(ArgTs&&... args) const
    {
        return setState(EvT(std::forward<ArgTs>(args)...));
    }

    /// Send a request to update the room state with the given event
    SetRoomStateWithKeyJob* setState(const StateEventBase& evt) const;

    /// The scope to apply an action on
    /*! This enumeration is used to pick a strategy to propagate certain
     * actions on the room to its predecessors and successors.
     */
    enum ActionScope {
        ThisRoomOnly, //< Do not apply to predecessors and successors
        WithinSameState, //< Apply to predecessors and successors in the same
                         //< state as the current one
        OmitLeftState, //< Apply to all reachable predecessors and successors
                       //< except those in Leave state
        WholeSequence //< Apply to all reachable predecessors and successors
    };

    /// Add a new tag to the current room
    /*! If this room already has this tag, nothing happens. If it's a new
     * tag for the room, the respective tag record is added to the set
     * of tags and the new set is sent to the server to update other
     * clients.
     */
    void addTag(const QString& name, const TagRecord& record = {});
    /// Add a new tag to the current room
    /*! This is a simplified addTag() overload that is callable from QML */
    Q_INVOKABLE void addTag(const QString& name, float order);

    /// Remove a tag from the room
    Q_INVOKABLE void removeTag(const QString& name);

    /** Overwrite the room's tags
     * This completely replaces the existing room's tags with a set
     * of new ones and updates the new set on the server. Unlike
     * most other methods in Room, this one sends a signal about changes
     * immediately, not waiting for confirmation from the server
     * (because tags are saved in account data rather than in shared
     * room state).
     * \param applyOn setting this to Room::OnAllConversations will set tags
     *                on this and all _known_ predecessors and successors;
     *                by default only the current room is changed
     */
    void setTags(TagsMap newTags, ActionScope applyOn = ThisRoomOnly);

public slots:
    void setRoom(Room* r);
    QString postMessage(const QString& plainText, MessageEventType type);
    QString postPlainText(const QString& plainText);
    QString postHtmlMessage(const QString& plainText, const QString& html,
                            MessageEventType type = MessageEventType::Text);
    QString postHtmlText(const QString& plainText, const QString& html);
    /// Send a reaction on a given event with a given key
    QString postReaction(const QString& eventId, const QString& key);
    QString postFile(const QString& plainText, const QUrl& localPath,
                     bool asGenericFile = false);
    /** Post a pre-created room message event
     *
     * Takes ownership of the event, deleting it once the matching one
     * arrives with the sync
     * \return transaction id associated with the event.
     */
    QString postEvent(RoomEvent* event);
    QString postJson(const QString& matrixType, const QJsonObject& eventContent);
    QString retryMessage(const QString& txnId);
    void discardMessage(const QString& txnId);

    void setName(const QString& newName);
    void setCanonicalAlias(const QString& newAlias);
    /// Set room aliases on the user's current server
    void setLocalAliases(const QStringList& aliases);
    void setTopic(const QString& newTopic);

    void inviteToRoom(const QString& memberId);
    LeaveRoomJob* leaveRoom();
    void kickMember(const QString& memberId, const QString& reason = {});
    void ban(const QString& userId, const QString& reason = {});
    void unban(const QString& userId);

    void redactEvent(const QString& eventId, const QString& reason = {});

    FileTransfer* uploadFile(const QString& id, const QUrl& localFilename,
                             const QString& overrideContentType = {});
    // If localFilename is empty a temporary file is created
    FileTransfer* downloadFile(const QString& eventId,
                               const QUrl& localFilename = {});

    /// Switch the room's version (aka upgrade)
    void switchVersion(QString newVersion);

    void inviteCall(const QString& callId, const int lifetime,
                    const QString& sdp);
    void sendCallCandidates(const QString& callId, const QJsonArray& candidates);
    void answerCall(const QString& callId, const int lifetime,
                    const QString& sdp);
    void answerCall(const QString& callId, const QString& sdp);
    void hangupCall(const QString& callId);

signals:
    /// The context for operations has changed
    void roomChanged();
    /// The server accepted the message
    /** This is emitted when an event sending request has successfully
     * completed. This does not mean that the event is already in the
     * local timeline, only that the server has accepted it.
     * \param txnId transaction id assigned by the client during sending
     * \param eventId event id assigned by the server upon acceptance
     * \sa postEvent, postPlainText, postMessage, postHtmlMessage
     * \sa Room::pendingEventMerged, Room::aboutToAddNewMessages
     */
    void messageSent(QString txnId, QString eventId);

    void newFileTransfer(QString id, FileTransfer* ft);

    /// An attempted room upgrade has failed
    void upgradeFailed(QString errorMessage);

private:
    class Private;
    Private* d;
};

}
