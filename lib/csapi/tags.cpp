/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "tags.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

namespace QMatrixClient
{
    // Converters

    template <> struct FromJsonObject<GetRoomTagsJob::Tag>
    {
        GetRoomTagsJob::Tag operator()(QJsonObject jo) const
        {
            GetRoomTagsJob::Tag result;
            result.order =
                fromJson<float>(jo.take("order"_ls));

            result.additionalProperties = fromJson<QVariantHash>(jo);
            return result;
        }
    };
} // namespace QMatrixClient

class GetRoomTagsJob::Private
{
    public:
        QHash<QString, Tag> tags;
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

const QHash<QString, GetRoomTagsJob::Tag>& GetRoomTagsJob::tags() const
{
    return d->tags;
}

BaseJob::Status GetRoomTagsJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    d->tags = fromJson<QHash<QString, Tag>>(json.value("tags"_ls));
    return Success;
}

static const auto SetRoomTagJobName = QStringLiteral("SetRoomTagJob");

SetRoomTagJob::SetRoomTagJob(const QString& userId, const QString& roomId, const QString& tag, Omittable<float> order)
    : BaseJob(HttpVerb::Put, SetRoomTagJobName,
        basePath % "/user/" % userId % "/rooms/" % roomId % "/tags/" % tag)
{
    QJsonObject _data;
    addParam<IfNotEmpty>(_data, QStringLiteral("order"), order);
    setRequestData(_data);
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

