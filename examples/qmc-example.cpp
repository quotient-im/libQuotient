
#include "connection.h"
#include "room.h"
#include "user.h"
#include "csapi/room_send.h"
#include "csapi/joining.h"
#include "csapi/leaving.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QStringBuilder>
#include <QtCore/QTimer>
#include <iostream>
#include <functional>

using namespace QMatrixClient;
using std::cout;
using std::endl;
using namespace std::placeholders;

class QMCTest : public QObject
{
    public:
        QMCTest(Connection* conn, const QString& testRoomName, QString source);

    private slots:
        void setup(const QString& testRoomName);
        void onNewRoom(Room* r);
        void startTests();
            void sendMessage();
            void addAndRemoveTag();
            void sendAndRedact();
                void checkRedactionOutcome(const QString& evtIdToRedact,
                                   RoomEventsRange events);
            void markDirectChat();
                void checkDirectChatOutcome(
                        const Connection::DirectChatsMap& added);
        void leave();
        void finalize();

    private:
        QScopedPointer<Connection, QScopedPointerDeleteLater> c;
        QStringList running;
        QStringList succeeded;
        QStringList failed;
        QString origin;
        Room* targetRoom = nullptr;
};

#define QMC_CHECK(description, condition) \
{ \
    const bool result = !!(condition); \
    Q_ASSERT(running.removeOne(description)); \
    (result ? succeeded : failed).push_back(description); \
    cout << (description) << (result ? " successul" : " FAILED") << endl; \
    if (targetRoom) \
        targetRoom->postMessage(origin % ": " % QStringLiteral(description) % \
            (result ? QStringLiteral(" successful") : QStringLiteral(" FAILED")), \
            result ? MessageEventType::Notice : MessageEventType::Text); \
}

QMCTest::QMCTest(Connection* conn, const QString& testRoomName, QString source)
    : c(conn), origin(std::move(source))
{
    if (!origin.isEmpty())
        cout << "Origin for the test message: " << origin.toStdString() << endl;
    if (!testRoomName.isEmpty())
        cout << "Test room name: " << testRoomName.toStdString() << endl;

    connect(c.data(), &Connection::connected,
            this, std::bind(&QMCTest::setup, this, testRoomName));
    connect(c.data(), &Connection::loadedRoomState, this, &QMCTest::onNewRoom);
    // Big countdown watchdog
    QTimer::singleShot(180000, this, &QMCTest::leave);
}

void QMCTest::setup(const QString& testRoomName)
{
    cout << "Connected, server: "
         << c->homeserver().toDisplayString().toStdString() << endl;
    cout << "Access token: " << c->accessToken().toStdString() << endl;

    // Setting up sync loop
    c->sync();
    connect(c.data(), &Connection::syncDone, c.data(), [this,testRoomName] {
        cout << "Sync complete, "
             << running.size() << " tests in the air" << endl;
        if (!running.isEmpty())
            c->sync(10000);
        else if (targetRoom)
        {
            targetRoom->postPlainText(origin % ": All tests finished");
            connect(targetRoom, &Room::pendingEventMerged, this, &QMCTest::leave);
        }
        else
            finalize();
    });

    // Join a testroom, if provided
    if (!targetRoom && !testRoomName.isEmpty())
    {
        cout << "Joining " << testRoomName.toStdString() << endl;
        running.push_back("Join room");
        auto joinJob = c->joinRoom(testRoomName);
        connect(joinJob, &BaseJob::failure, this,
            [this] { QMC_CHECK("Join room", false); finalize(); });
        // As of BaseJob::success, a Room object is not guaranteed to even
        // exist; it's a mere confirmation that the server processed
        // the request.
        connect(c.data(), &Connection::loadedRoomState, this,
            [this,testRoomName] (Room* room) {
                Q_ASSERT(room); // It's a grave failure if room is nullptr here
                if (room->canonicalAlias() != testRoomName)
                    return; // Not our room

                targetRoom = room;
                QMC_CHECK("Join room", true);
                startTests();
            });
    }
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

void QMCTest::startTests()
{
    cout << "Starting tests" << endl;
    sendMessage();
    addAndRemoveTag();
    sendAndRedact();
    markDirectChat();
}

void QMCTest::sendMessage()
{
    running.push_back("Message sending");
    cout << "Sending a message" << endl;
    auto txnId = targetRoom->postPlainText("Hello, " % origin % " is here");
    auto& pending = targetRoom->pendingEvents();
    if (pending.empty())
    {
        QMC_CHECK("Message sending", false);
        return;
    }
    auto it = std::find_if(pending.begin(), pending.end(),
                [&txnId] (const auto& e) {
                    return e->transactionId() == txnId;
                });
    QMC_CHECK("Message sending", it != pending.end());
    // TODO: Wait when it actually gets sent; check that it obtained an id
    // Independently, check when it shows up in the timeline.
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
            QObject::disconnect(targetRoom, &Room::tagsChanged, nullptr, nullptr);
        }
    });
    cout << "Adding a tag" << endl;
    targetRoom->addTag(TestTag);
}

void QMCTest::sendAndRedact()
{
    running.push_back("Redaction");
    cout << "Sending a message to redact" << endl;
    if (auto* job = targetRoom->connection()->sendMessage(targetRoom->id(),
            RoomMessageEvent(origin % ": message to redact")))
    {
        connect(job, &BaseJob::success, targetRoom, [job,this] {
            cout << "Redacting the message" << endl;
            targetRoom->redactEvent(job->eventId(), origin);
            // Make sure to save the event id because the job is about to end.
            connect(targetRoom, &Room::aboutToAddNewMessages, this,
                    std::bind(&QMCTest::checkRedactionOutcome,
                              this, job->eventId(), _1));
        });
    } else
        QMC_CHECK("Redaction", false);
}

void QMCTest::checkRedactionOutcome(const QString& evtIdToRedact,
                                    RoomEventsRange events)
{
    static bool checkSucceeded = false;
    // There are two possible (correct) outcomes: either the event comes already
    // redacted at the next sync, or the nearest sync completes with
    // the unredacted event but the next one brings redaction.
    auto it = std::find_if(events.begin(), events.end(),
                [=] (const RoomEventPtr& e) {
                    return e->id() == evtIdToRedact;
                });
    if (it == events.end())
        return; // Waiting for the next sync

    if ((*it)->isRedacted())
    {
        if (checkSucceeded)
        {
            const auto msg =
                    "The redacted event came in with the sync again, ignoring";
            cout << msg << endl;
            targetRoom->postPlainText(msg);
            return;
        }
        cout << "The sync brought already redacted message" << endl;
        QMC_CHECK("Redaction", true);
        // Not disconnecting because there are other connections from this class
        // to aboutToAddNewMessages
        checkSucceeded = true;
        return;
    }
    // The event is not redacted
    if (checkSucceeded)
    {
        const auto msg =
                "Warning: the redacted event came non-redacted with the sync!";
        cout << msg << endl;
        targetRoom->postPlainText(msg);
    }
    cout << "Message came non-redacted with the sync, waiting for redaction" << endl;
    connect(targetRoom, &Room::replacedEvent, targetRoom,
        [=] (const RoomEvent* newEvent, const RoomEvent* oldEvent) {
            QMC_CHECK("Redaction", oldEvent->id() == evtIdToRedact &&
                      newEvent->isRedacted() &&
                      newEvent->redactionReason() == origin);
            checkSucceeded = true;
            disconnect(targetRoom, &Room::replacedEvent, nullptr, nullptr);
        });

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

void QMCTest::leave()
{
    if (targetRoom)
    {
        cout << "Leaving the room" << endl;
        connect(targetRoom->leaveRoom(), &BaseJob::finished,
                this, &QMCTest::finalize);
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
            if (!failed.isEmpty())
                cout << "FAILED: " << failed.join(", ").toStdString() << endl;
            if (!running.isEmpty())
                cout << "DID NOT FINISH: "
                     << running.join(", ").toStdString() << endl;
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
