
#include "connection.h"
#include "room.h"

#include <QCoreApplication>
#include <iostream>
#include <string>

using namespace QMatrixClient;
using std::cout;
using std::endl;
using std::string;

void onNewRoom(Room* r)
{
    cout << "New room: " << r->id().toStdString() << endl;
    QObject::connect(r, &Room::namesChanged, [=] {
        cout << "Room " << r->id().toStdString() << ", name(s) changed:" << endl
             << "  Name: " << r->name().toStdString() << endl
             << "  Canonical alias: " << r->canonicalAlias().toStdString()
             << endl << endl;
    });
    QObject::connect(r, &Room::aboutToAddNewMessages, [=] (RoomEventsView events) {
        cout << events.size() << " new event(s) in room "
             << r->id().toStdString() << ":" << endl;
        for (auto e: events)
        {
            cout << "From: "
                 << r->roomMembername(e->senderId()).toStdString()
                 << endl << "Timestamp:"
                 << e->timestamp().toString().toStdString() << endl
                 << "JSON:" << endl << string(e->originalJson()) << endl;
        }
    });
}

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    if (argc < 2)
        return -1;

    auto conn = new Connection(QUrl("https://matrix.org"));
    conn->connectToServer(argv[1], argv[2], "QMatrixClient example application");
    auto c = QObject::connect(conn, &Connection::connected, [=] {
        cout << "Connected" << endl;
       conn->sync();
    });
    QObject::connect(conn, &Connection::syncDone, [=] {
        cout << "Sync done" << endl;
        conn->sync(30000);
    });
    QObject::connect(conn, &Connection::newRoom, onNewRoom);
    return app.exec();
}
