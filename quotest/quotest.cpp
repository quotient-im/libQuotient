// SPDX-FileCopyrightText: 2016 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include <Quotient/connection.h>
#include <Quotient/room.h>
#include <Quotient/settings.h>
#include <Quotient/thread.h>
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
#include <Quotient/events/stickerevent.h>

#include <QtTest/QSignalSpy>
#include <QtCore/QCoreApplication>
#include <QtCore/QFileInfo>
#include <QtCore/QStringBuilder>
#include <QtCore/QTemporaryFile>
#include <QtCore/QTimer>
#include <QtConcurrent/QtConcurrent>
#include <QtNetwork/QNetworkReply>

using namespace Quotient;

class TestSuite;

class TestManager : public QCoreApplication {
public:
    TestManager(int& argc, char** argv);

private:
    void setupAndRun(const QString &targetRoomAlias);
    void onNewRoom(Room* r);
    void doTests();
    void conclude();
    void finalize(const QString& lastWords = {});

private:
    Connection* c = nullptr;
    QString origin;
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
    TEST_DECL(loadCustomEvent)
    TEST_DECL(sendCustomEvent)
    TEST_DECL(setTopic)
    TEST_DECL(redactEvent)
    TEST_DECL(changeName)
    TEST_DECL(showLocalUsername)
    TEST_DECL(addAndRemoveTag)
    TEST_DECL(markDirectChat)
    TEST_DECL(visitResources)
    TEST_DECL(thread)
    TEST_DECL(reply)
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

    template <EventClass<RoomEvent> EventT>
    [[nodiscard]] bool validatePendingEvent(const QString& txnId);
    [[nodiscard]] bool checkDirectChat() const;
    void finishTest(const TestToken& token, bool condition,
                    std::source_location loc = std::source_location::current());

private:
    Room* targetRoom;
    QString origin;
};

#define TEST_IMPL(Name) bool TestSuite::Name(const TestToken& thisTest)

// Returning true (rather than a void) allows to reuse the convention with
// connectUntil() to break the QMetaObject::Connection upon finishing the test
// item.
#define FINISH_TEST(Condition) return (finishTest(thisTest, (Condition)), true)

#define FINISH_TEST_IF(Condition) \
    do {                          \
        if (Condition)            \
            FINISH_TEST(true);    \
    } while (false)

#define FAIL_TEST() FINISH_TEST(false)

#define FAIL_TEST_IF(Condition, ...)                         \
    do {                                                     \
        if (Condition) {                                     \
            __VA_OPT__(qWarning() << QUO_CSTR(__VA_ARGS__);) \
            FAIL_TEST();                                     \
        }                                                    \
    } while (false)

void TestSuite::doTest(const QByteArray& testName)
{
    qInfo() << "Starting:" << testName.constData();
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

void TestSuite::finishTest(const TestToken& token, bool condition, std::source_location loc)
{
    const auto& item = testName(token);
    if (condition) {
        qInfo() << item << "successful";
        if (targetRoom)
            targetRoom->postText<MessageEventType::Notice>(origin % ": "_L1 % QString::fromUtf8(item) % " successful"_L1);
    } else {
        qWarning().nospace() << item << " FAILED at " << loc.file_name() << ":" << loc.line();
        if (targetRoom)
            targetRoom->postText(origin % ": "_L1 % QString::fromUtf8(item) % " FAILED at "_L1
                                 % QString::fromUtf8(loc.file_name()) % ", line "_L1
                                 % QString::number(loc.line()));
    }

    emit finishedItem(item, condition);
}

inline void logConnectionDetails(Connection* c)
{
    qInfo() << "Connected to" << c->homeserver().toDisplayString();
    qInfo() << "Access token:" << c->accessToken();
}

using qsl_size_t = QStringList::size_type;

template <qsl_size_t From, qsl_size_t N>
inline std::array<QString, N> unpackList(const QStringList &l)
{
    return [&l]<qsl_size_t... Is>(std::integer_sequence<qsl_size_t, Is...>) {
        return std::to_array<QString>({l.size() > From + Is ? l[From + Is] : QString()...});
    }(std::make_integer_sequence<qsl_size_t, N>{});
}

TestManager::TestManager(int& argc, char** argv)
    : QCoreApplication(argc, argv), c(new Connection(this))
{
    setOrganizationName(u"Quotient"_s);
    setApplicationName(u"Quotest"_s);
    setApplicationVersion(versionString());

    // QCommandLineParser is of limited use here, as it cannot control for the number and format
    // of positional arguments; but at least it can show a nice help block
    const auto appDescription = u"Functional test suite for libQuotient"_s;
    QCommandLineParser clp;
    clp.setApplicationDescription(appDescription);
    clp.addHelpOption();
    clp.addVersionOption();
    clp.addPositionalArgument(u"user"_s, u"The user MXID that quotest will use to run tests"_s);
    clp.addPositionalArgument(u"password"_s, u"The password for the provided MXID"_s);
    clp.addPositionalArgument(u"device_name"_s, u"The device name to login with"_s);
    clp.addPositionalArgument(u"room_alias"_s, u"The alias of the room to run tests in"_s);
    clp.addPositionalArgument(u"origin"_s,
                              u"The invoker of the test or the conditions it is run in"_s,
                              u"origin"_s);
    clp.process(*this);
    const auto &positionalArgs = clp.positionalArguments();
    if (positionalArgs.size() < 5)
        clp.showHelp(EXIT_FAILURE);

    qInfo().noquote() << applicationName() << applicationVersion();
    const auto &[user, password, deviceName, targetRoomAlias] = unpackList<0, 4>(positionalArgs);
    origin = positionalArgs[4];
    qInfo().noquote() << "Connecting to Matrix as" << user;
    qInfo().noquote() << "Test room alias:" << targetRoomAlias;
    if (!origin.isEmpty())
        qInfo() << "Origin for the test message:" << origin;

    c->loginWithPassword(user, password, deviceName);
    connect(c, &Connection::connected, this, [this, targetRoomAlias] {
        if (QUO_ALARM(c->homeserver().isEmpty() || !c->homeserver().isValid())
            || QUO_ALARM(c->domain() != c->userId().section(u':', 1))) {
            qCritical() << "Connection information doesn't look right, "
                        << "check the parameters passed to quotest";
            exit(2);
            return;
        }
        logConnectionDetails(c);

        // Test Connection::assumeIdentity() while we can replace connection objects
        auto* newC = new Connection(c->homeserver(), this);
        newC->assumeIdentity(c->userId(), c->deviceId(), QString::fromLatin1(c->accessToken()));
        // NB: this will need to change when we switch E2EE on in quotest because encryption
        //     data is initialised asynchronously
        if (QUO_ALARM(newC->homeserver() != c->homeserver())
            || QUO_ALARM(newC->userId() != c->userId()) || QUO_ALARM(!newC->isLoggedIn())) {
            qCritical() << "Connection::assumeIdentity() is broken";
            exit(2);
            return;
        }

        c->deleteLater();
        c = newC;
        setupAndRun(targetRoomAlias);
    });
    connect(c, &Connection::resolveError, this,
        [](const QString& error) {
            qCritical() << "Could not start testing:" << error;
            exit(2);
        },
        Qt::QueuedConnection);
    connect(c, &Connection::loginError, this,
            [this](const QString &message, const QString &details) {
        qCritical().nospace() << "Failed to login to " << c->homeserver().toDisplayString() << ": "
                              << message;
        qWarning() << "Details:\n" << details;
        exit(2);
    }, Qt::QueuedConnection);

    // Big countdown watchdog
    QTimer::singleShot(180000, this, [this] {
        qWarning() << "Time is up, stopping the session";
        if (testSuite)
            conclude();
        else
            finalize();
    });
}

void listTests(const char *message, QByteArrayList testList)
{
    auto dbg = qInfo().noquote().nospace();
    dbg << testList.size() << ' ' << message << ':';
    dbg.space();
    for (const auto &test : std::as_const(testList))
        dbg << testName(test);
}

void TestManager::setupAndRun(const QString& targetRoomAlias)
{
    Q_ASSERT(!c->homeserver().isEmpty() && c->homeserver().isValid());
    Q_ASSERT(c->domain() == c->userId().section(u':', 1));
    logConnectionDetails(c);

    connect(c, &Connection::loadedRoomState, this, &TestManager::onNewRoom);

    c->setLazyLoading(true);

    qInfo() << "Joining" << targetRoomAlias;
    c->joinAndGetRoom(targetRoomAlias).then(this, [this](Room* room) {
        if (!room) {
            qCritical() << "Failed to join the test room";
            finalize();
            return;
        }
        // Ensure that the room has been joined and filled with some events
        // so that other tests could use that
        testSuite = new TestSuite(room, origin, this);
        // Only start the sync after joining, to make sure the room just
        // joined is in it
        c->syncLoop();
        connect(room, &Room::baseStateLoaded, this, [this, room] {
            room->getPreviousContent().then(this, &TestManager::doTests);
        }, Qt::SingleShotConnection);

        connect(room, &Room::changed, this, [room] {
            auto dbg = qInfo();
            dbg << "Test room timeline size =" << room->timelineSize();
            if (!room->pendingEvents().empty())
                dbg << ", pending size =" << room->pendingEvents().size();
        });
        connect(c, &Connection::syncDone, this, [this] {
            static int i = 0;
            qInfo() << "Sync" << ++i << "complete";
            if (!running.empty())
                listTests("test(s) in the air", running);
        });
    });
}

void TestManager::onNewRoom(Room* r)
{
    qDebug() << "New room:" << r->id();
    qDebug() << "  Name:" << r->name();
    qDebug() << "  Canonical alias: " << r->canonicalAlias();
    connect(r, &Room::aboutToAddNewMessages, r, [r](RoomEventsRange timeline) {
        qDebug() << timeline.size() << "new event(s) in room" << r->objectName();
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
    listTests("tests to do", running);
    connect(testSuite, &TestSuite::finishedItem, this,
            [this](const QByteArray& itemName, bool condition) {
                if (auto i = running.indexOf(itemName); i != -1)
                    (condition ? succeeded : failed).push_back(running.takeAt(i));
                else
                    Q_ASSERT_X(false, itemName.constData(),
                               "Test item is not in running state");
                if (running.empty()) {
                    qInfo() << "All tests finished";
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
    FAIL_TEST_IF(targetRoom->joinedMembers().size() >= targetRoom->joinedCount(),
                 "Lazy loading doesn't seem to be enabled");
    targetRoom->setDisplayed();
    connect(targetRoom, &Room::allMembersLoaded, this, [this, thisTest] {
        FINISH_TEST(targetRoom->joinedMembers().size() >= targetRoom->joinedCount());
    });
    return false;
}

TEST_IMPL(sendMessage)
{
    auto txnId = targetRoom->postText("Hello, "_L1 % origin % " is here"_L1);
    FAIL_TEST_IF(!validatePendingEvent<RoomMessageEvent>(txnId),
                 "Invalid pending event right after submitting");
    targetRoom->whenMessageMerged(txnId).then(this, [this, thisTest, txnId](const RoomEvent& evt) {
        const auto pendingIt = targetRoom->findPendingEvent(txnId);
        FAIL_TEST_IF(pendingIt == targetRoom->pendingEvents().end(),
                     "Pending event not found at the moment of local echo merging");
        FINISH_TEST(evt.is<RoomMessageEvent>() && !evt.id().isEmpty()
                    && txnId == (*pendingIt)->transactionId() && txnId == evt.transactionId());
    });
    return false;
}

TEST_IMPL(sendReaction)
{
    return targetRoom->post<RoomMessageEvent>(u"Reaction target"_s)
        .whenMerged()
        .then([this, thisTest](const RoomEvent& targetEvt) {
            const auto targetEvtId = targetEvt.id();
            qInfo() << "Reacting to the message just sent to the room" << targetEvtId;

            // TODO: a separate test unit for reactionevent.h
            FAIL_TEST_IF(loadEvent<ReactionEvent>(
                             ReactionEvent::TypeId,
                             QJsonObject{
                                 {RelatesToKey, toJson(EventRelation::replace(targetEvtId))}}),
                         "ReactionEvent can be created with an invalid relation type");

            const auto key = u"+"_s;
            const auto txnId = targetRoom->postReaction(targetEvtId, key);
            FAIL_TEST_IF(!validatePendingEvent<ReactionEvent>(txnId),
                         "Invalid pending event right after submitting");

            connectUntil(targetRoom, &Room::updatedEvent, this,
                         [this, thisTest, txnId, key, targetEvtId](const QString& actualTargetEvtId) {
                             if (actualTargetEvtId != targetEvtId)
                                 return false;
                             const auto reactions =
                                 targetRoom->relatedEvents(targetEvtId,
                                                           EventRelation::AnnotationType);
                             FAIL_TEST_IF(reactions.size() != 1);

                             const auto* evt = eventCast<const ReactionEvent>(reactions.back());
                             FAIL_TEST_IF(!is<ReactionEvent>(*evt) || evt->id().isEmpty()
                                         || evt->key() != key || evt->transactionId() != txnId,
                                         "Reaction event validation failed");

                             // Test the new RoomEvent relation API
                             const auto relation = evt->relatesTo();
                             FINISH_TEST(relation && relation->type == EventRelation::AnnotationType
                                        && relation->eventId == targetEvtId
                                        && relation->key == key);
                             // TODO: Test removing the reaction
                         });
            return false;
        })
        .isRunning();
}

TEST_IMPL(sendFile)
{
    auto* tf = new QTemporaryFile;
    FAIL_TEST_IF(!tf->open(), "Failed to create a temporary file");
    tf->write("Test");
    tf->close();
    const QFileInfo tfi { *tf };
    // QFileInfo::fileName brings only the file name; QFile::fileName brings
    // the full path
    const auto tfName = tfi.fileName();
    qInfo() << "Sending file" << tfName;
    const auto txnId = targetRoom->postFile(
        "Test file"_L1, std::make_unique<EventContent::FileContent>(tfi));
    if (!validatePendingEvent<RoomMessageEvent>(txnId)) {
        qWarning() << "Invalid pending event right after submitting";
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

            targetRoom->postText(origin % ": File upload failed: "_L1 % error);
            tf->deleteLater();
            FAIL_TEST();
        });
    return false;
}

using NetworkReplyPtr = QObjectHolder<QNetworkReply>;

void getResource(const QUrl& url, NetworkReplyPtr& r, QEventLoop& el)
{
    r.reset(NetworkAccessManager::instance()->get(QNetworkRequest(url)));
    QObject::connect(
        r.get(), &QNetworkReply::finished, &el,
        [url, &r, &el] {
            if (r->error() != QNetworkReply::NoError)
                getResource(url, r, el);
            else
                el.exit();
        },
        Qt::QueuedConnection);
}

bool testDownload(const QUrl& url)
{
    // The actual test is separate from the download invocation to help debugging
    const auto results = QtConcurrent::blockingMapped(QVector<int>{ 1, 2, 3 }, [url](int) {
        thread_local QEventLoop el;
        thread_local NetworkReplyPtr reply{};
        getResource(url, reply, el);
        el.exec();
        return reply->error();
    });
    return results == QVector<QNetworkReply::NetworkError>(3, QNetworkReply::NoError);
}

bool TestSuite::checkFileSendingOutcome(const TestToken& thisTest,
                                        const QString& txnId,
                                        const QString& fileName)
{
    auto it = targetRoom->findPendingEvent(txnId);
    FAIL_TEST_IF(it == targetRoom->pendingEvents().end(),
                 "Pending file event dropped before upload completion");
    if (it->deliveryStatus() != EventStatus::FileUploaded) {
        qWarning() << "Pending file event status upon upload completion is "
             << it->deliveryStatus() << " != FileUploaded("
             << EventStatus::FileUploaded << ')';
        FAIL_TEST();
    }

    targetRoom->whenMessageMerged(txnId).then(
        this, [this, thisTest, txnId, fileName](const RoomEvent& evt) {
            qInfo() << "File event" << txnId << "arrived in the timeline";
            using EventContent::FileContent;
            evt.switchOnType(
                [&](const RoomMessageEvent& e) {
                    // TODO: check #366 once #368 is implemented
                    FINISH_TEST(!e.id().isEmpty() && evt.transactionId() == txnId
                                && e.has<FileContent>()
                                && e.get<FileContent>()->originalName == fileName
                                && testDownload(targetRoom->connection()->makeMediaUrl(
                                    e.get<FileContent>()->url())));
                },
                [this, thisTest](const RoomEvent&) { FAIL_TEST(); });
        });
    return true;
}

QUO_DEFINE_SIMPLE_EVENT(CustomEvent, RoomEvent,
                        QUO_LIST("quotest.custom.unstable", "quotest.custom"), int, testValue,
                        "test_value")

TEST_IMPL(loadCustomEvent)
{
    // TODO: Make a unit test for event.*
    FAIL_TEST_IF(CustomEvent::MetaType.matrixIds.size() != 2);
    auto testEvent = loadEvent<RoomEvent>(CustomEvent::MetaType.matrixIds[1],
                                          QJsonObject{{"test_value"_L1, 17}});
    FINISH_TEST(testEvent && testEvent->is<CustomEvent>()
                && eventCast<CustomEvent>(testEvent)->testValue() == 17);
}

TEST_IMPL(sendCustomEvent)
{
    const auto& pendingEventItem = targetRoom->post<CustomEvent>(42);
    FAIL_TEST_IF(!validatePendingEvent<CustomEvent>(pendingEventItem->transactionId()),
                 "Invalid pending event right after submitting");
    pendingEventItem.whenMerged().then(
        this, [this, thisTest, txnId = pendingEventItem->transactionId()](const RoomEvent& evt) {
            evt.switchOnType(
                [this, thisTest, txnId, &evt](const CustomEvent& e) {
                    FINISH_TEST(!evt.id().isEmpty() && evt.transactionId() == txnId
                                && e.testValue() == 42);
                },
                [this, thisTest](const RoomEvent&) { FAIL_TEST(); });
        });
    return false;
}

TEST_IMPL(setTopic)
{
    const auto newTopic = connection()->generateTxnId(); // Just a way to make a unique id
    targetRoom->setTopic(newTopic);
    connectUntil(targetRoom, &Room::topicChanged, this, [this, thisTest, newTopic] {
        FINISH_TEST_IF(targetRoom->topic() == newTopic);

        qWarning() << "Requested topic was " << newTopic << ", " << targetRoom->topic()
                << " arrived instead";
        return false;
    });
    return false;
}

// TODO: maybe move it to Room?..
QFuture<void> ensureEvent(Room* room, const QString& evtId, QPromise<void>&& p = QPromise<void>{})
{
    auto future = p.future();
    if (room->findInTimeline(evtId) == room->historyEdge()) {
        qInfo() << "Loading a page of history, " << room->timelineSize() << " events so far";
        room->getPreviousContent().then(std::bind_front(ensureEvent, room, evtId, std::move(p)));
    } else
        p.finish();
    return future;
}

TEST_IMPL(redactEvent)
{
    using TargetEventType = RoomMemberEvent;

    // We use currentState() to quickly get an id of our own joining event,
    // to try to redact it. As long as the homeserver is compliant to the spec
    // nothing bad will happen upon an attempt to redact that member event,
    // the test user will remain a member of the room, while the library is
    // tested to implement MSC2176 correctly (see also our own bug #664).
    const auto* memberEventToRedact =
        targetRoom->currentState().get<TargetEventType>(connection()->userId());
    Q_ASSERT(memberEventToRedact); // ...or the room state is totally screwed
    const auto& evtId = memberEventToRedact->id();

    // Make sure the event is loaded in the timeline before proceeding with the test, to make sure
    // the replacement tracked below actually occurs
    ensureEvent(targetRoom, evtId).then([this, thisTest, evtId] {
        qInfo() << "Redacting the latest member event";
        targetRoom->redactEvent(evtId, origin);
        connectUntil(targetRoom, &Room::replacedEvent, this,
                     [this, thisTest, evtId](const RoomEvent* evt) {
                         // Concurrent replacement/redaction shouldn't happen as of now; but if/when
                         // event editing is added to the test suite, this may become a thing
                         if (evt->id() != evtId)
                             return false;
                         FINISH_TEST(evt->switchOnType([this](const TargetEventType& e) {
                             return e.redactionReason() == origin && e.membership() == Membership::Join;
                             // The second condition above tests MSC2176 - if it's violated (pre 0.8
                             // beta), membership() ends up being Membership::Undefined
                         }));
                     });
    });

    return false;
}

TEST_IMPL(changeName)
{
    // NB: this test races against redactEvent(); both update the same event
    // type and state key. In an extremely improbable case when changeName()
    // completes (with server roundtrips etc.) the first rename before
    // redactEvent() even starts, redactEvent() will capture the rename event
    // instead of the join, and likely break changeName() as a result.
    QtFuture::connect(targetRoom, &Room::allMembersLoaded).then([this, thisTest] {
        auto* const localUser = connection()->user();
        const auto& newName = connection()->generateTxnId(); // See setTopic()
        qInfo() << "Renaming the user to" << newName << "in the target room";
        localUser->rename(newName, targetRoom);
        connectUntil(
            targetRoom, &Room::aboutToAddNewMessages, this,
            [this, thisTest, localUser, newName](RoomEventsRange evts) {
                for (const auto& e : evts) {
                    if (const auto* rme = eventCast<const RoomMemberEvent>(e)) {
                        if (rme->stateKey() != localUser->id() || !rme->isRename())
                            continue;
                        FAIL_TEST_IF(!rme->newDisplayName() || *rme->newDisplayName() != newName);
                        // State events coming in the timeline are first
                        // processed to change the room state and then as
                        // timeline messages; aboutToAddNewMessages is triggered
                        // when the state is already updated, so check that
                        FAIL_TEST_IF(targetRoom->currentState().get<RoomMemberEvent>(localUser->id())
                                     != rme);
                        qInfo() << "Member rename successful, renaming the account";
                        const auto newN = newName.mid(0, 5);
                        localUser->rename(newN);
                        connectUntil(localUser, &User::defaultNameChanged, this,
                                     [this, thisTest, localUser, newN] {
                                         localUser->rename({});
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
    FINISH_TEST(!localUser->name().contains("@"_L1));
}

TEST_IMPL(addAndRemoveTag)
{
    static const auto TestTag = u"im.quotient.test"_s;
    // Pre-requisite
    if (targetRoom->tags().contains(TestTag))
        targetRoom->removeTag(TestTag);

    // Unlike for most of Quotient, tags are applied and tagsChanged is emitted
    // synchronously, with the server being notified async. The test checks
    // that the signal is emitted, not only that tags have changed; but there's
    // (currently) no way to check that the server has been correctly notified
    // of the tag change.
    const QSignalSpy spy(targetRoom, &Room::tagsChanged);
    targetRoom->addTag(TestTag);
    FAIL_TEST_IF(spy.size() != 1 || !targetRoom->tags().contains(TestTag), "Tag adding failed");
    const auto& tagsToRooms = connection()->tagsToRooms();
    FAIL_TEST_IF(!tagsToRooms.contains(TestTag) || !tagsToRooms[TestTag].contains(targetRoom),
                 "Tag adding succeeded but the connection doesn't know about it");
    qInfo() << "Test tag set, removing it now";
    targetRoom->removeTag(TestTag);
    FINISH_TEST(spy.size() == 2 && !targetRoom->tags().contains(TestTag));
}

bool TestSuite::checkDirectChat() const
{
    return targetRoom->directChatMembers().contains(targetRoom->member(connection()->user()->id()));
}

TEST_IMPL(markDirectChat)
{
    if (checkDirectChat())
        connection()->removeFromDirectChats(targetRoom->id(),
                                            connection()->user()->id());

    const auto id = qRegisterMetaType<DirectChatsMap>(); // For QSignalSpy
    Q_ASSERT(id != -1);

    // Same as with tags (and unusual for the rest of Quotient), direct chat
    // operations are synchronous.
    const QSignalSpy spy(connection(), &Connection::directChatsListChanged);
    qInfo() << "Marking the room as a direct chat";
    connection()->addToDirectChats(targetRoom, connection()->user()->id());
    FAIL_TEST_IF(spy.size() != 1 || !checkDirectChat());

    // Check that the first argument (added DCs) actually contains the room
    const auto& addedDCs = spy.back().front().value<DirectChatsMap>();
    FAIL_TEST_IF(addedDCs.size() != 1 || !addedDCs.contains(connection()->user(), targetRoom->id()),
                 "The room is not in added direct chats");

    qInfo() << "Unmarking the direct chat";
    connection()->removeFromDirectChats(targetRoom->id(), connection()->user()->id());
    FAIL_TEST_IF(spy.size() != 2 && checkDirectChat());

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
    const auto testResourceResolver = [this, thisTest](const QStringList& uris, auto signal,
                                                       auto* target, QVariantList otherArgs = {}) {
        const auto r = qRegisterMetaType<decltype(target)>();
        Q_ASSERT(r != 0);
        QSignalSpy spy(&ud, signal);
        for (const auto& uriString: uris) {
            const Uri uri { uriString };
            qInfo() << "Checking" << uriString << "->" << uri.toDisplayString();
            FAIL_TEST_IF(auto matrixToUrl = uri.toUrl(Uri::MatrixToUri).toDisplayString();
                         !matrixToUrl.startsWith("https://matrix.to/#/"_L1),
                         u"Incorrect matrix.to representation:" % matrixToUrl);

            const auto checkResult = checkResource(connection(), uriString);
            if ((checkResult != UriResolved && uri.type() != Uri::NonMatrix)
                || (uri.type() == Uri::NonMatrix && checkResult != CouldNotResolve)) {
                qWarning() << "checkResource() returned incorrect result:" << checkResult;
                FAIL_TEST();
            }
            ud.visitResource(connection(), uriString);
            if (spy.size() != 1) {
                qWarning() << "Wrong number of signal emissions (" << spy.size() << ')';
                FAIL_TEST();
            }
            const auto& emission = spy.front();
            Q_ASSERT(emission.size() >= 2);
            FAIL_TEST_IF(emission.front().value<decltype(target)>() != target,
                         "Signal emitted with an incorrect target");
            if (!otherArgs.empty()) {
                FAIL_TEST_IF(emission.size() < otherArgs.size() + 1,
                             "Emission doesn't include all arguments");
                for (auto i = 0; i < otherArgs.size(); ++i)
                    if (otherArgs[i] != emission[i + 1]) {
                        qWarning() << "Mismatch in argument #" << i + 1;
                        FAIL_TEST();
                    }
            }
            spy.clear();
        }
        return false;
    };

    // Basic tests
    for (const auto& u: { Uri {}, Uri { QUrl {} } })
        FAIL_TEST_IF(u.isValid() || !u.isEmpty(), "Empty Matrix URI test failed");
    FAIL_TEST_IF(Uri { u"#"_s }.isValid(), "Bare sigil URI test failed");
    QUrl invalidUrl { "https://"_L1 };
    invalidUrl.setAuthority("---:@@@"_L1);
    const Uri matrixUriFromInvalidUrl{ invalidUrl }, invalidMatrixUri{ u"matrix:&invalid@"_s };
    FAIL_TEST_IF(matrixUriFromInvalidUrl.isEmpty() || matrixUriFromInvalidUrl.isValid(),
                 "Invalid Matrix URI test failed");
    FAIL_TEST_IF(invalidMatrixUri.isEmpty() || invalidMatrixUri.isValid(),
                 "Invalid sigil in a Matrix URI - test failed");

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
        roomId, "matrix:roomid/"_L1 + roomId.mid(1),
        "https://matrix.to/#/%21"_L1/*`!`*/ + roomId.mid(1),
        roomAlias, "matrix:room/"_L1 + roomAlias.mid(1),
        "matrix:r/"_L1 + roomAlias.mid(1),
        "https://matrix.to/#/"_L1 + roomAlias,
    };
    const QStringList userUris { userId, "matrix:user/"_L1 + userId.mid(1),
                                 "matrix:u/"_L1 + userId.mid(1),
                                 "https://matrix.to/#/"_L1 + userId };
    const QStringList eventUris {
        "matrix:room/"_L1 + roomAlias.mid(1) + "/event/"_L1 + eventId.mid(1),
        "matrix:r/"_L1 + roomAlias.mid(1) + "/e/"_L1 + eventId.mid(1),
        "https://matrix.to/#/"_L1 + roomId + u'/' + eventId
    };
    // Check that reserved characters are correctly processed.
    static const auto joinRoomAlias = u"##/?.@\"unjoined:example.org"_s;
    static const auto& encodedRoomAliasNoSigil =
        QString::fromLatin1(QUrl::toPercentEncoding(joinRoomAlias.mid(1), ":"_ba));
    static const auto joinQuery = u"?action=join"_s;
    // These URIs are not supposed to be actually joined (and even exist,
    // as yet) - only to be syntactically correct
    static const QStringList joinByAliasUris{
        Uri(joinRoomAlias.toUtf8(), {}, joinQuery.mid(1)).toDisplayString(),
        "matrix:room/"_L1 % encodedRoomAliasNoSigil % joinQuery,
        "matrix:r/"_L1 % encodedRoomAliasNoSigil % joinQuery,
        "https://matrix.to/#/%23"_L1 /*`#`*/ % encodedRoomAliasNoSigil % joinQuery,
        "https://matrix.to/#/%23"_L1 % joinRoomAlias.mid(1) /* unencoded */ % joinQuery
    };
    static const auto joinRoomId = u"!anyid:example.org"_s;
    static constexpr auto viaServers = std::to_array({ "matrix.org"_L1, "example.org"_L1 });
    static const auto viaQuery = std::apply(
        [](const auto&... servers) { return QString((joinQuery % ... % (u"&via="_s % servers))); },
        viaServers);
    static const QStringList joinByIdUris{ "matrix:roomid/"_L1 % joinRoomId.mid(1) % viaQuery,
                                           "https://matrix.to/#/"_L1 % joinRoomId % viaQuery };
    // If any test breaks, the breaking call will return true, and further
    // execution will be cut by ||'s short-circuiting
    if (testResourceResolver(roomUris, &UriDispatcher::roomAction, room())
        || testResourceResolver(userUris, &UriDispatcher::userAction, connection()->user())
        || testResourceResolver(eventUris, &UriDispatcher::roomAction, room(), { eventId })
        || testResourceResolver(joinByAliasUris, &UriDispatcher::joinAction, connection(),
                                { joinRoomAlias })
        || testResourceResolver(joinByIdUris, &UriDispatcher::joinAction, connection(),
                                { joinRoomId, QStringList(viaServers.cbegin(), viaServers.cend()) }))
        return true;
    // TODO: negative cases
    FINISH_TEST(true);
}

TEST_IMPL(thread)
{
    auto rootTxnId = targetRoom->postText("Threadroot"_L1);
    connectUntil(targetRoom, &Room::pendingEventAboutToMerge, this,
                 [this, thisTest, rootTxnId](RoomEvent *rootEvt) {
        if (rootEvt->transactionId() != rootTxnId)
            return false;

        const auto rootEvtId = rootEvt->id();

        const auto stickerBody = u"Thread reply sticker"_s;
        const auto stickerUrl = QUrl(u"mxc://example.com/example"_s);
        auto replyEvent =
            makeEvent<StickerEvent>(stickerBody, EventContent::ImageContent(stickerUrl));
        replyEvent->setRelation(EventRelation::replyInThread(rootEvtId, true, rootEvtId));

        const auto setRelation = replyEvent->relatesTo();
        FAIL_TEST_IF(!setRelation || setRelation->type != EventRelation::ThreadType
                         || setRelation->eventId != rootEvtId,
                     "Relation not set correctly after setRelation()");

        const auto &pendingItem = targetRoom->post(std::move(replyEvent));
        const auto replyTxnId = pendingItem->transactionId();

        targetRoom->whenMessageMerged(replyTxnId)
            .then(this,
                  [this, thisTest, rootEvtId, stickerBody, stickerUrl](const RoomEvent &replyEvt) {
            replyEvt.switchOnType([&](const StickerEvent &mergedReply) {
                FAIL_TEST_IF(mergedReply.body() != stickerBody);
                FAIL_TEST_IF(mergedReply.url() != stickerUrl);

                const auto relation = mergedReply.relatesTo();
                FAIL_TEST_IF(!relation, "No relation on merged event");
                FAIL_TEST_IF(relation->type != EventRelation::ThreadType,
                             "Incorrect relation type on merged event");
                FAIL_TEST_IF(relation->eventId != rootEvtId,
                             "Thread root eventId doesn't match on merged event");

                FAIL_TEST_IF(!mergedReply.isThreaded(),
                             "isThreaded() returned false for thread reply");
                FAIL_TEST_IF(mergedReply.threadRootEventId() != rootEvtId,
                             "threadRootEventId() doesn't match root");

                // Threaded events are considered replies as a fallback; they are not "normal"
                // one-off replies
                FAIL_TEST_IF(!mergedReply.isReply(true),
                             "isReply() with fallbacks still returned false for thread reply");
                FAIL_TEST_IF(mergedReply.isReply(false),
                             "isReply() without fallbacks considered a threaded event as a reply");
                FAIL_TEST_IF(mergedReply.replyEventId(true) != rootEvtId,
                             "replyEventId() doesn't match root");

                const auto thread = targetRoom->threads()[mergedReply.threadRootEventId()];
                FINISH_TEST(thread.threadRootId == mergedReply.threadRootEventId()
                            && thread.latestEventId == mergedReply.id() && thread.size == 2);
            }, [this, thisTest](const RoomEvent &) { FAIL_TEST(); });
        });
        return true;
    });
    return false;
}

TEST_IMPL(reply)
{
    return targetRoom->post<RoomMessageEvent>(u"Reply target"_s)
        .whenMerged()
        .then([this, thisTest](const RoomEvent &rootEvt) {
        const auto rootEvtId = rootEvt.id();
        const auto replyRelation = EventRelation::replyTo(rootEvtId);
        auto replyEvent = makeEvent<RoomMessageEvent>(u"Reply message"_s,
                                                      RoomMessageEvent::MsgType::Text, nullptr,
                                                      replyRelation);

        const auto setRelation = replyEvent->relatesTo();
        FAIL_TEST_IF(!setRelation || setRelation->type != EventRelation::ReplyType
                         || setRelation->eventId != rootEvtId,
                     "Relation not set correctly after setRelation()");

        replyEvent->clearRelation();
        FAIL_TEST_IF(replyEvent->relatesTo().has_value(),
                     "Relation still exists after clearRelation()");

        // Restore the reply relation
        replyEvent->setRelation(replyRelation);

        targetRoom->post(std::move(replyEvent))
            .whenMerged()
            .then([this, thisTest, rootEvtId](const RoomEvent &replyEvt) {
            replyEvt.switchOnType([&](const RoomMessageEvent &mergedReply) {
                const auto relation = mergedReply.relatesTo();
                FAIL_TEST_IF(!relation || relation->type != EventRelation::ReplyType,
                             "None or incorrect relation on merged event");
                FAIL_TEST_IF(relation->eventId != rootEvtId,
                             "Replied-to eventId doesn't match on merged event");

                FAIL_TEST_IF(mergedReply.isThreaded(),
                             "Non-thread replies should not be considered threaded");
                FAIL_TEST_IF(!mergedReply.isReply(), "isReply() returned false for reply");
                FINISH_TEST(mergedReply.replyEventId() == rootEvtId);
            }, [this, thisTest](const RoomEvent &) { FAIL_TEST(); });
        });
        return true;
    }).isRunning();
}

void TestManager::conclude()
{
    // Clean up the room (best effort)
    auto* room = testSuite->room();
    room->setTopic({});
    c->user()->rename({});

    const QString succeededRec{ QString::number(succeeded.size()) % " of "_L1
                                % QString::number(succeeded.size() + failed.size() + running.size())
                                % " tests succeeded"_L1 };
    QString plainReport = origin % ": Testing complete, "_L1 % succeededRec;
    const QString color = failed.empty() && running.empty() ? "00AA00"_L1 : "AA0000"_L1;
    QString htmlReport = origin % ": <strong><font data-mx-color='#"_L1 % color
                         % "' color='#"_L1 % color
                         % "'>Testing complete</font></strong>, "_L1 % succeededRec;
    if (!failed.empty()) {
        QByteArray failedList;
        for (const auto& f : std::as_const(failed))
            failedList += ' ' + f;
        plainReport += "\nFAILED:"_L1 + QString::fromUtf8(failedList);
        htmlReport += "<br><strong>Failed:</strong>"_L1 + QString::fromUtf8(failedList);
    }
    if (!running.empty()) {
        QByteArray dnfList;
        for (const auto& r : std::as_const(running))
            dnfList += ' ' + r;
        plainReport += "\nDID NOT FINISH:"_L1 + QString::fromUtf8(dnfList);
        htmlReport += "<br><strong>Did not finish:</strong>"_L1 + QString::fromUtf8(dnfList);
    }

    auto txnId = room->postText(plainReport, htmlReport);
    // Now just wait until all the pending events reach the server
    connectUntil(room, &Room::messageSent, this, [this, txnId, room, plainReport] {
        const auto& pendingEvents = room->pendingEvents();
        if (const auto stillFlyingCount =
                std::ranges::count_if(pendingEvents,
                                      [](const PendingEventItem& pe) {
                                          return pe.deliveryStatus() < EventStatus::ReachedServer;
                                      });
            stillFlyingCount > 0) {
            qInfo().nospace() << "Events to reach the server: " << stillFlyingCount
                              << ", not leaving yet";
            return false;
        }

        qInfo("Leaving the room");
        room->leaveRoom().then(this, std::bind_front(&TestManager::finalize, this, plainReport));
        return true;
    });
}

void TestManager::finalize(const QString& lastWords)
{
    if (!c->isLoggedIn()) {
        qCritical("No usable connection reached");
        exit(2);
        return; // NB: QCoreApplication::exit() still returns to the caller
    }
    qInfo("Logging out");
    c->logout().then(
        this, [this, lastWords] {
            qInfo().noquote() << lastWords;
            exit(!testSuite ? 3
                 : succeeded.empty() && failed.empty() && running.empty()
                     ? 4
                     : -static_cast<int>(failed.size() + running.size()));
        });
}

int main(int argc, char* argv[])
{
    return TestManager(argc, argv).exec();
}

#include "quotest.moc"
