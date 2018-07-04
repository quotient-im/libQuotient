/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "tags.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

class GetRoomTagsJob::Private
{
    public:
        QJsonObject tags;
};

QUrl GetRoomTagsJob::makeRequestUrl(QUrl baseUrl, const QString& userId, const QString& roomId)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
            basePath % "/user/" % userId % "/rooms/" % roomId % "/tags");
}

static const auto GetRoomTagsJobName = QStringLiteral("GetRoomTagsJob");

GetRoomTagsJob::GetRoomTagsJob(const QString& userId, const QString& roomId)
    : BaseJob(HttpVerb::Get, GetRoomTagsJobName,
        basePath % "/user/" % userId % "/rooms/" % roomId % "/tags")
    , d(new Private)
{
}

GetRoomTagsJob::~GetRoomTagsJob() = default;

const QJsonObject& GetRoomTagsJob::tags() const
{
    return d->tags;
}

BaseJob::Status GetRoomTagsJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    d->tags = fromJson<QJsonObject>(json.value("tags"_ls));
    return Success;
}

static const auto SetRoomTagJobName = QStringLiteral("SetRoomTagJob");

SetRoomTagJob::SetRoomTagJob(const QString& userId, const QString& roomId, const QString& tag, const QJsonObject& body)
    : BaseJob(HttpVerb::Put, SetRoomTagJobName,
        basePath % "/user/" % userId % "/rooms/" % roomId % "/tags/" % tag)
{
    setRequestData(Data(toJson(body)));
}

QUrl DeleteRoomTagJob::makeRequestUrl(QUrl baseUrl, const QString& userId, const QString& roomId, const QString& tag)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
            basePath % "/user/" % userId % "/rooms/" % roomId % "/tags/" % tag);
}

static const auto DeleteRoomTagJobName = QStringLiteral("DeleteRoomTagJob");

DeleteRoomTagJob::DeleteRoomTagJob(const QString& userId, const QString& roomId, const QString& tag)
    : BaseJob(HttpVerb::Delete, DeleteRoomTagJobName,
        basePath % "/user/" % userId % "/rooms/" % roomId % "/tags/" % tag)
{
}

