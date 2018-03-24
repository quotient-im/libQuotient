
#include "connection.h"
#include "room.h"
#include "user.h"
#include "jobs/sendeventjob.h"

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
        void onNewRoom(Room* r, const QString& testRoomName);
            void doTests();
                void addAndRemoveTag();
                void sendAndRedact();
                    void checkRedactionOutcome(QString evtIdToRedact, RoomEventsRange events);
                void markDirectChat();
                    void checkDirectChatOutcome();
        void finalize();

    private:
        QScopedPointer<Connection, QScopedPointerDeleteLater> c;
        QString origin;
        Room* targetRoom = nullptr;
        int semaphor = 0;

};

#define QMC_CHECK(description, condition) \
{ \
    cout << (description) \
         << (!!(condition) ? " successul" : " FAILED") << endl; \
    targetRoom->postMessage(origin % ": " % QStringLiteral(description) % \
        (!!(condition) ? QStringLiteral(" successful") : \
                         QStringLiteral(" FAILED")), \
        !!(condition) ? MessageEventType::Notice : MessageEventType::Text); \
    --semaphor; \
}

QMCTest::QMCTest(Connection* conn, const QString& testRoomName, QString source)
    : c(conn), origin(std::move(source))
{
    if (!origin.isEmpty())
        cout << "Origin for the test message: " << origin.toStdString() << endl;
    if (!testRoomName.isEmpty())
        cout << "Test room name: " << testRoomName.toStdString() << endl;

    connect(c.data(), &Connection::newRoom,
            this, [this,testRoomName] (Room* r) { onNewRoom(r, testRoomName); });
    connect(c.data(), &Connection::syncDone, c.data(), [this] {
        cout << "Sync complete, " << semaphor << " tests in the air" << endl;
        if (semaphor)
            c->sync(10000);
        else if (targetRoom)
        {
            auto j = c->callApi<SendEventJob>(targetRoom->id(),
                    RoomMessageEvent(origin % ": All tests finished"));
            connect(j, &BaseJob::finished, this, &QMCTest::finalize);
        }
        else
            finalize();
    });
    // Big countdown watchdog
    QTimer::singleShot(180000, this, &QMCTest::finalize);
}

void QMCTest::onNewRoom(Room* r, const QString& testRoomName)
{
    cout << "New room: " << r->id().toStdString() << endl;
    connect(r, &Room::namesChanged, this, [=] {
        cout << "Room " << r->id().toStdString() << ", name(s) changed:" << endl
             << "  Name: " << r->name().toStdString() << endl
             << "  Canonical alias: " << r->canonicalAlias().toStdString() << endl
             << endl << endl;
        if (!testRoomName.isEmpty() && (r->name() == testRoomName ||
                r->canonicalAlias() == testRoomName))
        {
            cout << "Found the target room, proceeding for tests" << endl;
            targetRoom = r;
            ++semaphor;
            auto j = targetRoom->connection()->callApi<SendEventJob>(
                targetRoom->id(),
                RoomMessageEvent(origin % ": connected to test room",
                MessageEventType::Notice));
            connect(j, &BaseJob::success,
                    this, [this] { doTests(); --semaphor; });
        }
    });
    connect(r, &Room::aboutToAddNewMessages, r, [r] (RoomEventsRange timeline) {
        cout << timeline.size() << " new event(s) in room "
             << r->id().toStdString() << endl;
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

void QMCTest::doTests()
{
    ++semaphor; addAndRemoveTag();
    ++semaphor; sendAndRedact();
    ++semaphor; markDirectChat();
}

void QMCTest::addAndRemoveTag()
{
    static const auto TestTag = QStringLiteral("org.qmatrixclient.test");
    // Pre-requisite
    if (targetRoom->tags().contains(TestTag))
        targetRoom->removeTag(TestTag);

    QObject::connect(targetRoom, &Room::tagsChanged, targetRoom, [=] {
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
    // The reverse order because tagsChanged is emitted synchronously.
    cout << "Adding a tag" << endl;
    targetRoom->addTag(TestTag);
}

void QMCTest::sendAndRedact()
{
    cout << "Sending a message to redact" << endl;
    auto* job = targetRoom->connection()->callApi<SendEventJob>(targetRoom->id(),
            RoomMessageEvent(origin % ": Message to redact"));
    connect(job, &BaseJob::success, targetRoom, [job,this] {
        cout << "Message to redact has been succesfully sent, redacting" << endl;
        targetRoom->redactEvent(job->eventId(), origin);
        // Make sure to save the event id because the job is about to end.
        connect(targetRoom, &Room::aboutToAddNewMessages, this,
                std::bind(&QMCTest::checkRedactionOutcome,
                          this, job->eventId(), _1));
    });
}

void QMCTest::checkRedactionOutcome(QString evtIdToRedact,
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
            targetRoom->postMessage(msg);
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
        targetRoom->postMessage(msg);
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
    if (c->isDirectChat(targetRoom))
    {
        cout << "Warning: the room is already a direct chat,"
                " only unmarking will be tested" << endl;
        checkDirectChatOutcome();
    }
    cout << "Marking the room as a direct chat" << endl;
    c->addToDirectChats(targetRoom, c->user());
    connect(c.data(), &Connection::directChatsListChanged,
            this, &QMCTest::checkDirectChatOutcome);
}

void QMCTest::checkDirectChatOutcome()
{
    if (!c->isDirectChat(targetRoom))
    {
        cout << "Room not (yet?) added to direct chats, waiting" << endl;
        return;
    }

    cout << "Room marked as a direct chat, unmarking now" << endl;
    disconnect(c.data(), &Connection::directChatsListChanged, nullptr, nullptr);
    c->removeFromDirectChats(targetRoom, c->user());
    connect(c.data(), &Connection::directChatsListChanged, this, [this] {
        if (c->isDirectChat(targetRoom))
        {
            cout << "Room not (yet?) removed from direct chats, waiting" << endl;
            return;
        }

        QMC_CHECK("Direct chat test", !c->isDirectChat(targetRoom));
        disconnect(c.data(), &Connection::directChatsListChanged,
                   nullptr, nullptr);
    });
}

void QMCTest::finalize()
{
    if (semaphor)
        cout << "One or more tests FAILED" << endl;
    cout << "Logging out" << endl;
    c->logout();
    connect(c.data(), &Connection::loggedOut, QCoreApplication::instance(),
        [this] {
            QCoreApplication::processEvents();
            QCoreApplication::exit(semaphor);
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
    QObject::connect(conn, &Connection::connected, [=] {
        cout << "Connected, server: "
             << conn->homeserver().toDisplayString().toStdString() << endl;
        cout << "Access token: " << conn->accessToken().toStdString() << endl;
        conn->sync();
    });
    QMCTest test { conn, argc >= 5 ? argv[4] : nullptr,
                         argc >= 6 ? argv[5] : nullptr };
    return app.exec();
}
