/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "directory.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/_matrix/client/r0/directory");

static const auto SetRoomAliasJobName = QStringLiteral("SetRoomAliasJob");

SetRoomAliasJob::SetRoomAliasJob(const QString& roomAlias, const QString& roomId)
    : BaseJob(HttpVerb::Put, SetRoomAliasJobName,
        basePath % "/room/" % roomAlias)
{
    QJsonObject _data;
    addParam<>(_data, QStringLiteral("room_id"), roomId);
    setRequestData(_data);
}

class GetRoomIdByAliasJob::Private
{
    public:
        QString roomId;
        QStringList servers;
};

QUrl GetRoomIdByAliasJob::makeRequestUrl(QUrl baseUrl, const QString& roomAlias)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
            basePath % "/room/" % roomAlias);
}

static const auto GetRoomIdByAliasJobName = QStringLiteral("GetRoomIdByAliasJob");

GetRoomIdByAliasJob::GetRoomIdByAliasJob(const QString& roomAlias)
    : BaseJob(HttpVerb::Get, GetRoomIdByAliasJobName,
        basePath % "/room/" % roomAlias, false)
    , d(new Private)
{
}

GetRoomIdByAliasJob::~GetRoomIdByAliasJob() = default;

const QString& GetRoomIdByAliasJob::roomId() const
{
    return d->roomId;
}

const QStringList& GetRoomIdByAliasJob::servers() const
{
    return d->servers;
}

BaseJob::Status GetRoomIdByAliasJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    d->roomId = fromJson<QString>(json.value("room_id"_ls));
    d->servers = fromJson<QStringList>(json.value("servers"_ls));
    return Success;
}

QUrl DeleteRoomAliasJob::makeRequestUrl(QUrl baseUrl, const QString& roomAlias)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
            basePath % "/room/" % roomAlias);
}

static const auto DeleteRoomAliasJobName = QStringLiteral("DeleteRoomAliasJob");

DeleteRoomAliasJob::DeleteRoomAliasJob(const QString& roomAlias)
    : BaseJob(HttpVerb::Delete, DeleteRoomAliasJobName,
        basePath % "/room/" % roomAlias)
{
}

