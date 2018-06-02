/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "pusher.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

namespace QMatrixClient
{
    // Converters

    template <> struct FromJson<GetPushersJob::PusherData>
    {
        GetPushersJob::PusherData operator()(const QJsonValue& jv)
        {
            const auto& _json = jv.toObject();
            GetPushersJob::PusherData result;
            result.url =
                fromJson<QString>(_json.value("url"));

            return result;
        }
    };

    template <> struct FromJson<GetPushersJob::Pusher>
    {
        GetPushersJob::Pusher operator()(const QJsonValue& jv)
        {
            const auto& _json = jv.toObject();
            GetPushersJob::Pusher result;
            result.pushkey =
                fromJson<QString>(_json.value("pushkey"));
            result.kind =
                fromJson<QString>(_json.value("kind"));
            result.appId =
                fromJson<QString>(_json.value("app_id"));
            result.appDisplayName =
                fromJson<QString>(_json.value("app_display_name"));
            result.deviceDisplayName =
                fromJson<QString>(_json.value("device_display_name"));
            result.profileTag =
                fromJson<QString>(_json.value("profile_tag"));
            result.lang =
                fromJson<QString>(_json.value("lang"));
            result.data =
                fromJson<GetPushersJob::PusherData>(_json.value("data"));

            return result;
        }
    };
} // namespace QMatrixClient

class GetPushersJob::Private
{
    public:
        QVector<Pusher> pushers;
};

QUrl GetPushersJob::makeRequestUrl(QUrl baseUrl)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
            basePath % "/pushers");
}

GetPushersJob::GetPushersJob()
    : BaseJob(HttpVerb::Get, "GetPushersJob",
        basePath % "/pushers")
    , d(new Private)
{
}

GetPushersJob::~GetPushersJob() = default;

const QVector<GetPushersJob::Pusher>& GetPushersJob::pushers() const
{
    return d->pushers;
}

BaseJob::Status GetPushersJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    d->pushers = fromJson<QVector<Pusher>>(json.value("pushers"));
    return Success;
}

namespace QMatrixClient
{
    // Converters

    QJsonObject toJson(const PostPusherJob::PusherData& pod)
    {
        QJsonObject _json;
        addToJson<IfNotEmpty>(_json, "url", pod.url);
        return _json;
    }
} // namespace QMatrixClient

PostPusherJob::PostPusherJob(const QString& pushkey, const QString& kind, const QString& appId, const QString& appDisplayName, const QString& deviceDisplayName, const QString& lang, const PusherData& data, const QString& profileTag, bool append)
    : BaseJob(HttpVerb::Post, "PostPusherJob",
        basePath % "/pushers/set")
{
    QJsonObject _data;
    addToJson<>(_data, "pushkey", pushkey);
    addToJson<>(_data, "kind", kind);
    addToJson<>(_data, "app_id", appId);
    addToJson<>(_data, "app_display_name", appDisplayName);
    addToJson<>(_data, "device_display_name", deviceDisplayName);
    addToJson<IfNotEmpty>(_data, "profile_tag", profileTag);
    addToJson<>(_data, "lang", lang);
    addToJson<>(_data, "data", data);
    addToJson<IfNotEmpty>(_data, "append", append);
    setRequestData(_data);
}

