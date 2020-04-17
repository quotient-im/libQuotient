/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "tags.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace Quotient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

// Converters
namespace Quotient {

template <>
struct JsonObjectConverter<GetRoomTagsJob::Tag> {
    static void fillFrom(QJsonObject jo, GetRoomTagsJob::Tag& result)
    {
        fromJson(jo.take("order"_ls), result.order);
        fromJson(jo, result.additionalProperties);
    }
};

} // namespace Quotient

class GetRoomTagsJob::Private {
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

GetRoomTagsJob::GetRoomTagsJob(const QString& userId, const QString& roomId)
    : BaseJob(HttpVerb::Get, QStringLiteral("GetRoomTagsJob"),
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

SetRoomTagJob::SetRoomTagJob(const QString& userId, const QString& roomId,
                             const QString& tag, Omittable<float> order)
    : BaseJob(HttpVerb::Put, QStringLiteral("SetRoomTagJob"),
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

DeleteRoomTagJob::DeleteRoomTagJob(const QString& userId, const QString& roomId,
                                   const QString& tag)
    : BaseJob(HttpVerb::Delete, QStringLiteral("DeleteRoomTagJob"),
              basePath % "/user/" % userId % "/rooms/" % roomId % "/tags/" % tag)
{}
