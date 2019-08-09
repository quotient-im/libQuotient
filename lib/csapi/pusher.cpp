/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "pusher.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace Quotient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

// Converters
namespace Quotient
{

template <>
struct JsonObjectConverter<GetPushersJob::PusherData>
{
    static void fillFrom(const QJsonObject& jo,
                         GetPushersJob::PusherData& result)
    {
        fromJson(jo.value("url"_ls), result.url);
        fromJson(jo.value("format"_ls), result.format);
    }
};

template <>
struct JsonObjectConverter<GetPushersJob::Pusher>
{
    static void fillFrom(const QJsonObject& jo, GetPushersJob::Pusher& result)
    {
        fromJson(jo.value("pushkey"_ls), result.pushkey);
        fromJson(jo.value("kind"_ls), result.kind);
        fromJson(jo.value("app_id"_ls), result.appId);
        fromJson(jo.value("app_display_name"_ls), result.appDisplayName);
        fromJson(jo.value("device_display_name"_ls), result.deviceDisplayName);
        fromJson(jo.value("profile_tag"_ls), result.profileTag);
        fromJson(jo.value("lang"_ls), result.lang);
        fromJson(jo.value("data"_ls), result.data);
    }
};

} // namespace Quotient

class GetPushersJob::Private
{
public:
    QVector<Pusher> pushers;
};

QUrl GetPushersJob::makeRequestUrl(QUrl baseUrl)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl), basePath % "/pushers");
}

static const auto GetPushersJobName = QStringLiteral("GetPushersJob");

GetPushersJob::GetPushersJob()
    : BaseJob(HttpVerb::Get, GetPushersJobName, basePath % "/pushers")
    , d(new Private)
{}

GetPushersJob::~GetPushersJob() = default;

const QVector<GetPushersJob::Pusher>& GetPushersJob::pushers() const
{
    return d->pushers;
}

BaseJob::Status GetPushersJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    fromJson(json.value("pushers"_ls), d->pushers);

    return Success;
}

// Converters
namespace Quotient
{

template <>
struct JsonObjectConverter<PostPusherJob::PusherData>
{
    static void dumpTo(QJsonObject& jo, const PostPusherJob::PusherData& pod)
    {
        addParam<IfNotEmpty>(jo, QStringLiteral("url"), pod.url);
        addParam<IfNotEmpty>(jo, QStringLiteral("format"), pod.format);
    }
};

} // namespace Quotient

static const auto PostPusherJobName = QStringLiteral("PostPusherJob");

PostPusherJob::PostPusherJob(const QString& pushkey, const QString& kind,
                             const QString& appId, const QString& appDisplayName,
                             const QString& deviceDisplayName,
                             const QString& lang, const PusherData& data,
                             const QString& profileTag, Omittable<bool> append)
    : BaseJob(HttpVerb::Post, PostPusherJobName, basePath % "/pushers/set")
{
    QJsonObject _data;
    addParam<>(_data, QStringLiteral("pushkey"), pushkey);
    addParam<>(_data, QStringLiteral("kind"), kind);
    addParam<>(_data, QStringLiteral("app_id"), appId);
    addParam<>(_data, QStringLiteral("app_display_name"), appDisplayName);
    addParam<>(_data, QStringLiteral("device_display_name"), deviceDisplayName);
    addParam<IfNotEmpty>(_data, QStringLiteral("profile_tag"), profileTag);
    addParam<>(_data, QStringLiteral("lang"), lang);
    addParam<>(_data, QStringLiteral("data"), data);
    addParam<IfNotEmpty>(_data, QStringLiteral("append"), append);
    setRequestData(_data);
}
