
#include "connection.h"
#include "room.h"
#include "user.h"

#include "csapi/joining.h"
#include "csapi/leaving.h"
#include "csapi/room_send.h"

#include "events/reactionevent.h"
#include "events/redactionevent.h"
#include "events/simplestateevents.h"

#include <QtTest/QSignalSpy>
#include <QtCore/QCoreApplication>
#include <QtCore/QFileInfo>
#include <QtCore/QStringBuilder>
#include <QtCore/QTemporaryFile>
#include <QtCore/QTimer>

#include <functional>
#include <iostream>

using namespace Quotient;
using std::cout, std::endl;

class TestSuite;

class TestManager : public QObject {
public:
    TestManager(Connection* conn, QString testRoomName, QString source);

private:
    void setupAndRun();
    void onNewRoom(Room* r);
    void doTests();
    void conclude();
    void finalize();

private:
    QScopedPointer<Connection, QScopedPointerDeleteLater> c;
    QString origin;
    QString targetRoomName;
    TestSuite* testSuite = nullptr;
    QByteArrayList running {}, succeeded {}, failed {};
};

using TestToken = QByteArray; // return value of QMetaMethod::name
// For now, the token itself is the test name but that may change.
const char* testName(const TestToken& token) { return token.constData(); }

/// Test function declaration
/*!
 * \return true, if the test finished (successfully or unsuccessfully);
 *         false, if the test went async and will complete later
 */
#define TEST_DECL(Name) bool Name(const TestToken& thisTest);

/// The holder for the actual tests
/*!
 * This class takes inspiration from Qt Test in terms of tests invocation;
 * TestManager instantiates it and runs all public slots (cf. private slots in
 * Qt Test) one after another. An important diversion from Qt Test is that
 * the tests are assumed to by asynchronous rather than synchronous; so it's
 * perfectly normal to have a few tests running at the same time. To avoid
 * context clashes a special parameter with the name thisTest is passed to
 * each test. Each test must conclude (synchronously or asynchronously) with
 * an invocation of FINISH_TEST() macro (or FAIL_TEST() macro that expands to
 * FINISH_TEST) that expects thisTest variable to be reachable. If FINISH_TEST()
 * is invoked twice with the same thisTest, the second call will cause assertion
 * failure; if FINISH_TEST() is not invoked at all, the test will be killed
 * by a watchdog after a timeout and marked in the final report as not finished.
 */
class TestSuite : public QObject {
    Q_OBJECT
public:
    TestSuite(Connection* conn, QString testRoomAlias, QString source,
              TestManager* parent)
        : QObject(parent)
        , targetConn(conn)
        , targetRoomAlias(std::move(testRoomAlias))
        , origin(std::move(source))
    {
        Q_ASSERT(conn && parent);
        Q_ASSERT(targetRoomAlias.startsWith('!')
                 || targetRoomAlias.startsWith('#'));
    }

    TEST_DECL(joinRoom)

signals:
    void finishedItem(QByteArray /*name*/, bool /*condition*/);

public slots:
    TEST_DECL(loadMembers)
    TEST_DECL(sendMessage)
    TEST_DECL(sendReaction)
    TEST_DECL(sendFile)
    TEST_DECL(setTopic)
    TEST_DECL(sendAndRedact)
    TEST_DECL(addAndRemoveTag)
    TEST_DECL(markDirectChat)
    // Add more tests above here

public:
    Room* room() const { return targetRoom; }
    Connection* connection() const { return targetConn; }

private slots:
    bool checkFileSendingOutcome(const TestToken& thisTest,
                                 const QString& txnId, const QString& fileName);
    bool checkRedactionOutcome(const QByteArray& thisTest,
                               const QString& evtIdToRedact);

private:
    bool validatePendingEvent(const QString& txnId);
    bool checkDirectChat() const;
    void finishTest(const TestToken& token, bool condition, const char* file,
                    int line);

private:
    Connection* targetConn;
    QString targetRoomAlias;
    QString origin;
    Room* targetRoom = nullptr;
};

#define TEST_IMPL(Name) bool TestSuite::Name(const TestToken& thisTest)

// Returning true (rather than a void) allows to reuse the convention with
// connectUntil() to break the QMetaObject::Connection upon finishing the test
// item.
#define FINISH_TEST(Condition) \
    return (finishTest(thisTest, Condition, __FILE__, __LINE__), true)

#define FAIL_TEST() FINISH_TEST(false)

bool TestSuite::validatePendingEvent(const QString& txnId)
{
    auto it = targetRoom->findPendingEvent(txnId);
    return it != targetRoom->pendingEvents().end()
           && it->deliveryStatus() == EventStatus::Submitted
           && (*it)->transactionId() == txnId;
}

void TestSuite::finishTest(const TestToken& token, bool condition,
                           const char* file, int line)
{
    const auto& item = testName(token);
    if (condition) {
        cout << item << " successful" << endl;
        if (targetRoom)
            targetRoom->postMessage(origin % ": " % item % " successful",
                                    MessageEventType::Notice);
    } else {
        cout << item << " FAILED at " << file << ":" << line << endl;
        if (targetRoom)
            targetRoom->postPlainText(origin % ": " % item % " FAILED at "
                                      % file % ", line " % QString::number(line));
    }

    emit finishedItem(item, condition);
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

    c->setLazyLoading(true);
    c->syncLoop();
    connectSingleShot(c.data(), &Connection::syncDone, this,
                      &TestManager::doTests);
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

void TestManager::doTests()
{
    cout << "Starting tests" << endl;
    Q_ASSERT(!targetRoomName.isEmpty());
    testSuite = new TestSuite(c.data(), targetRoomName, origin, this);
    connect(testSuite, &TestSuite::finishedItem, this,
            [this](const QByteArray& itemName, bool condition) {
                if (auto i = running.indexOf(itemName); i != -1)
                    (condition ? succeeded : failed).push_back(running.takeAt(i));
                else
                    Q_ASSERT_X(false, itemName,
                               "Test item is not in running state");
            });

    running.push_back("joinRoom");
    testSuite->joinRoom("joinRoom");
    connectSingleShot(testSuite, &TestSuite::finishedItem, this,
        [this](const QByteArray&, bool condition) {
            if (!condition) {
                finalize();
                return;
            }
            const auto* metaObj = testSuite->metaObject();
            for (auto i = metaObj->methodOffset(); i < metaObj->methodCount();
                 ++i) {
                const auto metaMethod = metaObj->method(i);
                if (metaMethod.access() != QMetaMethod::Public
                    || metaMethod.methodType() != QMetaMethod::Slot)
                    continue;

                // By now connectSingleShot() has already disconnected this
                // slot so the tests below can emit finishedItem() without
                // the risk of recursion.
                cout << "Starting: " << metaMethod.name().constData() << endl;
                running.push_back(metaMethod.name());
                metaMethod.invoke(testSuite, Qt::DirectConnection,
                                  Q_ARG(QByteArray, metaMethod.name()));
            }
        });
    connect(c.data(), &Connection::syncDone, c.data(), [this] {
        cout << "Sync complete, ";
        if (running.empty()) {
            cout << "all tests finished" << endl;
            conclude();
            return;
        }
        cout << running.size() << " test(s) in the air:";
        for (const auto& test: qAsConst(running))
            cout << " " << testName(test);
        cout << endl;
        if (auto* r = testSuite->room())
            cout << "Test room timeline size = " << r->timelineSize()
                 << ", pending size = " << r->pendingEvents().size() << endl;
    });
}

TEST_IMPL(joinRoom)
{
    cout << "Joining " << targetRoomAlias.toStdString() << endl;
    auto joinJob = connection()->joinRoom(targetRoomAlias);
    // Ensure, before this test is completed, that the room has been joined
    // and filled with some events so that other tests could use that
    connect(joinJob, &BaseJob::success, this, [this, joinJob, thisTest] {
        targetRoom = connection()->room(joinJob->roomId());
        targetRoom->getPreviousContent();
        connectUntil(targetRoom, &Room::addedMessages, this, [this, thisTest] {
            FINISH_TEST(targetRoom->memberJoinState(connection()->user())
                        == JoinState::Join);
        });
    });
    connect(joinJob, &BaseJob::failure, this, [this, thisTest] { FAIL_TEST(); });
    return false;
}

TEST_IMPL(loadMembers)
{
    // Trying to load members from another (larger) room
    auto* r = connection()->roomByAlias(QStringLiteral("#quotient:matrix.org"),
                                        JoinState::Join);
    if (!r) {
        cout << "#quotient:matrix.org is not found in the test user's rooms"
             << endl;
        FAIL_TEST();
    }
    // It's not exactly correct because an arbitrary server might not support
    // lazy loading; but in the absence of capabilities framework we assume
    // it does.
    if (r->memberNames().size() >= r->joinedCount()) {
        cout << "Lazy loading doesn't seem to be enabled" << endl;
        FAIL_TEST();
    }
    r->setDisplayed();
    connect(r, &Room::allMembersLoaded, [this, thisTest, r] {
        FINISH_TEST(r->memberNames().size() >= r->joinedCount());
    });
    return false;
}

TEST_IMPL(sendMessage)
{
    auto txnId = targetRoom->postPlainText("Hello, " % origin % " is here");
    if (!validatePendingEvent(txnId)) {
        cout << "Invalid pending event right after submitting" << endl;
        FAIL_TEST();
    }
    connectUntil(targetRoom, &Room::pendingEventAboutToMerge, this,
        [this, thisTest, txnId](const RoomEvent* evt, int pendingIdx) {
            const auto& pendingEvents = targetRoom->pendingEvents();
            Q_ASSERT(pendingIdx >= 0 && pendingIdx < int(pendingEvents.size()));

            if (evt->transactionId() != txnId)
                return false;

            FINISH_TEST(is<RoomMessageEvent>(*evt) && !evt->id().isEmpty()
                        && pendingEvents[size_t(pendingIdx)]->transactionId()
                               == evt->transactionId());
        });
    return false;
}

TEST_IMPL(sendReaction)
{
    cout << "Reacting to the newest message in the room" << endl;
    Q_ASSERT(targetRoom->timelineSize() > 0);
    const auto targetEvtId = targetRoom->messageEvents().back()->id();
    const auto key = QStringLiteral("+1");
    const auto txnId = targetRoom->postReaction(targetEvtId, key);
    if (!validatePendingEvent(txnId)) {
        cout << "Invalid pending event right after submitting" << endl;
        FAIL_TEST();
    }

    // TODO: Check that it came back as a reaction event and that it attached to
    // the right event
    connectUntil(
        targetRoom, &Room::updatedEvent, this,
        [this, thisTest, txnId, key, targetEvtId](const QString& actualTargetEvtId) {
            if (actualTargetEvtId != targetEvtId)
                return false;
            const auto reactions = targetRoom->relatedEvents(
                targetEvtId, EventRelation::Annotation());
            // It's a test room, assuming no interference there should
            // be exactly one reaction
            if (reactions.size() != 1)
                FAIL_TEST();

            const auto* evt =
                eventCast<const ReactionEvent>(reactions.back());
            FINISH_TEST(is<ReactionEvent>(*evt) && !evt->id().isEmpty()
                        && evt->relation().key == key
                        && evt->transactionId() == txnId);
        });
    return false;
}

TEST_IMPL(sendFile)
{
    auto* tf = new QTemporaryFile;
    if (!tf->open()) {
        cout << "Failed to create a temporary file" << endl;
        FAIL_TEST();
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
        FAIL_TEST();
    }

    // FIXME: Clean away connections (connectUntil doesn't help here).
    connectUntil(targetRoom, &Room::fileTransferCompleted, this,
        [this, thisTest, txnId, tf, tfName](const QString& id) {
            auto fti = targetRoom->fileTransferInfo(id);
            Q_ASSERT(fti.status == FileTransferInfo::Completed);

            if (id != txnId)
                return false;

            delete tf;
            return checkFileSendingOutcome(thisTest, txnId, tfName);
        });
    connectUntil(targetRoom, &Room::fileTransferFailed, this,
        [this, thisTest, txnId, tf](const QString& id, const QString& error) {
            if (id != txnId)
                return false;

            targetRoom->postPlainText(origin % ": File upload failed: " % error);
            delete tf;
            FAIL_TEST();
        });
    return false;
}

bool TestSuite::checkFileSendingOutcome(const TestToken& thisTest,
                                        const QString& txnId,
                                        const QString& fileName)
{
    auto it = targetRoom->findPendingEvent(txnId);
    if (it == targetRoom->pendingEvents().end()) {
        cout << "Pending file event dropped before upload completion" << endl;
        FAIL_TEST();
    }
    if (it->deliveryStatus() != EventStatus::FileUploaded) {
        cout << "Pending file event status upon upload completion is "
             << it->deliveryStatus() << " != FileUploaded("
             << EventStatus::FileUploaded << ')' << endl;
        FAIL_TEST();
    }

    connectUntil(targetRoom, &Room::pendingEventAboutToMerge, this,
        [this, thisTest, txnId, fileName](const RoomEvent* evt, int pendingIdx) {
            const auto& pendingEvents = targetRoom->pendingEvents();
            Q_ASSERT(pendingIdx >= 0 && pendingIdx < int(pendingEvents.size()));

            if (evt->transactionId() != txnId)
                return false;

            cout << "File event " << txnId.toStdString()
                 << " arrived in the timeline" << endl;
            // This part tests visit()
            return visit(
                *evt,
                [&](const RoomMessageEvent& e) {
                    FINISH_TEST(
                        !e.id().isEmpty()
                            && pendingEvents[size_t(pendingIdx)]->transactionId()
                                   == txnId
                            && e.hasFileContent()
                            && e.content()->fileInfo()->originalName == fileName);
                },
                [this, thisTest](const RoomEvent&) { FAIL_TEST(); });
        });
    return true;
}

TEST_IMPL(setTopic)
{
    const auto newTopic = connection()->generateTxnId(); // Just a way to make
                                                         // a unique id
    targetRoom->setTopic(newTopic);
    connectUntil(targetRoom, &Room::topicChanged, this,
        [this, thisTest, newTopic] {
            if (targetRoom->topic() == newTopic)
                FINISH_TEST(true);

            cout << "Requested topic was " << newTopic.toStdString() << ", "
                 << targetRoom->topic().toStdString() << " arrived instead"
                 << endl;
            return false;
        });
    return false;
}

TEST_IMPL(sendAndRedact)
{
    cout << "Sending a message to redact" << endl;
    auto txnId = targetRoom->postPlainText(origin % ": message to redact");
    if (txnId.isEmpty())
        FAIL_TEST();

    connect(targetRoom, &Room::messageSent, this,
            [this, thisTest, txnId](const QString& tId, const QString& evtId) {
                if (tId != txnId)
                    return;

                cout << "Redacting the message" << endl;
                targetRoom->redactEvent(evtId, origin);

                connectUntil(targetRoom, &Room::addedMessages, this,
                             [this, thisTest, evtId] {
                                 return checkRedactionOutcome(thisTest, evtId);
                             });
            });
    return false;
}

bool TestSuite::checkRedactionOutcome(const QByteArray& thisTest,
                                      const QString& evtIdToRedact)
{
    // There are two possible (correct) outcomes: either the event comes already
    // redacted at the next sync, or the nearest sync completes with
    // the unredacted event but the next one brings redaction.
    auto it = targetRoom->findInTimeline(evtIdToRedact);
    if (it == targetRoom->timelineEdge())
        return false; // Waiting for the next sync

    if ((*it)->isRedacted()) {
        cout << "The sync brought already redacted message" << endl;
        FINISH_TEST(true);
    }

    cout << "Message came non-redacted with the sync, waiting for redaction"
         << endl;
    connectUntil(targetRoom, &Room::replacedEvent, this,
                 [this, thisTest, evtIdToRedact](const RoomEvent* newEvent,
                                                 const RoomEvent* oldEvent) {
                     if (oldEvent->id() != evtIdToRedact)
                         return false;

                     FINISH_TEST(newEvent->isRedacted()
                                 && newEvent->redactionReason() == origin);
                 });
    return true;
}

TEST_IMPL(addAndRemoveTag)
{
    static const auto TestTag = QStringLiteral("im.quotient.test");
    // Pre-requisite
    if (targetRoom->tags().contains(TestTag))
        targetRoom->removeTag(TestTag);

    // Unlike for most of Quotient, tags are applied and tagsChanged is emitted
    // synchronously, with the server being notified async. The test checks
    // that the signal is emitted, not only that tags have changed; but there's
    // (currently) no way to check that the server has been correctly notified
    // of the tag change.
    QSignalSpy spy(targetRoom, &Room::tagsChanged);
    targetRoom->addTag(TestTag);
    if (spy.count() != 1 || !targetRoom->tags().contains(TestTag)) {
        cout << "Tag adding failed" << endl;
        FAIL_TEST();
    }
    cout << "Test tag set, removing it now" << endl;
    targetRoom->removeTag(TestTag);
    FINISH_TEST(spy.count() == 2 && !targetRoom->tags().contains(TestTag));
}

bool TestSuite::checkDirectChat() const
{
    return targetRoom->directChatUsers().contains(connection()->user());
}

TEST_IMPL(markDirectChat)
{
    if (checkDirectChat())
        connection()->removeFromDirectChats(targetRoom->id(),
                                            connection()->user());

    int id = qRegisterMetaType<DirectChatsMap>(); // For QSignalSpy
    Q_ASSERT(id != -1);

    // Same as with tags (and unusual for the rest of Quotient), direct chat
    // operations are synchronous.
    QSignalSpy spy(connection(), &Connection::directChatsListChanged);
    cout << "Marking the room as a direct chat" << endl;
    connection()->addToDirectChats(targetRoom, connection()->user());
    if (spy.count() != 1 || !checkDirectChat())
        FAIL_TEST();

    // Check that the first argument (added DCs) actually contains the room
    const auto& addedDCs = spy.back().front().value<DirectChatsMap>();
    if (addedDCs.size() != 1
        || !addedDCs.contains(connection()->user(), targetRoom->id())) {
        cout << "The room is not in added direct chats" << endl;
        FAIL_TEST();
    }

    cout << "Unmarking the direct chat" << endl;
    connection()->removeFromDirectChats(targetRoom->id(), connection()->user());
    if (spy.count() != 2 && checkDirectChat())
        FAIL_TEST();

    // Check that the second argument (removed DCs) actually contains the room
    const auto& removedDCs = spy.back().back().value<DirectChatsMap>();
    FINISH_TEST(removedDCs.size() == 1
                && removedDCs.contains(connection()->user(), targetRoom->id()));
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
        QByteArray failedList;
        for (const auto& f : failed)
            failedList += ' ' + f;
        plainReport += "\nFAILED:" + failedList;
        htmlReport += "<br><strong>Failed:</strong>" + failedList;
    }
    if (!running.empty()) {
        QByteArray dnfList;
        for (const auto& r : running)
            dnfList += ' ' + r;
        plainReport += "\nDID NOT FINISH:" + dnfList;
        htmlReport += "<br><strong>Did not finish:</strong>" + dnfList;
    }
    cout << plainReport.toStdString() << endl;

    // TODO: Waiting for proper futures to come so that it could be:
    //            targetRoom->postHtmlText(...)
    //            .then(this, &TestManager::finalize); // Qt-style or
    //            .then([this] { finalize(); }); // STL-style
    auto* room = testSuite->room();
    auto txnId = room->postHtmlText(plainReport, htmlReport);
    connect(room, &Room::messageSent, this,
            [this, room, txnId](const QString& serverTxnId) {
                if (txnId != serverTxnId)
                    return;

                cout << "Leaving the room" << endl;
                auto* job = room->leaveRoom();
                connect(job, &BaseJob::finished, this, [this, job] {
                    Q_ASSERT(job->status().good());
                    finalize();
                });
            });
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

#include "quotest.moc"
