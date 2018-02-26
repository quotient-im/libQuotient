
#include "connection.h"
#include "room.h"
#include "user.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QStringBuilder>
#include <iostream>

using namespace QMatrixClient;
using std::cout;
using std::endl;
using std::bind;
using namespace std::placeholders;

void onNewRoom(Room* r, const char* targetRoomName)
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
            r->postMessage(
                "This is a test message from an example application\n"
                "The current user is " % r->localUser()->fullName(r) % "\n" %
                QStringLiteral("This room has %1 member(s)")
                    .arg(r->memberCount()) % "\n" %
                "The room is " %
                    (r->isDirectChat() ? "" : "not ") % "a direct chat\n" %
                "Have a good day",
                MessageEventType::Notice
            );
        }
    });
    QObject::connect(r, &Room::tagsChanged, [=] {
        cout << "Room " << r->id().toStdString() << ", tag(s) changed:" << endl
             << "  " << r->tagNames().join(", ").toStdString() << endl << endl;
    });
    QObject::connect(r, &Room::aboutToAddNewMessages, [=] (RoomEventsRange timeline) {
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
    cout << "Logging out" << endl;
    conn->logout();
    QObject::connect(conn, &Connection::loggedOut, QCoreApplication::instance(),
        [conn] {
            conn->deleteLater();
            QCoreApplication::instance()->processEvents();
            QCoreApplication::instance()->quit();
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
        cout << "Target room name: " << targetRoomName;
    QObject::connect(conn, &Connection::newRoom,
                     bind(onNewRoom, _1, targetRoomName));
    QObject::connect(conn, &Connection::syncDone,
                     bind(finalize, conn));
    return app.exec();
}
