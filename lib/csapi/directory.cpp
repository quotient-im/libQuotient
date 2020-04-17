/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "directory.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace Quotient;

static const auto basePath = QStringLiteral("/_matrix/client/r0/directory");

SetRoomAliasJob::SetRoomAliasJob(const QString& roomAlias, const QString& roomId)
    : BaseJob(HttpVerb::Put, QStringLiteral("SetRoomAliasJob"),
              basePath % "/room/" % roomAlias)
{
    QJsonObject _data;
    addParam<>(_data, QStringLiteral("room_id"), roomId);
    setRequestData(_data);
}

class GetRoomIdByAliasJob::Private {
public:
    QString roomId;
    QStringList servers;
};

QUrl GetRoomIdByAliasJob::makeRequestUrl(QUrl baseUrl, const QString& roomAlias)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   basePath % "/room/" % roomAlias);
}

GetRoomIdByAliasJob::GetRoomIdByAliasJob(const QString& roomAlias)
    : BaseJob(HttpVerb::Get, QStringLiteral("GetRoomIdByAliasJob"),
              basePath % "/room/" % roomAlias, false)
    , d(new Private)
{}

GetRoomIdByAliasJob::~GetRoomIdByAliasJob() = default;

const QString& GetRoomIdByAliasJob::roomId() const { return d->roomId; }

const QStringList& GetRoomIdByAliasJob::servers() const { return d->servers; }

BaseJob::Status GetRoomIdByAliasJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    fromJson(json.value("room_id"_ls), d->roomId);
    fromJson(json.value("servers"_ls), d->servers);

    return Success;
}

QUrl DeleteRoomAliasJob::makeRequestUrl(QUrl baseUrl, const QString& roomAlias)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   basePath % "/room/" % roomAlias);
}

DeleteRoomAliasJob::DeleteRoomAliasJob(const QString& roomAlias)
    : BaseJob(HttpVerb::Delete, QStringLiteral("DeleteRoomAliasJob"),
              basePath % "/room/" % roomAlias)
{}
