
#include "connection.h"
#include "room.h"
#include "user.h"
#include "csapi/room_send.h"
#include "csapi/joining.h"
#include "csapi/leaving.h"
#include "events/simplestateevents.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QStringBuilder>
#include <QtCore/QTimer>
#include <QtCore/QTemporaryFile>
#include <QtCore/QFileInfo>
#include <iostream>
#include <functional>

using namespace QMatrixClient;
using std::cout;
using std::endl;
using namespace std::placeholders;

class QMCTest : public QObject
{
    public:
        QMCTest(Connection* conn, QString testRoomName, QString source);

    private slots:
        void setupAndRun();
        void onNewRoom(Room* r);
        void run();
        void doTests();
            void loadMembers();
            void sendMessage();
            void sendFile();
                void checkFileSendingOutcome(const QString& txnId,
                                             const QString& fileName);
            void setTopic();
            void addAndRemoveTag();
            void sendAndRedact();
                bool checkRedactionOutcome(const QString& evtIdToRedact);
            void markDirectChat();
                void checkDirectChatOutcome(
                        const Connection::DirectChatsMap& added);
        void conclude();
        void finalize();

    private:
        QScopedPointer<Connection, QScopedPointerDeleteLater> c;
        QStringList running;
        QStringList succeeded;
        QStringList failed;
        QString origin;
        QString targetRoomName;
        Room* targetRoom = nullptr;

        bool validatePendingEvent(const QString& txnId);
};

#define QMC_CHECK(description, condition) \
{ \
    Q_ASSERT(running.removeOne(description)); \
    if (!!(condition)) \
    { \
        succeeded.push_back(description); \
        cout << (description) << " successful" << endl; \
        if (targetRoom) \
            targetRoom->postMessage( \
                origin % ": " % (description) % " successful", \
                MessageEventType::Notice); \
    } else { \
        failed.push_back(description); \
        cout << (description) << " FAILED" << endl; \
        if (targetRoom) \
            targetRoom->postPlainText( \
                origin % ": " % (description) % " FAILED"); \
    } \
}

bool QMCTest::validatePendingEvent(const QString& txnId)
{
    auto it = targetRoom->findPendingEvent(txnId);
    return it != targetRoom->pendingEvents().end() &&
            it->deliveryStatus() == EventStatus::Submitted &&
            (*it)->transactionId() == txnId;
}

QMCTest::QMCTest(Connection* conn, QString testRoomName, QString source)
    : c(conn), origin(std::move(source)), targetRoomName(std::move(testRoomName))
{
    if (!origin.isEmpty())
        cout << "Origin for the test message: " << origin.toStdString() << endl;
    if (!targetRoomName.isEmpty())
        cout << "Test room name: " << targetRoomName.toStdString() << endl;

    connect(c.data(), &Connection::connected, this, &QMCTest::setupAndRun);
    connect(c.data(), &Connection::loadedRoomState, this, &QMCTest::onNewRoom);
    // Big countdown watchdog
    QTimer::singleShot(180000, this, &QMCTest::conclude);
}

void QMCTest::setupAndRun()
{
    cout << "Connected, server: "
         << c->homeserver().toDisplayString().toStdString() << endl;
    cout << "Access token: " << c->accessToken().toStdString() << endl;

    if (!targetRoomName.isEmpty())
    {
        cout << "Joining " << targetRoomName.toStdString() << endl;
        running.push_back("Join room");
        auto joinJob = c->joinRoom(targetRoomName);
        connect(joinJob, &BaseJob::failure, this,
            [this] { QMC_CHECK("Join room", false); conclude(); });
        // Connection::joinRoom() creates a Room object upon JoinRoomJob::success
        // but this object is empty until the first sync is done.
        connect(joinJob, &BaseJob::success, this, [this,joinJob] {
            targetRoom = c->room(joinJob->roomId(), JoinState::Join);
            QMC_CHECK("Join room", targetRoom != nullptr);

            run();
        });
    } else
        run();
}

void QMCTest::onNewRoom(Room* r)
{
    cout << "New room: " << r->id().toStdString() << endl
         << "  Name: " << r->name().toStdString() << endl
         << "  Canonical alias: " << r->canonicalAlias().toStdString() << endl
         << endl;
    connect(r, &Room::aboutToAddNewMessages, r, [r] (RoomEventsRange timeline) {
        cout << timeline.size() << " new event(s) in room "
             << r->canonicalAlias().toStdString() << endl;
//        for (const auto& item: timeline)
//        {
//            cout << "From: "
//                 << r->roomMembername(item->senderId()).toStdString()
//                 << endl << "Timestamp:"
//                 << item->timestamp().toString().toStdString() << endl
//                 << "JSON:" << endl << item->originalJson().toStdString() << endl;
//        }
    });
}

void QMCTest::run()
{
    c->setLazyLoading(true);
    c->sync();
    connectSingleShot(c.data(), &Connection::syncDone, this, &QMCTest::doTests);
    connect(c.data(), &Connection::syncDone, c.data(), [this] {
        cout << "Sync complete, " << running.size() << " test(s) in the air: "
             << running.join(", ").toStdString() << endl;
        if (!running.isEmpty())
            c->sync(10000);
        else
            conclude();
    });
}

void QMCTest::doTests()
{
    cout << "Starting tests" << endl;

    loadMembers();
    // Add here tests not requiring the test room
    if (targetRoomName.isEmpty())
        return;

    sendMessage();
    sendFile();
    setTopic();
    addAndRemoveTag();
    sendAndRedact();
    markDirectChat();
    // Add here tests with the test room
}

void QMCTest::loadMembers()
{
    running.push_back("Loading members");
    // The dedicated qmc-test room is too small to test
    // lazy-loading-then-full-loading; use #qmatrixclient:matrix.org instead.
    // TODO: #264
    auto* r = c->room(QStringLiteral("!PCzUtxtOjUySxSelof:matrix.org"));
    if (!r)
    {
        cout << "#test:matrix.org is not found in the test user's rooms" << endl;
        QMC_CHECK("Loading members", false);
        return;
    }
    // It's not exactly correct because an arbitrary server might not support
    // lazy loading; but in the absence of capabilities framework we assume
    // it does.
    if (r->memberNames().size() >= r->joinedCount())
    {
        cout << "Lazy loading doesn't seem to be enabled" << endl;
        QMC_CHECK("Loading members", false);
        return;
    }
    r->setDisplayed();
    connect(r, &Room::allMembersLoaded, [this,r] {
        QMC_CHECK("Loading members",
                  r->memberNames().size() >= r->joinedCount());
    });
}

void QMCTest::sendMessage()
{
    running.push_back("Message sending");
    cout << "Sending a message" << endl;
    auto txnId = targetRoom->postPlainText("Hello, " % origin % " is here");
    if (!validatePendingEvent(txnId))
    {
        cout << "Invalid pending event right after submitting" << endl;
        QMC_CHECK("Message sending", false);
        return;
    }

    connectUntil(targetRoom, &Room::pendingEventAboutToMerge, this,
        [this,txnId] (const RoomEvent* evt, int pendingIdx) {
            const auto& pendingEvents = targetRoom->pendingEvents();
            Q_ASSERT(pendingIdx >= 0 && pendingIdx < int(pendingEvents.size()));

            if (evt->transactionId() != txnId)
                return false;

            QMC_CHECK("Message sending",
                is<RoomMessageEvent>(*evt) && !evt->id().isEmpty() &&
                pendingEvents[size_t(pendingIdx)]->transactionId()
                    == evt->transactionId());
            return true;
        });
}

void QMCTest::sendFile()
{
    running.push_back("File sending");
    cout << "Sending a file" << endl;
    auto* tf = new QTemporaryFile;
    if (!tf->open())
    {
        cout << "Failed to create a temporary file" << endl;
        QMC_CHECK("File sending", false);
        return;
    }
    tf->write("Test");
    tf->close();
    // QFileInfo::fileName brings only the file name; QFile::fileName brings
    // the full path
    const auto tfName = QFileInfo(*tf).fileName();
    cout << "Sending file" << tfName.toStdString() << endl;
    const auto txnId = targetRoom->postFile("Test file",
                                            QUrl::fromLocalFile(tf->fileName()));
    if (!validatePendingEvent(txnId))
    {
        cout << "Invalid pending event right after submitting" << endl;
        QMC_CHECK("File sending", false);
        delete tf;
        return;
    }

    // FIXME: Clean away connections (connectUntil doesn't help here).
    connect(targetRoom, &Room::fileTransferCompleted, this,
        [this,txnId,tf,tfName] (const QString& id) {
            auto fti = targetRoom->fileTransferInfo(id);
            Q_ASSERT(fti.status == FileTransferInfo::Completed);

            if (id != txnId)
                return;

            delete tf;

            checkFileSendingOutcome(txnId, tfName);
        });
    connect(targetRoom, &Room::fileTransferFailed, this,
        [this,txnId,tf]
        (const QString& id, const QString& error) {
            if (id != txnId)
                return;

            targetRoom->postPlainText(origin % ": File upload failed: " % error);
            delete tf;

            QMC_CHECK("File sending", false);
        });
}

void QMCTest::checkFileSendingOutcome(const QString& txnId,
                                      const QString& fileName)
{
    auto it = targetRoom->findPendingEvent(txnId);
    if (it == targetRoom->pendingEvents().end())
    {
        cout << "Pending file event dropped before upload completion"
             << endl;
        QMC_CHECK("File sending", false);
        return;
    }
    if (it->deliveryStatus() != EventStatus::FileUploaded)
    {
        cout << "Pending file event status upon upload completion is "
             << it->deliveryStatus() << " != FileUploaded("
             << EventStatus::FileUploaded << ')' << endl;
        QMC_CHECK("File sending", false);
        return;
    }

    connectUntil(targetRoom, &Room::pendingEventAboutToMerge, this,
        [this,txnId,fileName] (const RoomEvent* evt, int pendingIdx) {
            const auto& pendingEvents = targetRoom->pendingEvents();
            Q_ASSERT(pendingIdx >= 0 && pendingIdx < int(pendingEvents.size()));

            if (evt->transactionId() != txnId)
                return false;

            cout << "File event " << txnId.toStdString()
                 << " arrived in the timeline" << endl;
            visit(*evt,
                [&] (const RoomMessageEvent& e) {
                    QMC_CHECK("File sending",
                        !e.id().isEmpty() &&
                        pendingEvents[size_t(pendingIdx)]
                            ->transactionId() == txnId &&
                        e.hasFileContent() &&
                        e.content()->fileInfo()->originalName == fileName);
                },
                [this] (const RoomEvent&) {
                    QMC_CHECK("File sending", false);
                });
            return true;
    });
}

void QMCTest::setTopic()
{
    static const char* const stateTestName = "State setting test";
    static const char* const fakeStateTestName = "Fake state event immunity test";
    running.push_back(stateTestName);
    running.push_back(fakeStateTestName);
    auto initialTopic = targetRoom->topic();

    const auto newTopic = c->generateTxnId();
    targetRoom->setTopic(newTopic); // Sets the state by proper means
    const auto fakeTopic = c->generateTxnId();
    targetRoom->postJson(RoomTopicEvent::matrixTypeId(), // Fake state event
                         RoomTopicEvent(fakeTopic).contentJson());

    connectUntil(targetRoom, &Room::topicChanged, this,
        [this,newTopic,fakeTopic,initialTopic] {
            if (targetRoom->topic() == newTopic)
            {
                QMC_CHECK(stateTestName, true);
                // Don't reset the topic yet if the negative test still runs
                if (!running.contains(fakeStateTestName))
                    targetRoom->setTopic(initialTopic);

                return true;
            }
            return false;
        });

    connectUntil(targetRoom, &Room::pendingEventAboutToMerge, this,
        [this,fakeTopic,initialTopic] (const RoomEvent* e, int) {
            if (e->contentJson().value("topic").toString() != fakeTopic)
                return false; // Wait on for the right event

            QMC_CHECK(fakeStateTestName, !e->isStateEvent());
            if (!running.contains(fakeStateTestName))
                targetRoom->setTopic(initialTopic);
            return true;
        });
}

void QMCTest::addAndRemoveTag()
{
    running.push_back("Tagging test");
    static const auto TestTag = QStringLiteral("org.qmatrixclient.test");
    // Pre-requisite
    if (targetRoom->tags().contains(TestTag))
        targetRoom->removeTag(TestTag);

    // Connect first because the signal is emitted synchronously.
    connect(targetRoom, &Room::tagsChanged, targetRoom, [=] {
        cout << "Room " << targetRoom->id().toStdString()
             << ", tag(s) changed:" << endl
             << "  " << targetRoom->tagNames().join(", ").toStdString() << endl;
        if (targetRoom->tags().contains(TestTag))
        {
            cout << "Test tag set, removing it now" << endl;
            targetRoom->removeTag(TestTag);
            QMC_CHECK("Tagging test", !targetRoom->tags().contains(TestTag));
            disconnect(targetRoom, &Room::tagsChanged, nullptr, nullptr);
        }
    });
    cout << "Adding a tag" << endl;
    targetRoom->addTag(TestTag);
}

void QMCTest::sendAndRedact()
{
    running.push_back("Redaction");
    cout << "Sending a message to redact" << endl;
    auto txnId = targetRoom->postPlainText(origin % ": message to redact");
    if (txnId.isEmpty())
    {
        QMC_CHECK("Redaction", false);
        return;
    }
    connect(targetRoom, &Room::messageSent, this,
        [this,txnId] (const QString& tId, const QString& evtId) {
            if (tId != txnId)
                return;

            cout << "Redacting the message" << endl;
            targetRoom->redactEvent(evtId, origin);

            connectUntil(targetRoom, &Room::addedMessages, this,
                [this,evtId] { return checkRedactionOutcome(evtId); });
        });
}

bool QMCTest::checkRedactionOutcome(const QString& evtIdToRedact)
{
    // There are two possible (correct) outcomes: either the event comes already
    // redacted at the next sync, or the nearest sync completes with
    // the unredacted event but the next one brings redaction.
    auto it = targetRoom->findInTimeline(evtIdToRedact);
    if (it == targetRoom->timelineEdge())
        return false; // Waiting for the next sync

    if ((*it)->isRedacted())
    {
        cout << "The sync brought already redacted message" << endl;
        QMC_CHECK("Redaction", true);
    } else {
        cout << "Message came non-redacted with the sync, waiting for redaction"
             << endl;
        connectUntil(targetRoom, &Room::replacedEvent, this,
            [this,evtIdToRedact]
            (const RoomEvent* newEvent, const RoomEvent* oldEvent) {
                if (oldEvent->id() != evtIdToRedact)
                    return false;

                QMC_CHECK("Redaction", newEvent->isRedacted() &&
                                       newEvent->redactionReason() == origin);
                return true;
            });
    }
    return true;
}

void QMCTest::markDirectChat()
{
    if (targetRoom->directChatUsers().contains(c->user()))
    {
        cout << "Warning: the room is already a direct chat,"
                " only unmarking will be tested" << endl;
        checkDirectChatOutcome({{ c->user(), targetRoom->id() }});
        return;
    }
    // Connect first because the signal is emitted synchronously.
    connect(c.data(), &Connection::directChatsListChanged,
            this, &QMCTest::checkDirectChatOutcome);
    cout << "Marking the room as a direct chat" << endl;
    c->addToDirectChats(targetRoom, c->user());
}

void QMCTest::checkDirectChatOutcome(const Connection::DirectChatsMap& added)
{
    running.push_back("Direct chat test");
    disconnect(c.data(), &Connection::directChatsListChanged, nullptr, nullptr);
    if (!targetRoom->isDirectChat())
    {
        cout << "The room has not been marked as a direct chat" << endl;
        QMC_CHECK("Direct chat test", false);
        return;
    }
    if (!added.contains(c->user(), targetRoom->id()))
    {
        cout << "The room has not been listed in new direct chats" << endl;
        QMC_CHECK("Direct chat test", false);
        return;
    }

    cout << "Unmarking the direct chat" << endl;
    c->removeFromDirectChats(targetRoom->id(), c->user());
    QMC_CHECK("Direct chat test", !c->isDirectChat(targetRoom->id()));
}

void QMCTest::conclude()
{
    auto succeededRec = QString::number(succeeded.size()) + " tests succeeded";
    if (!failed.isEmpty() || !running.isEmpty())
        succeededRec += " of " %
            QString::number(succeeded.size() + failed.size() + running.size()) %
            " total";
    QString plainReport = origin % ": Testing complete, " % succeededRec;
    QString color = failed.isEmpty() && running.isEmpty() ? "00AA00" : "AA0000";
    QString htmlReport = origin % ": <strong><font data-mx-color='#" % color %
            "' color='#" % color % "'>Testing complete</font></strong>, " %
            succeededRec;
    if (!failed.isEmpty())
    {
        plainReport += "\nFAILED: " % failed.join(", ");
        htmlReport += "<br><strong>Failed:</strong> " % failed.join(", ");
    }
    if (!running.isEmpty())
    {
        plainReport += "\nDID NOT FINISH: " % running.join(", ");
        htmlReport +=
            "<br><strong>Did not finish:</strong> " % running.join(", ");
    }
    cout << plainReport.toStdString() << endl;

    if (targetRoom)
    {
            // TODO: Waiting for proper futures to come so that it could be:
//            targetRoom->postHtmlText(...)
//            .then(this, &QMCTest::finalize); // Qt-style or
//            .then([this] { finalize(); }); // STL-style
        auto txnId = targetRoom->postHtmlText(plainReport, htmlReport);
        connect(targetRoom, &Room::messageSent, this,
            [this,txnId] (QString serverTxnId) {
                if (txnId != serverTxnId)
                    return;

                cout << "Leaving the room" << endl;
                connect(targetRoom->leaveRoom(), &BaseJob::finished,
                        this, &QMCTest::finalize);
            });
    }
    else
        finalize();
}

void QMCTest::finalize()
{
    cout << "Logging out" << endl;
    c->logout();
    connect(c.data(), &Connection::loggedOut, qApp,
        [this] {
            QCoreApplication::processEvents();
            QCoreApplication::exit(failed.size() + running.size());
        });
}

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    if (argc < 4)
    {
        cout << "Usage: qmc-example <user> <passwd> <device_name> [<room_alias> [origin]]" << endl;
        return -1;
    }

    cout << "Connecting to the server as " << argv[1] << endl;
    auto conn = new Connection;
    conn->connectToServer(argv[1], argv[2], argv[3]);
    QMCTest test { conn, argc >= 5 ? argv[4] : nullptr,
                         argc >= 6 ? argv[5] : nullptr };
    return app.exec();
}
