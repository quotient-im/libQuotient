#include "joinroom.h"

#include <QtCore/QDebug>

using namespace QMatrixClient::ServerApi;

JoinRoom::JoinRoom(QString roomAlias)
    : CallConfig("JoinRoom", JobHttpType::PostJob, "/join/" + roomAlias)
{ }

Result<QString> JoinRoom::parseReply(const QJsonObject& json) const
{
    if( json.contains("room_id") )
    {
        return json.value("room_id").toString();
    }

    qDebug() << "JoinRoom call returned invalid JSON:";
    qDebug() << json;
    return { UserDefinedError, "No room_id in the JSON response" };
}
