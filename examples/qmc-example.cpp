
#include "connection.h"
#include "room.h"
#include "user.h"
#include "jobs/sendeventjob.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QStringBuilder>
#include <QtCore/QTimer>
#include <iostream>

using namespace QMatrixClient;
using std::cout;
using std::endl;
using namespace std::placeholders;

static int semaphor = 0;
static Room* targetRoom = nullptr;

#define QMC_CHECK(origin, description, condition) \
    cout << (description) \
         << (!!(condition) ? " successul" : " FAILED") << endl; \
    targetRoom->postMessage(QString(origin) % ": " % QStringLiteral(description) % \
        (!!(condition) ? QStringLiteral(" successful") : \
                         QStringLiteral(" FAILED")), MessageEventType::Notice)

void addAndRemoveTag(const char* origin)
{
    ++semaphor;
    static const auto TestTag = QStringLiteral("org.qmatrixclient.test");
    QObject::connect(targetRoom, &Room::tagsChanged, targetRoom, [=] {
        cout << "Room " << targetRoom->id().toStdString()
             << ", tag(s) changed:" << endl
             << "  " << targetRoom->tagNames().join(", ").toStdString() << endl;
        if (targetRoom->tags().contains(TestTag))
        {
            targetRoom->removeTag(TestTag);
            QMC_CHECK(origin, "Tagging test",
                      !targetRoom->tags().contains(TestTag));
            --semaphor;
            QObject::disconnect(targetRoom, &Room::tagsChanged, nullptr, nullptr);
        }
    });
    // The reverse order because tagsChanged is emitted synchronously.
    targetRoom->addTag(TestTag);
}

void sendAndRedact(const char* origin)
{
    ++semaphor;
    auto* job = targetRoom->connection()->callApi<SendEventJob>(targetRoom->id(),
            RoomMessageEvent("Message to redact"));
    QObject::connect(job, &BaseJob::success, targetRoom, [job] {
        targetRoom->redactEvent(job->eventId(), "qmc-example");
    });
    QObject::connect(targetRoom, &Room::replacedEvent, targetRoom,
        [=] (const RoomEvent* newEvent) {
            QMC_CHECK(origin, "Redaction", newEvent->isRedacted() &&
                        newEvent->redactionReason() == "qmc-example");
            --semaphor;
            QObject::disconnect(targetRoom, &Room::replacedEvent,
                                nullptr, nullptr);
        });
}

void onNewRoom(Room* r, const char* targetRoomName, const char* origin)
{
    cout << "New room: " << r->id().toStdString() << endl;
    QObject::connect(r, &Room::namesChanged, [=] {
        cout << "Room " << r->id().toStdString() << ", name(s) changed:" << endl
             << "  Name: " << r->name().toStdString() << endl
             << "  Canonical alias: " << r->canonicalAlias().toStdString() << endl
             << endl << endl;
        if (targetRoomName && (r->name() == targetRoomName ||
                r->canonicalAlias() == targetRoomName))
        {
            cout << "Found the target room, proceeding for tests" << endl;
            targetRoom = r;
            addAndRemoveTag(origin);
            sendAndRedact(origin);
            targetRoom->postMessage(
                "This is a test notice from an example application\n"
                "Origin: " % QString(origin) % "\n"
                "The current user is " %
                    targetRoom->localUser()->fullName(targetRoom) % "\n" %
//                "The room is " %
//                    (r->isDirectChat() ? "" : "not ") % "a direct chat\n" %
                "Have a good day",
                MessageEventType::Notice
            );
        }
    });
    QObject::connect(r, &Room::aboutToAddNewMessages, [r] (RoomEventsRange timeline) {
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

void finalize(Connection* conn)
{
    if (semaphor)
        cout << "One or more tests FAILED" << endl;
    cout << "Logging out" << endl;
    conn->logout();
    QObject::connect(conn, &Connection::loggedOut, QCoreApplication::instance(),
        [conn] {
            conn->deleteLater();
            QCoreApplication::processEvents();
            QCoreApplication::exit(semaphor);
        });
}

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    if (argc < 3)
        return -1;

    cout << "Connecting to the server as " << argv[1] << endl;
    auto conn = new Connection;
    conn->connectToServer(argv[1], argv[2], "QMatrixClient example application");
    QObject::connect(conn, &Connection::connected, [=] {
        cout << "Connected, server: "
             << conn->homeserver().toDisplayString().toStdString() << endl;
        cout << "Access token: " << conn->accessToken().toStdString() << endl;
        conn->sync();
    });
    const char* targetRoomName = argc >= 4 ? argv[3] : nullptr;
    if (targetRoomName)
        cout << "Target room name: " << targetRoomName << endl;
    const char* origin = argc >= 5 ? argv[4] : nullptr;
    if (origin)
        cout << "Origin for the test message: " << origin << endl;
    QObject::connect(conn, &Connection::newRoom,
        [=](Room* room) { onNewRoom(room, targetRoomName, origin); });
    QObject::connect(conn, &Connection::syncDone, conn, [conn] {
        cout << "Sync complete, " << semaphor << " tests in the air" << endl;
        if (semaphor)
            conn->sync(10000);
        else
        {
            if (targetRoom)
            {
                auto j = conn->callApi<SendEventJob>(targetRoom->id(),
                            RoomMessageEvent("All tests finished"));
                QObject::connect(j, &BaseJob::finished,
                                 conn, [conn] { finalize(conn); });
            }
            else
                finalize(conn);
        }
    });
    // Big red countdown
    QTimer::singleShot(180000, conn, [conn] { finalize(conn); });
    return app.exec();
}
