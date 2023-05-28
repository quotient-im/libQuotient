// SPDX-FileCopyrightText: 2016 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include <Quotient/connection.h>
#include <Quotient/room.h>
#include <Quotient/user.h>
#include <Quotient/uriresolver.h>
#include <Quotient/networkaccessmanager.h>
#include <Quotient/qt_connection_util.h>

#include <Quotient/csapi/joining.h>
#include <Quotient/csapi/leaving.h>
#include <Quotient/csapi/room_send.h>

#include <Quotient/events/reactionevent.h>
#include <Quotient/events/redactionevent.h>
#include <Quotient/events/simplestateevents.h>
#include <Quotient/events/roommemberevent.h>

#include <QtTest/QSignalSpy>
#include <QtCore/QCoreApplication>
#include <QtCore/QFileInfo>
#include <QtCore/QStringBuilder>
#include <QtCore/QTemporaryFile>
#include <QtCore/QTimer>
#include <QtConcurrent/QtConcurrent>
#include <QtNetwork/QNetworkReply>

#include <functional>
#include <iostream>

using namespace Quotient;
using std::clog, std::endl;

class TestSuite;

class TestManager : public QCoreApplication {
public:
    TestManager(int& argc, char** argv);

private:
    void setupAndRun();
    void onNewRoom(Room* r);
    void doTests();
    void conclude();
    void finalize();

private:
    Connection* c = nullptr;
    QString origin;
    QString targetRoomName;
    TestSuite* testSuite = nullptr;
    QByteArrayList running {}, succeeded {}, failed {};
};

using TestToken = decltype(std::declval<QMetaMethod>().name());
Q_DECLARE_METATYPE(TestToken)

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
    TestSuite(Room* testRoom, QString source, TestManager* parent)
        : QObject(parent), targetRoom(testRoom), origin(std::move(source))
    {
        qRegisterMetaType<TestToken>();
        Q_ASSERT(testRoom && parent);
    }

signals:
    void finishedItem(QByteArray /*name*/, bool /*condition*/);

public slots:
    void doTest(const QByteArray& testName);

private slots:
    TEST_DECL(findRoomByAlias)
    TEST_DECL(loadMembers)
    TEST_DECL(sendMessage)
    TEST_DECL(sendReaction)
    TEST_DECL(sendFile)
    TEST_DECL(sendCustomEvent)
    TEST_DECL(setTopic)
    TEST_DECL(changeName)
    TEST_DECL(sendAndRedact)
    TEST_DECL(showLocalUsername)
    TEST_DECL(addAndRemoveTag)
    TEST_DECL(markDirectChat)
    TEST_DECL(visitResources)
    TEST_DECL(prettyPrintTests)
    // Add more tests above here

public:
    [[nodiscard]] Room* room() const { return targetRoom; }
    [[nodiscard]] Connection* connection() const
    {
        return targetRoom->connection();
    }

private:
    [[nodiscard]] bool checkFileSendingOutcome(const TestToken& thisTest,
                                               const QString& txnId,
                                               const QString& fileName);
    [[nodiscard]] bool testDownload(const TestToken& thisTest,
                                    const QUrl& mxcUrl);
    [[nodiscard]] bool checkRedactionOutcome(const QByteArray& thisTest,
                                             const QString& evtIdToRedact);

    template <EventClass<RoomEvent> EventT>
    [[nodiscard]] bool validatePendingEvent(const QString& txnId);
    [[nodiscard]] bool checkDirectChat() const;
    void finishTest(const TestToken& token, bool condition, const char* file,
                    int line);

private:
    Room* targetRoom;
    QString origin;
};

#define TEST_IMPL(Name) bool TestSuite::Name(const TestToken& thisTest)

// Returning true (rather than a void) allows to reuse the convention with
// connectUntil() to break the QMetaObject::Connection upon finishing the test
// item.
#define FINISH_TEST(Condition) \
    return (finishTest(thisTest, (Condition), __FILE__, __LINE__), true)

#define FAIL_TEST() FINISH_TEST(false)

void TestSuite::doTest(const QByteArray& testName)
{
    clog << "Starting: " << testName.constData() << endl;
    QMetaObject::invokeMethod(this, testName.constData(), Qt::DirectConnection,
                              Q_ARG(TestToken, testName));
}

template <EventClass<RoomEvent> EventT>
bool TestSuite::validatePendingEvent(const QString& txnId)
{
    auto it = targetRoom->findPendingEvent(txnId);
    return it != targetRoom->pendingEvents().end()
           && it->deliveryStatus() == EventStatus::Submitted
           && (*it)->transactionId() == txnId && is<EventT>(**it)
           && (*it)->matrixType() == EventT::TypeId;
}

void TestSuite::finishTest(const TestToken& token, bool condition,
                           const char* file, int line)
{
    const auto& item = testName(token);
    if (condition) {
        clog << item << " successful" << endl;
        if (targetRoom)
            targetRoom->postMessage(origin % ": "_ls % QString::fromUtf8(item) % " successful"_ls,
                                    MessageEventType::Notice);
    } else {
        clog << item << " FAILED at " << file << ":" << line << endl;
        if (targetRoom)
            targetRoom->postPlainText(origin % ": "_ls % QString::fromUtf8(item) % " FAILED at "_ls
                                      % QString::fromUtf8(file) % ", line "_ls % QString::number(line));
    }

    emit finishedItem(item, condition);
}

TestManager::TestManager(int& argc, char** argv)
    : QCoreApplication(argc, argv), c(new Connection(this))
{
    Q_ASSERT(argc >= 5);
    clog << "Connecting to Matrix as " << argv[1] << endl;
    c->loginWithPassword(QString::fromUtf8(argv[1]), QString::fromUtf8(argv[2]), QString::fromUtf8(argv[3]));
    targetRoomName = QString::fromUtf8(argv[4]);
    clog << "Test room name: " << argv[4] << endl;
    if (argc > 5) {
        origin = QString::fromUtf8(argv[5]);
        clog << "Origin for the test message: " << origin.toStdString() << endl;
    }

    connect(c, &Connection::connected, this, &TestManager::setupAndRun);
    connect(c, &Connection::resolveError, this,
        [](const QString& error) {
            clog << "Failed to resolve the server: " << error.toStdString()
                 << endl;
            QCoreApplication::exit(-2);
        },
        Qt::QueuedConnection);
    connect(c, &Connection::loginError, this,
        [this](const QString& message, const QString& details) {
            clog << "Failed to login to "
                 << c->homeserver().toDisplayString().toStdString() << ": "
                 << message.toStdString() << endl
                 << "Details:" << endl
                 << details.toStdString() << endl;
            QCoreApplication::exit(-2);
        },
        Qt::QueuedConnection);
    connect(c, &Connection::loadedRoomState, this, &TestManager::onNewRoom);

    // Big countdown watchdog
    QTimer::singleShot(180000, this, [this] {
        clog << "Time is up, stopping the session\n";
        if (testSuite)
            conclude();
        else
            finalize();
    });
}

void TestManager::setupAndRun()
{
    Q_ASSERT(!c->homeserver().isEmpty() && c->homeserver().isValid());
    Q_ASSERT(c->domain() == c->userId().section(u':', 1));
    clog << "Connected, server: "
         << c->homeserver().toDisplayString().toStdString() << endl;
    clog << "Access token: " << c->accessToken().toStdString() << endl;

    c->setLazyLoading(true);

    clog << "Joining " << targetRoomName.toStdString() << endl;
    auto joinJob = c->joinRoom(targetRoomName);
    // Ensure that the room has been joined and filled with some events
    // so that other tests could use that
    connect(joinJob, &BaseJob::success, this, [this, joinJob] {
        testSuite = new TestSuite(c->room(joinJob->roomId()), origin, this);
        // Only start the sync after joining, to make sure the room just
        // joined is in it
        c->syncLoop();
        connect(c, &Connection::syncDone, this, [this] {
            static int i = 0;
            clog << "Sync " << ++i << " complete" << endl;
            if (auto* r = testSuite->room()) {
                clog << "Test room timeline size = " << r->timelineSize();
                if (!r->pendingEvents().empty())
                    clog << ", pending size = " << r->pendingEvents().size();
                clog << endl;
            }
            if (!running.empty()) {
                clog << running.size() << " test(s) in the air:";
                for (const auto& test: qAsConst(running))
                    clog << " " << testName(test);
                clog << endl;
            }
            if (i == 1) {
                testSuite->room()->getPreviousContent();
                connectSingleShot(testSuite->room(), &Room::addedMessages, this,
                                  &TestManager::doTests);
            }
        });
    });
    connect(joinJob, &BaseJob::failure, this, [this] {
        clog << "Failed to join the test room" << endl;
        finalize();
    });
}

void TestManager::onNewRoom(Room* r)
{
    clog << "New room: " << r->id().toStdString() << endl
         << "  Name: " << r->name().toStdString() << endl
         << "  Canonical alias: " << r->canonicalAlias().toStdString() << endl
         << endl;
    connect(r, &Room::aboutToAddNewMessages, r, [r](RoomEventsRange timeline) {
        clog << timeline.size() << " new event(s) in room "
             << r->objectName().toStdString() << endl;
    });
}

void TestManager::doTests()
{
    const auto* metaObj = testSuite->metaObject();
    for (auto i = metaObj->methodOffset(); i < metaObj->methodCount(); ++i) {
        const auto metaMethod = metaObj->method(i);
        if (metaMethod.access() != QMetaMethod::Private
            || metaMethod.methodType() != QMetaMethod::Slot)
            continue;

        const auto testName = metaMethod.name();
        running.push_back(testName);
        // Some tests return the result immediately but we queue everything
        // and process all tests asynchronously.
        QMetaObject::invokeMethod(testSuite, "doTest", Qt::QueuedConnection,
                                  Q_ARG(QByteArray, testName));
    }
    clog << "Tests to do:";
    for (const auto& test: qAsConst(running))
        clog << " " << testName(test);
    clog << endl;
    connect(testSuite, &TestSuite::finishedItem, this,
            [this](const QByteArray& itemName, bool condition) {
                if (auto i = running.indexOf(itemName); i != -1)
                    (condition ? succeeded : failed).push_back(running.takeAt(i));
                else
                    Q_ASSERT_X(false, itemName.constData(),
                               "Test item is not in running state");
                if (running.empty()) {
                    clog << "All tests finished" << endl;
                    conclude();
                }
            });
}

TEST_IMPL(findRoomByAlias)
{
    auto* roomByAlias = connection()->roomByAlias(targetRoom->canonicalAlias(),
                                        JoinState::Join);
    FINISH_TEST(roomByAlias == targetRoom);
}

TEST_IMPL(loadMembers)
{
    // It's not exactly correct because an arbitrary server might not support
    // lazy loading; but in the absence of capabilities framework we assume
    // it does.
    if (targetRoom->users().size() >= targetRoom->joinedCount()) {
        clog << "Lazy loading doesn't seem to be enabled" << endl;
        FAIL_TEST();
    }
    targetRoom->setDisplayed();
    connect(targetRoom, &Room::allMembersLoaded, this, [this, thisTest] {
        FINISH_TEST(targetRoom->users().size() >= targetRoom->joinedCount());
    });
    return false;
}

TEST_IMPL(sendMessage)
{
    auto txnId = targetRoom->postPlainText("Hello, "_ls % origin % " is here"_ls);
    if (!validatePendingEvent<RoomMessageEvent>(txnId)) {
        clog << "Invalid pending event right after submitting" << endl;
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
    clog << "Reacting to the newest message in the room" << endl;
    Q_ASSERT(targetRoom->timelineSize() > 0);
    const auto targetEvtId = targetRoom->messageEvents().back()->id();

    // TODO: a separate test unit for reactionevent.h
    if (loadEvent<ReactionEvent>(RoomEvent::basicJson(
            ReactionEvent::TypeId,
            { { RelatesToKey, toJson(EventRelation::replace(targetEvtId)) } }))) {
        clog << "ReactionEvent can be created with an invalid relation type"
             << endl;
        FAIL_TEST();
    }

    const auto key = QStringLiteral("+1");
    const auto txnId = targetRoom->postReaction(targetEvtId, key);
    if (!validatePendingEvent<ReactionEvent>(txnId)) {
        clog << "Invalid pending event right after submitting" << endl;
        FAIL_TEST();
    }

    connectUntil(targetRoom, &Room::updatedEvent, this,
        [this, thisTest, txnId, key, targetEvtId](const QString& actualTargetEvtId) {
            if (actualTargetEvtId != targetEvtId)
                return false;
            const auto reactions = targetRoom->relatedEvents(
                targetEvtId, EventRelation::AnnotationType);
            // It's a test room, assuming no interference there should
            // be exactly one reaction
            if (reactions.size() != 1)
                FAIL_TEST();

            const auto* evt =
                eventCast<const ReactionEvent>(reactions.back());
            FINISH_TEST(is<ReactionEvent>(*evt) && !evt->id().isEmpty()
                        && evt->key() == key && evt->transactionId() == txnId);
            // TODO: Test removing the reaction
        });
    return false;
}

static constexpr auto fileContent = "Test";

TEST_IMPL(sendFile)
{
    auto* tf = new QTemporaryFile;
    if (!tf->open()) {
        clog << "Failed to create a temporary file" << endl;
        FAIL_TEST();
    }
    tf->write(fileContent);
    tf->close();
    QFileInfo tfi { *tf };
    // QFileInfo::fileName brings only the file name; QFile::fileName brings
    // the full path
    const auto tfName = tfi.fileName();
    clog << "Sending file " << tfName.toStdString() << endl;
    const auto txnId = targetRoom->postFile(
        "Test file"_ls, new EventContent::FileContent(tfi));
    if (!validatePendingEvent<RoomMessageEvent>(txnId)) {
        clog << "Invalid pending event right after submitting" << endl;
        tf->deleteLater();
        FAIL_TEST();
    }

    // Using tf as a context object to clean away both connections
    // once either of them triggers.
    connectUntil(targetRoom, &Room::fileTransferCompleted, tf,
        [this, thisTest, txnId, tf, tfName](const QString& id) {
            auto fti = targetRoom->fileTransferInfo(id);
            Q_ASSERT(fti.status == FileTransferInfo::Completed);

            if (id != txnId)
                return false;

            tf->deleteLater();
            return checkFileSendingOutcome(thisTest, txnId, tfName);
        });
    connectUntil(targetRoom, &Room::fileTransferFailed, tf,
        [this, thisTest, txnId, tf](const QString& id, const QString& error) {
            if (id != txnId)
                return false;

            targetRoom->postPlainText(origin % ": File upload failed: "_ls % error);
            tf->deleteLater();
            FAIL_TEST();
        });
    return false;
}

// Can be replaced with a lambda once QtConcurrent is able to resolve return
// types from lambda invocations (Qt 6 can, not sure about earlier)
struct DownloadRunner {
    QUrl url;

    using result_type = QNetworkReply::NetworkError;

    result_type operator()(int) const
    {
        QEventLoop el;
        const QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> reply {
            NetworkAccessManager::instance()->get(QNetworkRequest(url))
        };
        QObject::connect(
            reply.data(), &QNetworkReply::finished, &el, [&el] { el.exit(); },
            Qt::QueuedConnection);
        el.exec();
        return reply->error() != QNetworkReply::NoError ? reply->error()
               : reply->readAll() != fileContent
                   ? QNetworkReply::UnknownContentError
                   : QNetworkReply::NoError;
    }

    static QVector<result_type> run(const QUrl& url, int threads)
    {
        return QtConcurrent::blockingMapped(QVector<int>(threads),
                                            DownloadRunner{ url });
    }
};

bool TestSuite::testDownload(const TestToken& thisTest, const QUrl& mxcUrl)
{
    // Testing direct media requests needs explicit allowance
    QSettings s;
    static constexpr auto DirectMediaRequestsSetting =
        "Network/allow_direct_media_requests"_ls;
    s.setValue(DirectMediaRequestsSetting, true);
    if (const auto result = DownloadRunner::run(mxcUrl, 1);
        result.front() != QNetworkReply::NoError) {
        clog << "Direct media request to "
             << mxcUrl.toDisplayString().toStdString()
             << " was allowed but failed" << endl;
        FAIL_TEST();
    }
    s.setValue(DirectMediaRequestsSetting, false);
    if (const auto result = DownloadRunner::run(mxcUrl, 1);
        result.front() == QNetworkReply::NoError) {
        clog << "Direct media request to "
             << mxcUrl.toDisplayString().toStdString()
             << " was disallowed but succeeded" << endl;
        FAIL_TEST();
    }

    static constexpr auto ThreadsCount = 3;
    const auto httpUrl = targetRoom->connection()->makeMediaUrl(mxcUrl);
    const auto results = DownloadRunner::run(httpUrl, ThreadsCount);
    // Move out actual test from the multithreaded code to help debugging
    // NB: remove explicit template argument once entirely at Qt 6 or C++23
    FINISH_TEST(results
                == QVector<QNetworkReply::NetworkError>(ThreadsCount,
                                                        QNetworkReply::NoError));
}

bool TestSuite::checkFileSendingOutcome(const TestToken& thisTest,
                                        const QString& txnId,
                                        const QString& fileName)
{
    auto it = targetRoom->findPendingEvent(txnId);
    if (it == targetRoom->pendingEvents().end()) {
        clog << "Pending file event dropped before upload completion" << endl;
        FAIL_TEST();
    }
    if (it->deliveryStatus() != EventStatus::FileUploaded) {
        clog << "Pending file event status upon upload completion is "
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

            clog << "File event " << txnId.toStdString()
                 << " arrived in the timeline" << endl;
            // This part tests switchOnType()
            return switchOnType(
                *evt,
                [&](const RoomMessageEvent& e) {
                    // TODO: check #366 once #368 is implemented
                    if (e.id().isEmpty()
                        || pendingEvents[size_t(pendingIdx)]->transactionId()
                               != txnId
                        || !e.hasFileContent()
                        || e.content()->fileInfo()->originalName != fileName) {
                        clog << "Malformed file event";
                        FAIL_TEST();
                    }
                    return testDownload(thisTest, e.content()->fileInfo()->url());
                },
                [this, thisTest](const RoomEvent&) { FAIL_TEST(); });
        });
    return true;
}

DEFINE_SIMPLE_EVENT(CustomEvent, RoomEvent, "quotest.custom", int, testValue,
                    "test_value")

TEST_IMPL(sendCustomEvent)
{
    auto txnId = targetRoom->postEvent(new CustomEvent(42));
    if (!validatePendingEvent<CustomEvent>(txnId)) {
        clog << "Invalid pending event right after submitting" << endl;
        FAIL_TEST();
    }
    connectUntil(
        targetRoom, &Room::pendingEventAboutToMerge, this,
        [this, thisTest, txnId](const RoomEvent* evt, int pendingIdx) {
            const auto& pendingEvents = targetRoom->pendingEvents();
            Q_ASSERT(pendingIdx >= 0 && pendingIdx < int(pendingEvents.size()));

            if (evt->transactionId() != txnId)
                return false;

            return switchOnType(*evt,
                [this, thisTest, &evt](const CustomEvent& e) {
                    FINISH_TEST(!evt->id().isEmpty() && e.testValue() == 42);
                },
                [this, thisTest] (const RoomEvent&) { FAIL_TEST(); });
        });
    return false;

}

TEST_IMPL(setTopic)
{
    const auto newTopic = QString::fromLatin1(connection()->generateTxnId()); // Just a way to make
                                                         // a unique id
    targetRoom->setTopic(newTopic);
    connectUntil(targetRoom, &Room::topicChanged, this,
        [this, thisTest, newTopic] {
            if (targetRoom->topic() == newTopic)
                FINISH_TEST(true);

            clog << "Requested topic was " << newTopic.toStdString() << ", "
                 << targetRoom->topic().toStdString() << " arrived instead"
                 << endl;
            return false;
        });
    return false;
}

TEST_IMPL(changeName)
{
    connectSingleShot(targetRoom, &Room::allMembersLoaded, this, [this, thisTest] {
        auto* const localUser = connection()->user();
        const auto& newName = QString::fromLatin1(connection()->generateTxnId()); // See setTopic()
        clog << "Renaming the user to " << newName.toStdString()
             << " in the target room" << endl;
        localUser->rename(newName, targetRoom);
        connectUntil(
            targetRoom, &Room::aboutToAddNewMessages, this,
            [this, thisTest, localUser, newName](const RoomEventsRange& evts) {
                for (const auto& e : evts) {
                    if (const auto* rme = eventCast<const RoomMemberEvent>(e)) {
                        if (rme->stateKey() != localUser->id()
                            || !rme->isRename())
                            continue;
                        if (!rme->newDisplayName()
                            || *rme->newDisplayName() != newName)
                            FAIL_TEST();
                        // State events coming in the timeline are first
                        // processed to change the room state and then as
                        // timeline messages; aboutToAddNewMessages is triggered
                        // when the state is already updated, so check that
                        if (targetRoom->currentState().get<RoomMemberEvent>(
                                localUser->id())
                            != rme)
                            FAIL_TEST();
                        clog << "Member rename successful, renaming the account"
                             << endl;
                        const auto newN = newName.mid(0, 5);
                        localUser->rename(newN);
                        connectUntil(localUser, &User::defaultNameChanged, this,
                                     [this, thisTest, localUser, newN] {
                                         targetRoom->localUser()->rename({});
                                         FINISH_TEST(localUser->name() == newN);
                                     });
                        return true;
                    }
                }
                return false;
            });
    });
    return false;
}

TEST_IMPL(showLocalUsername)
{
    auto* const localUser = connection()->user();
    FINISH_TEST(!localUser->name().contains("@"_ls));
}

TEST_IMPL(sendAndRedact)
{
    clog << "Sending a message to redact" << endl;
    auto txnId = targetRoom->postPlainText(origin % ": message to redact"_ls);
    if (txnId.isEmpty())
        FAIL_TEST();

    connectUntil(targetRoom, &Room::messageSent, this,
            [this, thisTest, txnId](const QString& tId, const QString& evtId) {
                if (tId != txnId)
                    return false;

                // The event may end up having been merged, and that's ok;
                // but if it's not, it has to be in the ReachedServer state.
                if (auto it = room()->findPendingEvent(tId);
                    it != room()->pendingEvents().cend()
                    && it->deliveryStatus() != EventStatus::ReachedServer) {
                    clog << "Incorrect sent event status ("
                         << it->deliveryStatus() << ')' << endl;
                    FAIL_TEST();
                }

                clog << "Redacting the message" << endl;
                targetRoom->redactEvent(evtId, origin);
                connectUntil(targetRoom, &Room::addedMessages, this,
                             [this, thisTest, evtId] {
                                 return checkRedactionOutcome(thisTest, evtId);
                             });
                return false;
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
    if (it == targetRoom->historyEdge())
        return false; // Waiting for the next sync

    if ((*it)->isRedacted()) {
        clog << "The sync brought already redacted message" << endl;
        FINISH_TEST(true);
    }

    clog << "Message came non-redacted with the sync, waiting for redaction"
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
        clog << "Tag adding failed" << endl;
        FAIL_TEST();
    }
    clog << "Test tag set, removing it now" << endl;
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
    clog << "Marking the room as a direct chat" << endl;
    connection()->addToDirectChats(targetRoom, connection()->user());
    if (spy.count() != 1 || !checkDirectChat())
        FAIL_TEST();

    // Check that the first argument (added DCs) actually contains the room
    const auto& addedDCs = spy.back().front().value<DirectChatsMap>();
    if (addedDCs.size() != 1
        || !addedDCs.contains(connection()->user(), targetRoom->id())) {
        clog << "The room is not in added direct chats" << endl;
        FAIL_TEST();
    }

    clog << "Unmarking the direct chat" << endl;
    connection()->removeFromDirectChats(targetRoom->id(), connection()->user());
    if (spy.count() != 2 && checkDirectChat())
        FAIL_TEST();

    // Check that the second argument (removed DCs) actually contains the room
    const auto& removedDCs = spy.back().back().value<DirectChatsMap>();
    FINISH_TEST(removedDCs.size() == 1
                && removedDCs.contains(connection()->user(), targetRoom->id()));
}

TEST_IMPL(visitResources)
{
    // Same as the two tests above, ResourceResolver emits signals
    // synchronously so we use signal spies to intercept them instead of
    // connecting lambdas before calling openResource(). NB: this test
    // assumes that ResourceResolver::openResource is implemented in terms
    // of ResourceResolver::visitResource, so the latter doesn't need a
    // separate test.
    static UriDispatcher ud;

    // This lambda returns true in case of error, false if it's fine so far
    auto testResourceResolver = [this, thisTest](const QStringList& uris,
                                                 auto signal, auto* target,
                                                 QVariantList otherArgs = {}) {
        int r = qRegisterMetaType<decltype(target)>();
        Q_ASSERT(r != 0);
        QSignalSpy spy(&ud, signal);
        for (const auto& uriString: uris) {
            Uri uri { uriString };
            clog << "Checking " << uriString.toStdString()
                 << " -> " << uri.toDisplayString().toStdString() << endl;
            if (auto matrixToUrl = uri.toUrl(Uri::MatrixToUri).toDisplayString();
                !matrixToUrl.startsWith("https://matrix.to/#/"_ls)) {
                clog << "Incorrect matrix.to representation:"
                     << matrixToUrl.toStdString() << endl;
            }
            const auto checkResult = checkResource(connection(), uriString);
            if ((checkResult != UriResolved && uri.type() != Uri::NonMatrix)
                || (uri.type() == Uri::NonMatrix
                    && checkResult != CouldNotResolve)) {
                clog << "checkResource() returned incorrect result:"
                     << checkResult;
                FAIL_TEST();
            }
            ud.visitResource(connection(), uriString);
            if (spy.count() != 1) {
                clog << "Wrong number of signal emissions (" << spy.count()
                     << ')' << endl;
                FAIL_TEST();
            }
            const auto& emission = spy.front();
            Q_ASSERT(emission.count() >= 2);
            if (emission.front().value<decltype(target)>() != target) {
                clog << "Signal emitted with an incorrect target" << endl;
                FAIL_TEST();
            }
            if (!otherArgs.empty()) {
                if (emission.size() < otherArgs.size() + 1) {
                    clog << "Emission doesn't include all arguments" << endl;
                    FAIL_TEST();
                }
                for (auto i = 0; i < otherArgs.size(); ++i)
                    if (otherArgs[i] != emission[i + 1]) {
                        clog << "Mismatch in argument #" << i + 1 << endl;
                        FAIL_TEST();
                    }
            }
            spy.clear();
        }
        return false;
    };

    // Basic tests
    for (const auto& u: { Uri {}, Uri { QUrl {} } })
        if (u.isValid() || !u.isEmpty()) {
            clog << "Empty Matrix URI test failed" << endl;
            FAIL_TEST();
        }
    if (Uri { QStringLiteral("#") }.isValid()) {
        clog << "Bare sigil URI test failed" << endl;
        FAIL_TEST();
    }
    QUrl invalidUrl { "https://"_ls };
    invalidUrl.setAuthority("---:@@@"_ls);
    const Uri matrixUriFromInvalidUrl { invalidUrl },
        invalidMatrixUri { QStringLiteral("matrix:&invalid@") };
    if (matrixUriFromInvalidUrl.isEmpty() || matrixUriFromInvalidUrl.isValid()) {
        clog << "Invalid Matrix URI test failed" << endl;
        FAIL_TEST();
    }
    if (invalidMatrixUri.isEmpty() || invalidMatrixUri.isValid()) {
        clog << "Invalid sigil in a Matrix URI - test failed" << endl;
        FAIL_TEST();
    }

    // Matrix identifiers used throughout all URI tests
    const auto& roomId = room()->id();
    const auto& roomAlias = room()->canonicalAlias();
    const auto& userId = connection()->userId();
    const auto& eventId = room()->messageEvents().back()->id();
    Q_ASSERT(!roomId.isEmpty());
    Q_ASSERT(!roomAlias.isEmpty());
    Q_ASSERT(!userId.isEmpty());
    Q_ASSERT(!eventId.isEmpty());

    const QStringList roomUris {
        roomId, "matrix:roomid/"_ls + roomId.mid(1),
        "https://matrix.to/#/%21"_ls/*`!`*/ + roomId.mid(1),
        roomAlias, "matrix:room/"_ls + roomAlias.mid(1),
        "matrix:r/"_ls + roomAlias.mid(1),
        "https://matrix.to/#/"_ls + roomAlias,
    };
    const QStringList userUris { userId, "matrix:user/"_ls + userId.mid(1),
                                 "matrix:u/"_ls + userId.mid(1),
                                 "https://matrix.to/#/"_ls + userId };
    const QStringList eventUris {
        "matrix:room/"_ls + roomAlias.mid(1) + "/event/"_ls + eventId.mid(1),
        "matrix:r/"_ls + roomAlias.mid(1) + "/e/"_ls + eventId.mid(1),
        "https://matrix.to/#/"_ls + roomId + u'/' + eventId
    };
    // Check that reserved characters are correctly processed.
    static const auto& joinRoomAlias =
        QStringLiteral("##/?.@\"unjoined:example.org");
    static const auto& encodedRoomAliasNoSigil =
        QString::fromLatin1(QUrl::toPercentEncoding(joinRoomAlias.mid(1), QByteArrayLiteral(":")));
    static const QString joinQuery { "?action=join"_ls };
    // These URIs are not supposed to be actually joined (and even exist,
    // as yet) - only to be syntactically correct
    static const QStringList joinByAliasUris {
        Uri(joinRoomAlias.toUtf8(), {}, joinQuery.mid(1)).toDisplayString(),
        "matrix:room/"_ls + encodedRoomAliasNoSigil + joinQuery,
        "matrix:r/"_ls + encodedRoomAliasNoSigil + joinQuery,
        "https://matrix.to/#/%23"_ls/*`#`*/ + encodedRoomAliasNoSigil + joinQuery,
        "https://matrix.to/#/%23"_ls + joinRoomAlias.mid(1) /* unencoded */ + joinQuery
    };
    static const auto& joinRoomId = QStringLiteral("!anyid:example.org");
    static const QStringList viaServers { "matrix.org"_ls, "example.org"_ls };
    static const auto viaQuery =
        std::accumulate(viaServers.cbegin(), viaServers.cend(), joinQuery,
                        [](const QString& q, const QString& s) {
                            return q + "&via="_ls + s;
                        });
    static const QStringList joinByIdUris {
        "matrix:roomid/"_ls + joinRoomId.mid(1) + viaQuery,
        "https://matrix.to/#/"_ls + joinRoomId + viaQuery
    };
    // If any test breaks, the breaking call will return true, and further
    // execution will be cut by ||'s short-circuiting
    if (testResourceResolver(roomUris, &UriDispatcher::roomAction, room())
        || testResourceResolver(userUris, &UriDispatcher::userAction,
                                connection()->user())
        || testResourceResolver(eventUris, &UriDispatcher::roomAction,
                                room(), { eventId })
        || testResourceResolver(joinByAliasUris, &UriDispatcher::joinAction,
                                connection(), { joinRoomAlias })
        || testResourceResolver(joinByIdUris, &UriDispatcher::joinAction,
                                connection(), { joinRoomId, viaServers }))
        return true;
    // TODO: negative cases
    FINISH_TEST(true);
}

bool checkPrettyPrint(
    std::initializer_list<std::pair<const char*, const char*>> tests)
{
    bool result = true;
    for (const auto& [test, etalon] : tests) {
        const auto is = prettyPrint(QString::fromUtf8(test)).toStdString();
        const auto shouldBe = std::string("<span style='white-space:pre-wrap'>")
                              + etalon + "</span>";
        if (is == shouldBe)
            continue;
        clog << is << " != " << shouldBe << endl;
        result = false;
    }
    return result;
}

TEST_IMPL(prettyPrintTests)
{
    const bool prettyPrintTestResult = checkPrettyPrint(
        { { "https://www.matrix.org",
            R"(<a href="https://www.matrix.org">https://www.matrix.org</a>)" },
//          { "www.matrix.org", // Doesn't work yet
//             R"(<a href="https://www.matrix.org">www.matrix.org</a>)" },
          { "smb://somewhere/file", "smb://somewhere/file" }, // Disallowed scheme
          { "https:/something", "https:/something" }, // Malformed URL
          { "https://matrix.to/#/!roomid:example.org",
            R"(<a href="https://matrix.to/#/!roomid:example.org">https://matrix.to/#/!roomid:example.org</a>)" },
          { "https://matrix.to/#/@user_id:example.org",
            R"(<a href="https://matrix.to/#/@user_id:example.org">https://matrix.to/#/@user_id:example.org</a>)" },
          { "https://matrix.to/#/#roomalias:example.org",
            R"(<a href="https://matrix.to/#/#roomalias:example.org">https://matrix.to/#/#roomalias:example.org</a>)" },
          { "https://matrix.to/#/##ircroomalias:example.org",
            R"(<a href="https://matrix.to/#/##ircroomalias:example.org">https://matrix.to/#/##ircroomalias:example.org</a>)" },
          { "me@example.org",
            R"(<a href="mailto:me@example.org">me@example.org</a>)" },
          { "mailto:me@example.org",
            R"(<a href="mailto:me@example.org">mailto:me@example.org</a>)" },
          { "!room_id:example.org",
            R"(<a href="https://matrix.to/#/!room_id:example.org">!room_id:example.org</a>)" },
          { "@user_id:example.org",
            R"(<a href="https://matrix.to/#/@user_id:example.org">@user_id:example.org</a>)" },
          { "#room_alias:example.org",
            R"(<a href="https://matrix.to/#/#room_alias:example.org">#room_alias:example.org</a>)" } });
    FINISH_TEST(prettyPrintTestResult);
}

void TestManager::conclude()
{
    // Clean up the room (best effort)
    auto* room = testSuite->room();
    room->setTopic({});
    room->localUser()->rename({});

    QString succeededRec { QString::number(succeeded.size()) % " of "_ls
                           % QString::number(succeeded.size() + failed.size()
                                             + running.size())
                           % " tests succeeded"_ls };
    QString plainReport = origin % ": Testing complete, "_ls % succeededRec;
    QString color = failed.empty() && running.empty() ? "00AA00"_ls : "AA0000"_ls;
    QString htmlReport = origin % ": <strong><font data-mx-color='#"_ls % color
                         % "' color='#"_ls % color
                         % "'>Testing complete</font></strong>, "_ls % succeededRec;
    if (!failed.empty()) {
        QByteArray failedList;
        for (const auto& f : qAsConst(failed))
            failedList += ' ' + f;
        plainReport += "\nFAILED:"_ls + QString::fromUtf8(failedList);
        htmlReport += "<br><strong>Failed:</strong>"_ls + QString::fromUtf8(failedList);
    }
    if (!running.empty()) {
        QByteArray dnfList;
        for (const auto& r : qAsConst(running))
            dnfList += ' ' + r;
        plainReport += "\nDID NOT FINISH:"_ls + QString::fromUtf8(dnfList);
        htmlReport += "<br><strong>Did not finish:</strong>"_ls + QString::fromUtf8(dnfList);
    }

    auto txnId = room->postHtmlText(plainReport, htmlReport);
    // Now just wait until all the pending events reach the server
    connectUntil(room, &Room::messageSent, this,
        [this, txnId, room, plainReport] (const QString& sentTxnId) {
            if (sentTxnId != txnId)
                return false;
            const auto& pendingEvents = room->pendingEvents();
            if (auto c = std::count_if(pendingEvents.cbegin(),
                                       pendingEvents.cend(),
                                       [](const PendingEventItem& pe) {
                                           return pe.deliveryStatus()
                                                  < EventStatus::ReachedServer;
                                       });
                c > 0) {
                clog << "Events to reach the server: " << c
                     << ", not leaving yet" << endl;
                return false;
            }

            clog << "Leaving the room" << endl;
            // TODO: Waiting for proper futures to come so that it could be:
//           room->leaveRoom()
//           .then(this, &TestManager::finalize); // Qt-style or
//           .then([this] { finalize(); }); // STL-style
            auto* job = room->leaveRoom();
            connect(job, &BaseJob::result, this, [this, job,plainReport] {
                Q_ASSERT(job->status().good());
                finalize();
                // Still flying, as the exit() connection in finalize() is queued
                clog << plainReport.toStdString() << endl;
            });
            return true;
        });
}

void TestManager::finalize()
{
    if (!c->isUsable() || !c->isLoggedIn()) {
        clog << "No usable connection reached" << endl;
        QCoreApplication::exit(-2);
        return; // NB: QCoreApplication::exit() does return to the caller
    }
    clog << "Logging out" << endl;
    c->logout();
    connect(
        c, &Connection::loggedOut, this,
        [this] {
            QCoreApplication::exit(!testSuite ? -3
                                   : succeeded.empty() && failed.empty()
                                           && running.empty()
                                       ? -4
                                       : failed.size() + running.size());
        },
        Qt::QueuedConnection);
}

int main(int argc, char* argv[])
{
    // TODO: use QCommandLineParser
    if (argc < 5) {
        clog << "Usage: quotest <user> <passwd> <device_name> <room_alias> [origin]"
             << endl;
        return -1;
    }
    // NOLINTNEXTLINE(readability-static-accessed-through-instance)
    return TestManager(argc, argv).exec();
}

#include "quotest.moc"
