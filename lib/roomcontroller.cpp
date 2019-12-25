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

#include "roomcontroller.h"

#include "room.h"
#include "filetransfer.h"

#include "csapi/account-data.h"
#include "csapi/banning.h"
#include "csapi/inviting.h"
#include "csapi/kicking.h"
#include "csapi/leaving.h"
#include "csapi/redaction.h"
#include "csapi/room_send.h"
#include "csapi/room_state.h"
#include "csapi/room_upgrades.h"
#include "csapi/tags.h"
#include "jobs/downloadfilejob.h"

#include "events/reactionevent.h"
#include "events/simplestateevents.h"
#include "events/callanswerevent.h"
#include "events/callcandidatesevent.h"
#include "events/callhangupevent.h"
#include "events/callinviteevent.h"

#include <QtCore/QFileInfo>
#include <QtCore/QPointer>
#include <QtCore/QStringBuilder>

using namespace Quotient;

class FileTransfers {
    using FileTransferPtr = FileTransfer*;
    /// A map from room and event/txn ids to information about the long
    /// operation; used for both download (with event id) and upload (txn id)
    /// operations
    std::unordered_map<Room*, std::unordered_map<QString, FileTransferPtr>>
        transfersMap;
    // FIXME: the file transfers map is ever-growing (same as Room's timeline)
    // The only case when a file transfer is deleted is its cancellation
public:
    void add(Room* r, const QString& tid, FileTransfer* ft)
    {
        // If there was a previous transfer (completed or failed), overwrite it.
        transfersMap[r].insert_or_assign(tid, ft);
        QObject::connect(ft, &QObject::destroyed, ft, [this, r, tid, ft] {
            erase(r, tid, ft);
        });
    }
    FileTransfer* find(Room* r, const QString& tid)
    {
        if (auto roomTransfersIt = transfersMap.find(r);
            roomTransfersIt != transfersMap.cend()) {
            if (auto transferIt = roomTransfersIt->second.find(tid);
                transferIt != roomTransfersIt->second.cend())
                return transferIt->second;
        }
        return nullptr;
    }
    void replaceId(Room* r, const RoomEvent& evt)
    {
        if (auto scopeIt = transfersMap.find(r); scopeIt != transfersMap.end()) {
            auto& submap = scopeIt->second;
            if (submap.find(evt.id()) != submap.end()) {
                qWarning(MAIN)
                    << "Cannot move file transfer from txnId"
                    << evt.transactionId() << "to event id" << evt.id()
                    << "as there's another file transfer with that id";
                qWarning(MAIN) << "Both file transfers remain intact";
                return;
            }
            auto node = submap.extract(evt.transactionId());
            if (!node.empty()) {
                node.key() = evt.id();
                qCDebug(MAIN) << "File transfer id:" << evt.transactionId()
                              << "->" << evt.id();

                bool inserted = !submap.insert(std::move(node)).inserted;
                Q_ASSERT(inserted);
            }
        }
    }

    bool erase(Room* r, const QString& tid, FileTransfer* ft)
    {
        bool deleted = false;
        auto& submap = transfersMap[r];
        if (auto it = submap.find(tid); it != submap.end()) {
            if (it->second == ft) {
                submap.erase(tid);
                deleted = true;
            } // else it's some other ft already and shouldn't be touched
        } else
            qWarning(MAIN) << "FileTransfer" << tid
                           << "has already been deleted";
        if (submap.empty())
            transfersMap.erase(r);
        return deleted;
    }
};

class RoomController::Private {
public:
    RoomController* q = nullptr;
    Room* currentRoom = nullptr;

    static FileTransfers& fileTransfers()
    {
        static FileTransfers _;
        return _;
    }

    template <typename JobT, typename... ArgTs>
    JobT* callRoomApi(ArgTs&&... args) const
    {
        return currentRoom->connection()->callApi<JobT>(
            currentRoom->id(), std::forward<ArgTs>(args)...);
    }

    template <typename JobT, typename... ArgTs>
    JobT* callUserApi(ArgTs&&... args) const
    {
        auto* c = currentRoom->connection();
        return c->callApi<JobT>(c->userId(), std::forward<ArgTs>(args)...);
    }

    SetRoomStateWithKeyJob* requestSetState(const StateEventBase& event) const
    {
        //            if (event.roomId().isEmpty())
        //                event.setRoomId(id);
        //            if (event.senderId().isEmpty())
        //                event.setSender(connection->userId());
        // TODO: Queue up state events sending (see #133).
        // TODO: Maybe addAsPending() as well, despite having no txnId
        return callRoomApi<SetRoomStateWithKeyJob>(event.matrixType(),
                                                   event.stateKey(),
                                                   event.contentJson());
    }

    template <typename EvT, typename... ArgTs>
    auto requestSetState(ArgTs&&... args) const
    {
        return requestSetState(EvT(std::forward<ArgTs>(args)...));
    }

    QString sendEvent(RoomEventPtr&& event);

    template <typename EventT, typename... ArgTs>
    QString sendEvent(ArgTs&&... eventArgs)
    {
        return sendEvent(makeEvent<EventT>(std::forward<ArgTs>(eventArgs)...));
    }

    SendMessageJob* doSendEvent(const RoomEvent &event);
    void onEventSendingFailure(const QString& txnId, BaseJob* call = nullptr);

    FileTransfer* addFileTransfer(const QString& tid, FileTransfer* ft)
    {
        fileTransfers().add(currentRoom, tid, ft);
        emit q->newFileTransfer(tid, ft);
        return ft;
    }
};

RoomController::RoomController(QObject* parent)
    : QObject(parent), d(new Private)
{
    // All RoomController initialisation should occur before this point
    d->q = this;
    // RoomController::Private post-init operations that need q come after here
}

RoomController::RoomController(Room* initialRoom, QObject* parent)
    : RoomController(parent)
{
    setRoom(initialRoom);
}

RoomController::~RoomController() = default;

Room* RoomController::room() const { return d->currentRoom; }

Connection* RoomController::connection() const { return room()->connection(); }

void RoomController::setRoom(Room* r)
{
    if (d->currentRoom == r)
        return;

    d->currentRoom = r;
    emit roomChanged();
}

FileTransfer* RoomController::fileTransfer(const QString& id) const
{
    // TODO: Add lib tests to make sure FileTransferInfo::status stays
    // consistent with FileTransferInfo::job
    return d->fileTransfers().find(room(), id);
}

QUrl RoomController::fileSource(const QString& id) const
{
    if (auto url = room()->urlToDownload(id); url.isValid())
        return url;

    // No urlToDownload means it's a pending or completed upload.
    if (auto* transfer = fileTransfer(id))
        return transfer->localPath();

    qCWarning(MAIN) << "File source for identifier" << id << "not found";
    return {};
}

void RoomController::addTag(const QString& name, const TagRecord& record)
{
    // FIXME: allow propagation of tags in the same way setTags() does
    auto tags = room()->tags();
    const auto& checkRes = room()->validatedTag(name);
    if (tags.contains(name)
        || (checkRes.first && tags.contains(checkRes.second)))
        return;

    room()->updateTags([newName = checkRes.second, record](TagsMap& tags) {
        tags.insert(newName, record);
    });
    d->callUserApi<SetRoomTagJob>(room()->id(), checkRes.second, record.order);
}

void RoomController::addTag(const QString& name, float order)
{
    addTag(name, TagRecord { order });
}

void RoomController::removeTag(const QString& name)
{
    // FIXME: allow propagation of tags in the same way setTags() does
    auto tags = room()->tags();
    if (tags.contains(name)) {
        d->callUserApi<DeleteRoomTagJob>(room()->id(), name);
        room()->updateTags([name](TagsMap& tags) { tags.remove(name); });
    } else if (!name.startsWith("u."))
        removeTag("u." + name);
    else
        qCWarning(MAIN) << "Tag" << name << "on room" << room()->objectName()
                        << "not found, nothing to remove";
}

void RoomController::setTags(TagsMap newTags, ActionScope applyOn)
{
    bool propagate = applyOn != ActionScope::ThisRoomOnly;
    auto joinStates = applyOn == ActionScope::WithinSameState
                          ? room()->joinState()
                          : applyOn == ActionScope::OmitLeftState
                                ? JoinState::Join | JoinState::Invite
                                : JoinState::Join | JoinState::Invite
                                      | JoinState::Leave;
    if (propagate) {
        for (auto* r = room(); (r = r->predecessor(joinStates));)
            RoomController(r).setTags(newTags, ActionScope::ThisRoomOnly);
    }

    room()->updateTags([newTags = std::move(newTags)](TagsMap& tags) {
        tags = std::move(newTags);
    });

    d->callUserApi<SetAccountDataPerRoomJob>(room()->id(),
                                             TagEvent::matrixTypeId(),
                                             TagEvent(newTags).contentJson());

    if (propagate) {
        for (auto* r = room(); (r = r->successor(joinStates));)
            RoomController(r).setTags(newTags, ActionScope::ThisRoomOnly);
    }
}

QString RoomController::Private::sendEvent(RoomEventPtr&& event)
{
    if (currentRoom->usesEncryption()) {
        qCCritical(MAIN) << "Room" << currentRoom
                         << "enforces encryption; sending encrypted messages "
                            "is not supported yet";
    }
    if (currentRoom->successorId().isEmpty()) {
        auto pEvt = currentRoom->addAsPending(std::move(event));
        doSendEvent(*pEvt);
        return pEvt->transactionId();
    }

    // FIXME, #276: Strictly speaking, this should be governed by power levels
    qCWarning(MAIN) << currentRoom << "has been upgraded, events won't be sent";
    return {};
}

SendMessageJob* RoomController::Private::doSendEvent(const RoomEvent& event)
{
    const auto txnId = event.transactionId();
    Q_ASSERT(!txnId.isEmpty());
    // TODO, #133: Enqueue the job rather than immediately trigger it.
    auto job = currentRoom->connection()->callApi<SendMessageJob>(
        BackgroundRequest, // FIXME: it's not really a background request,
                           // it's just a matter of error handling visuals
        currentRoom->id(), event.matrixType(), txnId, event.contentJson());
    if (job) {
        QObject::connect(job, &BaseJob::sentRequest, q, [this, txnId] {
            auto it = currentRoom->findPendingEvent(txnId);
            if (it == currentRoom->pendingEvents().end()) {
                qCWarning(EVENTS) << "Pending event for transaction" << txnId
                                  << "not found - got synced so soon?";
                return;
            }
            qCDebug(EVENTS) << "Event txn" << txnId << "has departed";
            currentRoom->updatePendingEvent(it,
                                            [](auto& pe) { pe.setDeparted(); });
        });
        QObject::connect(job, &BaseJob::failure, q,
                         std::bind(&Private::onEventSendingFailure, this, txnId,
                                   job));
        QObject::connect(job, &BaseJob::success, q, [this, job, txnId] {
            emit q->messageSent(txnId, job->eventId());
            // TODO: RoomController::Private::sendNextPending()
            auto it = currentRoom->findPendingEvent(txnId);
            if (it == currentRoom->pendingEvents().end()) {
                qCDebug(MESSAGES) << "Pending event for transaction" << txnId
                                  << "already merged";
                return;
            }

            if (it->deliveryStatus() != EventStatus::ReachedServer) {
                currentRoom->updatePendingEvent(it, [job](auto& pe) {
                    pe.setReachedServer(job->eventId());
                });
            }
        });
    } else
        onEventSendingFailure(txnId);
    return job;
}

void RoomController::Private::onEventSendingFailure(const QString& txnId,
                                                    BaseJob* call)
{
    auto it = currentRoom->findPendingEvent(txnId);
    if (it == currentRoom->pendingEvents().end()) {
        qCritical(EVENTS) << "Pending event for transaction" << txnId
                          << "could not be sent";
        return;
    }
    currentRoom->updatePendingEvent(it, [call](auto& pe) {
        pe.setSendingFailed(call ? call->statusCaption() % ": "
                                       % call->errorString()
                                 : tr("The call could not be started"));
    });
}

QString RoomController::retryMessage(const QString& txnId)
{
    const auto it = room()->findPendingEvent(txnId);
    Q_ASSERT(it != room()->pendingEvents().end());
    qCDebug(EVENTS) << "Retrying transaction" << txnId;
    if (auto* transfer = fileTransfer(txnId)) {
        Q_ASSERT(transfer->isUpload());
        if (transfer->status() == FileTransfer::Completed) {
            qCDebug(MESSAGES)
                << "File for transaction" << txnId
                << "has already been uploaded, bypassing re-upload";
        } else {
            if (transfer->status() == FileTransfer::Started) {
                qCDebug(MESSAGES) << "Abandoning the upload job for transaction"
                                  << txnId << "and starting again";
                transfer->cancel();
            }
            // FIXME: Content type is no more passed here but it should
            uploadFile(txnId, transfer->localPath());
        }
    }
    if (it->deliveryStatus() == EventStatus::ReachedServer) {
        qCWarning(MAIN)
            << "The previous attempt has reached the server; two"
               " events are likely to be in the timeline after retry";
    }
    room()->updatePendingEvent(it, std::mem_fn(&PendingEventItem::resetStatus));
    d->doSendEvent(*it->event());
    return (*it)->transactionId();
}

void RoomController::discardMessage(const QString& txnId)
{
    qCDebug(EVENTS) << "Discarding transaction" << txnId;
    if (auto* transfer = fileTransfer(txnId)) {
        Q_ASSERT(transfer->isUpload());
        // TODO: Move out to FileTransfer::cancel()
        if (transfer->status() == FileTransfer::Started)
            transfer->cancel();
        else if (transfer->status() == FileTransfer::Completed) {
            qCWarning(MAIN)
                << "File for transaction" << txnId
                << "has been uploaded but the message was discarded";
        } // else?
    }
    bool erased = room()->erasePendingEvent(txnId);
    Q_ASSERT_X(erased, __FUNCTION__, "Couldn't find pending event to erase");
}

QString RoomController::postMessage(const QString& plainText,
                                    MessageEventType type)
{
    return d->sendEvent<RoomMessageEvent>(plainText, type);
}

QString RoomController::postPlainText(const QString& plainText)
{
    return postMessage(plainText, MessageEventType::Text);
}

QString RoomController::postHtmlMessage(const QString& plainText,
                                        const QString& html,
                                        MessageEventType type)
{
    return d->sendEvent<RoomMessageEvent>(
        plainText, type,
        new EventContent::TextContent(html, QStringLiteral("text/html")));
}

QString RoomController::postHtmlText(const QString& plainText,
                                     const QString& html)
{
    return postHtmlMessage(plainText, html);
}

QString RoomController::postReaction(const QString& eventId, const QString& key)
{
    return d->sendEvent<ReactionEvent>(EventRelation::annotate(eventId, key));
}

QString RoomController::postFile(const QString& plainText,
                                 const QUrl& localPath, bool asGenericFile)
{
    QFileInfo localFile { localPath.toLocalFile() };
    Q_ASSERT(localFile.isFile());

    const auto txnId = room()
            ->addAsPending(makeEvent<RoomMessageEvent>(plainText, localFile,
                                                       asGenericFile))
            ->transactionId();
    // Remote URL will only be known after upload; fill in the local path
    // to enable the preview while the event is pending.
    auto* transfer = uploadFile(txnId, localPath);
    connect(room(), &Room::pendingEventAboutToMerge, transfer,
            [this](const RoomEvent* nextPendingEvt) {
                d->fileTransfers().replaceId(room(), *nextPendingEvt);
            });
    // Below, the upload job is used as a context object to clean up connections
    connect(transfer, &FileTransfer::completed, transfer->job(),
            [this, txnId](QUrl, const QUrl& mxcUri) {
                if (auto it = room()->findPendingEvent(txnId);
                        it != room()->pendingEvents().end()) {
                    room()->updatePendingEvent(it, [mxcUri](auto& pe) {
                        pe.setFileUploaded(mxcUri);
                    });
                    d->doSendEvent(*it->event());
                } else {
                    // Normally in this situation we should instruct
                    // the media server to delete the file; alas, there's no
                    // API specced for that.
                    qCWarning(MAIN) << "File uploaded to" << mxcUri
                                    << "but the event referring to it was "
                                       "cancelled";
                }
            });
    connect(transfer, &FileTransfer::cancelled, transfer->job(),
            [this, txnId] { room()->erasePendingEvent(txnId); });
    return txnId;
}

QString RoomController::postEvent(RoomEvent* event)
{
    return d->sendEvent(RoomEventPtr(event));
}

QString RoomController::postJson(const QString& matrixType,
                       const QJsonObject& eventContent)
{
    return d->sendEvent(loadEvent<RoomEvent>(matrixType, eventContent));
}

SetRoomStateWithKeyJob* RoomController::setState(const StateEventBase& evt) const
{
    return d->requestSetState(evt);
}

void RoomController::setName(const QString& newName)
{
    d->requestSetState<RoomNameEvent>(newName);
}

void RoomController::setCanonicalAlias(const QString& newAlias)
{
    d->requestSetState<RoomCanonicalAliasEvent>(newAlias);
}

void RoomController::setLocalAliases(const QStringList& aliases)
{
    d->requestSetState<RoomAliasesEvent>(connection()->domain(),
                                         aliases);
}

void RoomController::setTopic(const QString& newTopic)
{
    d->requestSetState<RoomTopicEvent>(newTopic);
}

void RoomController::inviteToRoom(const QString& memberId)
{
    d->callRoomApi<InviteUserJob>(memberId);
}

LeaveRoomJob* RoomController::leaveRoom()
{
    // FIXME, #63: It should be RoomManager, not Connection
    return connection()->leaveRoom(room());
}

void RoomController::kickMember(const QString& memberId, const QString& reason)
{
    d->callRoomApi<KickJob>(memberId, reason);
}

void RoomController::ban(const QString& userId, const QString& reason)
{
    d->callRoomApi<BanJob>(userId, reason);
}

void RoomController::unban(const QString& userId)
{
    d->callRoomApi<UnbanJob>(userId);
}

void RoomController::redactEvent(const QString& eventId, const QString& reason)
{
    d->callRoomApi<RedactEventJob>(QUrl::toPercentEncoding(eventId),
                                   connection()->generateTxnId(), reason);
}

FileTransfer* RoomController::uploadFile(const QString& id,
                                         const QUrl& localFilename,
                                         const QString& overrideContentType)
{
    Q_ASSERT_X(localFilename.isLocalFile(), __FUNCTION__,
               "localFilename should point at a local file");
    auto fileName = localFilename.toLocalFile();
    auto* job = connection()->uploadFile(fileName, overrideContentType);
    return d->addFileTransfer(id, new FileTransfer(job, fileName));
}

FileTransfer* RoomController::downloadFile(const QString& eventId,
                                           const QUrl& localFilename)
{
    if (auto ongoingTransfer = fileTransfer(eventId); ongoingTransfer
        && ongoingTransfer->status() == FileTransfer::Started) {
        qCWarning(MAIN) << "Transfer for" << eventId << "is already ongoing";
        return ongoingTransfer; // It may be an upload too
    }

    Q_ASSERT(localFilename.isEmpty() || localFilename.isLocalFile());
    const auto* event = room()->getFileEvent(eventId);
    if (!event) {
        qCCritical(MAIN)
            << eventId << "is not in the local timeline or has no file content";
        Q_ASSERT(false);
        return nullptr;
    }
    const auto* const fileInfo = event->content()->fileInfo();
    if (!fileInfo->isValid()) {
        qCWarning(MAIN) << "Event" << eventId
                        << "has an empty or malformed mxc URL; won't download";
        return nullptr;
    }
    const auto fileUrl = fileInfo->url;
    auto filePath = localFilename.toLocalFile();
    if (filePath.isEmpty()) { // Setup default file path
        filePath =
            fileInfo->url.path().mid(1) % '_' % event->getFileName();

        if (filePath.size() > 200) // If too long, elide in the middle
            filePath.replace(128, filePath.size() - 192, "---");

        filePath = QDir::tempPath() % '/' % filePath;
        qDebug(MAIN) << "File path:" << filePath;
    }
    auto* job = connection()->downloadFile(fileUrl, filePath);
    return d->addFileTransfer(eventId,
                              new FileTransfer(job, job->targetFileName()));
}

void RoomController::switchVersion(QString newVersion)
{
    if (!room()->successorId().isEmpty()) {
        emit upgradeFailed(tr("The room is already upgraded"));
        return;
    }

    if (auto* job = d->callRoomApi<UpgradeRoomJob>(newVersion))
        connect(job, &BaseJob::failure, this,
                [this, job] { emit upgradeFailed(job->errorString()); });
    else {
        Q_ASSERT(false);
        emit upgradeFailed(tr("Couldn't initiate upgrade"));
    }
}

void RoomController::inviteCall(const QString& callId, const int lifetime,
                      const QString& sdp)
{
    Q_ASSERT(room()->supportsCalls());
    d->sendEvent<CallInviteEvent>(callId, lifetime, sdp);
}

void RoomController::sendCallCandidates(const QString& callId,
                              const QJsonArray& candidates)
{
    Q_ASSERT(room()->supportsCalls());
    d->sendEvent<CallCandidatesEvent>(callId, candidates);
}

void RoomController::answerCall(const QString& callId, const int lifetime,
                      const QString& sdp)
{
    Q_ASSERT(room()->supportsCalls());
    d->sendEvent<CallAnswerEvent>(callId, lifetime, sdp);
}

void RoomController::answerCall(const QString& callId, const QString& sdp)
{
    Q_ASSERT(room()->supportsCalls());
    d->sendEvent<CallAnswerEvent>(callId, sdp);
}

void RoomController::hangupCall(const QString& callId)
{
    Q_ASSERT(room()->supportsCalls());
    d->sendEvent<CallHangupEvent>(callId);
}
