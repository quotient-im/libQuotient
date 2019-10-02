
#include "connection.h"
#include "room.h"
#include "user.h"

#include "csapi/joining.h"
#include "csapi/leaving.h"
#include "csapi/room_send.h"

#include "events/reactionevent.h"
#include "events/simplestateevents.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QFileInfo>
#include <QtCore/QStringBuilder>
#include <QtCore/QTemporaryFile>
#include <QtCore/QTimer>

#include <functional>
#include <iostream>

using namespace Quotient;
using std::cout, std::endl;

class TestManager : public QObject {
public:
    TestManager(Connection* conn, QString testRoomName, QString source);

private slots:
    // clang-format off
    void setupAndRun();
    void onNewRoom(Room* r);
    void run();
    void doTests();
        void loadMembers();
        void sendMessage();
            void sendReaction(const QString& targetEvtId);
        void sendFile();
            void checkFileSendingOutcome(const QString& txnId,
                                         const QString& fileName);
        void setTopic();
        void sendAndRedact();
            bool checkRedactionOutcome(const QString& evtIdToRedact);
        void addAndRemoveTag();
        void markDirectChat();
            void checkDirectChatOutcome(
                    const Connection::DirectChatsMap& added);
    void conclude();
    void finalize();
    // clang-format on

private:
    QScopedPointer<Connection, QScopedPointerDeleteLater> c;
    QStringList running;
    QStringList succeeded;
    QStringList failed;
    QString origin;
    QString targetRoomName;
    Room* targetRoom = nullptr;

using TestToken = QByteArray; // return value of QMetaMethod::name
// For now, the token itself is the test name but that may change.
const char* testName(const TestToken& token) { return token.constData(); }
    bool validatePendingEvent(const QString& txnId);
    void finishTest(const TestToken& token, bool condition, const char* file,
                    int line);};

#define TEST_IMPL(Name) void TestManager::Name()

#define FINISH_TEST(description, Condition) \
    finishTest(description, Condition, __FILE__, __LINE__)

#define FAIL_TEST(description) FINISH_TEST(description, false)

bool TestManager::validatePendingEvent(const QString& txnId)
{
    auto it = targetRoom->findPendingEvent(txnId);
    return it != targetRoom->pendingEvents().end()
           && it->deliveryStatus() == EventStatus::Submitted
           && (*it)->transactionId() == txnId;
}

void TestManager::finishTest(const TestToken& token, bool condition,
                           const char* file, int line)
{
    const auto& item = testName(token);
    Q_ASSERT_X(running.contains(item), item,
               "Trying to finish an item that's not running");
    running.removeOne(item);
    if (condition) {
        succeeded.push_back(item);
        cout << item << " successful" << endl;
        if (targetRoom)
            targetRoom->postMessage(origin % ": " % item % " successful",
                                    MessageEventType::Notice);
    } else {
        failed.push_back(item);
        cout << item << " FAILED at " << file << ":" << line << endl;
        if (targetRoom)
            targetRoom->postPlainText(origin % ": " % item % " FAILED at "
                                      % file % ", line " % QString::number(line));
    }
}

TestManager::TestManager(Connection* conn, QString testRoomName, QString source)
    : c(conn), origin(std::move(source)), targetRoomName(std::move(testRoomName))
{
    if (!origin.isEmpty())
        cout << "Origin for the test message: " << origin.toStdString() << endl;
    cout << "Test room name: " << targetRoomName.toStdString() << endl;

    connect(c.data(), &Connection::connected, this, &TestManager::setupAndRun);
    connect(c.data(), &Connection::loadedRoomState, this, &TestManager::onNewRoom);
    // Big countdown watchdog
    QTimer::singleShot(180000, this, &TestManager::conclude);
}

void TestManager::setupAndRun()
{
    Q_ASSERT(!c->homeserver().isEmpty() && c->homeserver().isValid());
    Q_ASSERT(c->domain() == c->userId().section(':', 1));
    cout << "Connected, server: "
         << c->homeserver().toDisplayString().toStdString() << endl;
    cout << "Access token: " << c->accessToken().toStdString() << endl;

    cout << "Joining " << targetRoomName.toStdString() << endl;
    running.push_back("Join room");
    auto joinJob = c->joinRoom(targetRoomName);
    connect(joinJob, &BaseJob::failure, this, [this] {
        FAIL_TEST("Join room");
        conclude();
    });
    // Connection::joinRoom() creates a Room object upon JoinRoomJob::success
    // but this object is empty until the first sync is done.
    connect(joinJob, &BaseJob::success, this, [this, joinJob] {
        targetRoom = c->room(joinJob->roomId(), JoinState::Join);
        FINISH_TEST("Join room", targetRoom != nullptr);

        run();
    });
}

void TestManager::onNewRoom(Room* r)
{
    cout << "New room: " << r->id().toStdString() << endl
         << "  Name: " << r->name().toStdString() << endl
         << "  Canonical alias: " << r->canonicalAlias().toStdString() << endl
         << endl;
    connect(r, &Room::aboutToAddNewMessages, r, [r](RoomEventsRange timeline) {
        cout << timeline.size() << " new event(s) in room "
             << r->canonicalAlias().toStdString() << endl;
        //        for (const auto& item: timeline)
        //        {
        //            cout << "From: "
        //                 << r->roomMembername(item->senderId()).toStdString()
        //                 << endl << "Timestamp:"
        //                 << item->timestamp().toString().toStdString() << endl
        //                 << "JSON:" << endl <<
        //                 item->originalJson().toStdString() << endl;
        //        }
    });
}

void TestManager::run()
{
    c->setLazyLoading(true);
    c->syncLoop();
    connectSingleShot(c.data(), &Connection::syncDone, this, &TestManager::doTests);
    connect(c.data(), &Connection::syncDone, c.data(), [this] {
        cout << "Sync complete, " << running.size()
             << " test(s) in the air: " << running.join(", ").toStdString()
             << endl;
        if (running.isEmpty())
            conclude();
    });
}

void TestManager::doTests()
{
    cout << "Starting tests" << endl;

    loadMembers();

    sendMessage();
    sendFile();
    setTopic();
    addAndRemoveTag();
    sendAndRedact();
    markDirectChat();
    // Add here tests with the test room
}

TEST_IMPL(loadMembers)
{
    running.push_back("Loading members");
    auto* r = c->roomByAlias(QStringLiteral("#quotient:matrix.org"),
                             JoinState::Join);
    if (!r) {
        cout << "#quotient:matrix.org is not found in the test user's rooms"
             << endl;
        FAIL_TEST("Loading members");
        return;
    }
    // It's not exactly correct because an arbitrary server might not support
    // lazy loading; but in the absence of capabilities framework we assume
    // it does.
    if (r->memberNames().size() >= r->joinedCount()) {
        cout << "Lazy loading doesn't seem to be enabled" << endl;
        FAIL_TEST("Loading members");
        return;
    }
    r->setDisplayed();
    connect(r, &Room::allMembersLoaded, [this, r] {
        FINISH_TEST("Loading members",
                    r->memberNames().size() >= r->joinedCount());
    });
}

TEST_IMPL(sendMessage)
{
    running.push_back("Message sending");
    cout << "Sending a message" << endl;
    auto txnId = targetRoom->postPlainText("Hello, " % origin % " is here");
    if (!validatePendingEvent(txnId)) {
        cout << "Invalid pending event right after submitting" << endl;
        FAIL_TEST("Message sending");
        return;
    }
    connectUntil(
        targetRoom, &Room::pendingEventAboutToMerge, this,
        [this, txnId](const RoomEvent* evt, int pendingIdx) {
            const auto& pendingEvents = targetRoom->pendingEvents();
            Q_ASSERT(pendingIdx >= 0 && pendingIdx < int(pendingEvents.size()));

            if (evt->transactionId() != txnId)
                return false;

            FINISH_TEST("Message sending",
                        is<RoomMessageEvent>(*evt) && !evt->id().isEmpty()
                            && pendingEvents[size_t(pendingIdx)]->transactionId()
                                   == evt->transactionId());
            sendReaction(evt->id());
            return true;
        });
}

void TestManager::sendReaction(const QString& targetEvtId)
{
    running.push_back("Reaction sending");
    cout << "Reacting to the newest message in the room" << endl;
    Q_ASSERT(targetRoom->timelineSize() > 0);
    const auto key = QStringLiteral("+1");
    const auto txnId = targetRoom->postReaction(targetEvtId, key);
    if (!validatePendingEvent(txnId)) {
        cout << "Invalid pending event right after submitting" << endl;
        FAIL_TEST("Reaction sending");
        return;
    }

    // TODO: Check that it came back as a reaction event and that it attached to
    // the right event
    connectUntil(
        targetRoom, &Room::updatedEvent, this,
        [this, txnId, key, targetEvtId](const QString& actualTargetEvtId) {
            if (actualTargetEvtId != targetEvtId)
                return false;
            const auto reactions = targetRoom->relatedEvents(
                targetEvtId, EventRelation::Annotation());
            // It's a test room, assuming no interference there should
            // be exactly one reaction
            if (reactions.size() != 1) {
                FAIL_TEST("Reaction sending");
            } else {
                const auto* evt =
                    eventCast<const ReactionEvent>(reactions.back());
                FINISH_TEST("Reaction sending",
                            is<ReactionEvent>(*evt) && !evt->id().isEmpty()
                                && evt->relation().key == key
                                && evt->transactionId() == txnId);
            }
            return true;
        });
}

TEST_IMPL(sendFile)
{
    running.push_back("File sending");
    cout << "Sending a file" << endl;
    auto* tf = new QTemporaryFile;
    if (!tf->open()) {
        cout << "Failed to create a temporary file" << endl;
        FAIL_TEST("File sending");
        return;
    }
    tf->write("Test");
    tf->close();
    // QFileInfo::fileName brings only the file name; QFile::fileName brings
    // the full path
    const auto tfName = QFileInfo(*tf).fileName();
    cout << "Sending file " << tfName.toStdString() << endl;
    const auto txnId =
        targetRoom->postFile("Test file", QUrl::fromLocalFile(tf->fileName()));
    if (!validatePendingEvent(txnId)) {
        cout << "Invalid pending event right after submitting" << endl;
        delete tf;
        FAIL_TEST("File sending");
        return;
    }

    // FIXME: Clean away connections (connectUntil doesn't help here).
    connect(targetRoom, &Room::fileTransferCompleted, this,
            [this, txnId, tf, tfName](const QString& id) {
                auto fti = targetRoom->fileTransferInfo(id);
                Q_ASSERT(fti.status == FileTransferInfo::Completed);

                if (id != txnId)
                    return;

                delete tf;

                checkFileSendingOutcome(txnId, tfName);
            });
    connect(targetRoom, &Room::fileTransferFailed, this,
            [this, txnId, tf](const QString& id, const QString& error) {
                if (id != txnId)
                    return;

                targetRoom->postPlainText(origin % ": File upload failed: "
                                          % error);
                delete tf;

                FAIL_TEST("File sending");
            });
}

void TestManager::checkFileSendingOutcome(const QString& txnId,
                                      const QString& fileName)
{
    auto it = targetRoom->findPendingEvent(txnId);
    if (it == targetRoom->pendingEvents().end()) {
        cout << "Pending file event dropped before upload completion" << endl;
        FAIL_TEST("File sending");
        return;
    }
    if (it->deliveryStatus() != EventStatus::FileUploaded) {
        cout << "Pending file event status upon upload completion is "
             << it->deliveryStatus() << " != FileUploaded("
             << EventStatus::FileUploaded << ')' << endl;
        FAIL_TEST("File sending");
        return;
    }

    connectUntil(
        targetRoom, &Room::pendingEventAboutToMerge, this,
        [this, txnId, fileName](const RoomEvent* evt, int pendingIdx) {
            const auto& pendingEvents = targetRoom->pendingEvents();
            Q_ASSERT(pendingIdx >= 0 && pendingIdx < int(pendingEvents.size()));

            if (evt->transactionId() != txnId)
                return false;

            cout << "File event " << txnId.toStdString()
                 << " arrived in the timeline" << endl;
            visit(
                *evt,
                [&](const RoomMessageEvent& e) {
                    FINISH_TEST(
                        "File sending",
                        !e.id().isEmpty()
                            && pendingEvents[size_t(pendingIdx)]->transactionId()
                                   == txnId
                            && e.hasFileContent()
                            && e.content()->fileInfo()->originalName == fileName);
                },
                [this](const RoomEvent&) { FAIL_TEST("File sending"); });
            return true;
        });
}

TEST_IMPL(setTopic)
{
    running.push_back("State setting test");

    const auto newTopic = c->generateTxnId(); // Just a way to get a unique id
    targetRoom->setTopic(newTopic);

    connectUntil(targetRoom, &Room::topicChanged, this,
                 [this, newTopic] {
                     FINISH_TEST("State setting test",
                                 targetRoom->topic() == newTopic);
                     return true;
                 });
}

TEST_IMPL(sendAndRedact)
{
    running.push_back("Redaction");
    cout << "Sending a message to redact" << endl;
    auto txnId = targetRoom->postPlainText(origin % ": message to redact");
    if (txnId.isEmpty()) {
        FAIL_TEST("Redaction");
        return;
    }
    connect(targetRoom, &Room::messageSent, this,
            [this, txnId](const QString& tId, const QString& evtId) {
                if (tId != txnId)
                    return;

                cout << "Redacting the message" << endl;
                targetRoom->redactEvent(evtId, origin);

                connectUntil(targetRoom, &Room::addedMessages, this,
                             [this, evtId] {
                                 return checkRedactionOutcome(evtId);
                             });
            });
}

bool TestManager::checkRedactionOutcome(const QString& evtIdToRedact)
{
    // There are two possible (correct) outcomes: either the event comes already
    // redacted at the next sync, or the nearest sync completes with
    // the unredacted event but the next one brings redaction.
    auto it = targetRoom->findInTimeline(evtIdToRedact);
    if (it == targetRoom->timelineEdge())
        return false; // Waiting for the next sync

    if ((*it)->isRedacted()) {
        cout << "The sync brought already redacted message" << endl;
        FINISH_TEST("Redaction", true);
    } else {
        cout << "Message came non-redacted with the sync, waiting for redaction"
             << endl;
        connectUntil(targetRoom, &Room::replacedEvent, this,
                     [this, evtIdToRedact](const RoomEvent* newEvent,
                                           const RoomEvent* oldEvent) {
                         if (oldEvent->id() != evtIdToRedact)
                             return false;

                         FINISH_TEST("Redaction",
                                     newEvent->isRedacted()
                                         && newEvent->redactionReason() == origin);
                         return true;
                     });
    }
    return true;
}

TEST_IMPL(addAndRemoveTag)
{
    running.push_back("Tagging test");
    static const auto TestTag = QStringLiteral("org.quotient.test");
    // Pre-requisite
    if (targetRoom->tags().contains(TestTag))
        targetRoom->removeTag(TestTag);

    // Connect first because the signal is emitted synchronously.
    connect(targetRoom, &Room::tagsChanged, targetRoom, [=] {
        cout << "Room " << targetRoom->id().toStdString()
             << ", tag(s) changed:" << endl
             << "  " << targetRoom->tagNames().join(", ").toStdString() << endl;
        if (targetRoom->tags().contains(TestTag)) {
            cout << "Test tag set, removing it now" << endl;
            targetRoom->removeTag(TestTag);
            FINISH_TEST("Tagging test", !targetRoom->tags().contains(TestTag));
            disconnect(targetRoom, &Room::tagsChanged, nullptr, nullptr);
        }
    });
    cout << "Adding a tag" << endl;
    targetRoom->addTag(TestTag);
}

TEST_IMPL(markDirectChat)
{
    running.push_back("Direct chat test");
    if (targetRoom->directChatUsers().contains(c->user())) {
        cout << "Warning: the room is already a direct chat,"
                " only unmarking will be tested"
             << endl;
        checkDirectChatOutcome({ { c->user(), targetRoom->id() } });
        return;
    }
    // Connect first because the signal is emitted synchronously.
    connect(c.data(), &Connection::directChatsListChanged, this,
            &TestManager::checkDirectChatOutcome);
    cout << "Marking the room as a direct chat" << endl;
    c->addToDirectChats(targetRoom, c->user());
}

void TestManager::checkDirectChatOutcome(const Connection::DirectChatsMap& added)
{
    disconnect(c.data(), &Connection::directChatsListChanged, nullptr, nullptr);
    if (!targetRoom->isDirectChat()) {
        cout << "The room has not been marked as a direct chat" << endl;
        FAIL_TEST("Direct chat test");
        return;
    }
    if (!added.contains(c->user(), targetRoom->id())) {
        cout << "The room has not been listed in new direct chats" << endl;
        FAIL_TEST("Direct chat test");
        return;
    }

    cout << "Unmarking the direct chat" << endl;
    c->removeFromDirectChats(targetRoom->id(), c->user());
    FINISH_TEST("Direct chat test", !c->isDirectChat(targetRoom->id()));
}

void TestManager::conclude()
{
    c->stopSync();
    auto succeededRec = QString::number(succeeded.size()) + " tests succeeded";
    if (!failed.empty() || !running.empty())
        succeededRec +=
            " of "
            % QString::number(succeeded.size() + failed.size() + running.size())
            % " total";
    QString plainReport = origin % ": Testing complete, " % succeededRec;
    QString color = failed.empty() && running.empty() ? "00AA00" : "AA0000";
    QString htmlReport = origin % ": <strong><font data-mx-color='#" % color
                         % "' color='#" % color
                         % "'>Testing complete</font></strong>, " % succeededRec;
    if (!failed.empty()) {
        plainReport += "\nFAILED: " % failed.join(", ");
        htmlReport += "<br><strong>Failed:</strong> " % failed.join(", ");
    }
    if (!running.empty()) {
        plainReport += "\nDID NOT FINISH: " % running.join(", ");
        htmlReport += "<br><strong>Did not finish:</strong> "
                      % running.join(", ");
    }
    cout << plainReport.toStdString() << endl;

    if (targetRoom) {
        // TODO: Waiting for proper futures to come so that it could be:
        //            targetRoom->postHtmlText(...)
        //            .then(this, &TestManager::finalize); // Qt-style or
        //            .then([this] { finalize(); }); // STL-style
        auto txnId = targetRoom->postHtmlText(plainReport, htmlReport);
        connect(targetRoom, &Room::messageSent, this,
                [this, txnId](const QString& serverTxnId) {
                    if (txnId != serverTxnId)
                        return;

                    cout << "Leaving the room" << endl;
                    connect(targetRoom->leaveRoom(), &BaseJob::finished, this,
                            &TestManager::finalize);
                });
    } else
        finalize();
}

void TestManager::finalize()
{
    cout << "Logging out" << endl;
    c->logout();
    connect(c.data(), &Connection::loggedOut, qApp, [this] {
        QCoreApplication::processEvents();
        QCoreApplication::exit(failed.size() + running.size());
    });
}

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    if (argc < 5) {
        cout << "Usage: quotest <user> <passwd> <device_name> <room_alias> [origin]"
             << endl;
        return -1;
    }

    cout << "Connecting to the server as " << argv[1] << endl;
    auto conn = new Connection;
    conn->connectToServer(argv[1], argv[2], argv[3]);
    TestManager test { conn, argv[4], argc >= 6 ? argv[5] : nullptr };
    return app.exec();
}
