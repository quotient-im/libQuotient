/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "tags.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

// Converters
namespace QMatrixClient
{

template <>
struct JsonObjectConverter<GetRoomTagsJob::Tag>
{
    static void fillFrom(QJsonObject jo, GetRoomTagsJob::Tag& result)
    {
        fromJson(jo.take("order"_ls), result.order);
        fromJson(jo, result.additionalProperties);
    }
};

} // namespace QMatrixClient

class GetRoomTagsJob::Private
{
public:
    QHash<QString, Tag> tags;
};

QUrl GetRoomTagsJob::makeRequestUrl(QUrl baseUrl, const QString& userId,
                                    const QString& roomId)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl), basePath % "/user/"
                                                           % userId % "/rooms/"
                                                           % roomId % "/tags");
}

static const auto GetRoomTagsJobName = QStringLiteral("GetRoomTagsJob");

GetRoomTagsJob::GetRoomTagsJob(const QString& userId, const QString& roomId)
    : BaseJob(HttpVerb::Get, GetRoomTagsJobName,
              basePath % "/user/" % userId % "/rooms/" % roomId % "/tags")
    , d(new Private)
{}

GetRoomTagsJob::~GetRoomTagsJob() = default;

const QHash<QString, GetRoomTagsJob::Tag>& GetRoomTagsJob::tags() const
{
    return d->tags;
}

BaseJob::Status GetRoomTagsJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    fromJson(json.value("tags"_ls), d->tags);

    return Success;
}

static const auto SetRoomTagJobName = QStringLiteral("SetRoomTagJob");

SetRoomTagJob::SetRoomTagJob(const QString& userId, const QString& roomId,
                             const QString& tag, Omittable<float> order)
    : BaseJob(HttpVerb::Put, SetRoomTagJobName,
              basePath % "/user/" % userId % "/rooms/" % roomId % "/tags/" % tag)
{
    QJsonObject _data;
    addParam<IfNotEmpty>(_data, QStringLiteral("order"), order);
    setRequestData(_data);
}

QUrl DeleteRoomTagJob::makeRequestUrl(QUrl baseUrl, const QString& userId,
                                      const QString& roomId, const QString& tag)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   basePath % "/user/" % userId % "/rooms/"
                                       % roomId % "/tags/" % tag);
}

static const auto DeleteRoomTagJobName = QStringLiteral("DeleteRoomTagJob");

DeleteRoomTagJob::DeleteRoomTagJob(const QString& userId, const QString& roomId,
                                   const QString& tag)
    : BaseJob(HttpVerb::Delete, DeleteRoomTagJobName,
              basePath % "/user/" % userId % "/rooms/" % roomId % "/tags/" % tag)
{}
