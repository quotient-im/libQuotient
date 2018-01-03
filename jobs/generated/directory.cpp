/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "directory.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/_matrix/client/r0/directory");

SetRoomAliasJob::SetRoomAliasJob(const QString& roomAlias, const QString& roomId)
    : BaseJob(HttpVerb::Put, "SetRoomAliasJob",
        basePath % "/room/" % roomAlias,
        Query { }
    )
{
    QJsonObject _data;
    if (!roomId.isEmpty())
        _data.insert("room_id", toJson(roomId));
    setRequestData(_data);
}

class GetRoomIdByAliasJob::Private
{
    public:
        QString roomId;
        QVector<QString> servers;
};

GetRoomIdByAliasJob::GetRoomIdByAliasJob(const QString& roomAlias)
    : BaseJob(HttpVerb::Get, "GetRoomIdByAliasJob",
        basePath % "/room/" % roomAlias,
        Query { }, Data { }, false
    ), d(new Private)
{ }

GetRoomIdByAliasJob::~GetRoomIdByAliasJob() = default;

const QString& GetRoomIdByAliasJob::roomId() const
{
    return d->roomId;
}

const QVector<QString>& GetRoomIdByAliasJob::servers() const
{
    return d->servers;
}

BaseJob::Status GetRoomIdByAliasJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    d->roomId = fromJson<QString>(json.value("room_id"));
    d->servers = fromJson<QVector<QString>>(json.value("servers"));
    return Success;
}

DeleteRoomAliasJob::DeleteRoomAliasJob(const QString& roomAlias)
    : BaseJob(HttpVerb::Delete, "DeleteRoomAliasJob",
        basePath % "/room/" % roomAlias,
        Query { }
    )
{ }

