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
                fromJson<QString>(_json.value("url"_ls));
            result.format =
                fromJson<QString>(_json.value("format"_ls));

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
                fromJson<QString>(_json.value("pushkey"_ls));
            result.kind =
                fromJson<QString>(_json.value("kind"_ls));
            result.appId =
                fromJson<QString>(_json.value("app_id"_ls));
            result.appDisplayName =
                fromJson<QString>(_json.value("app_display_name"_ls));
            result.deviceDisplayName =
                fromJson<QString>(_json.value("device_display_name"_ls));
            result.profileTag =
                fromJson<QString>(_json.value("profile_tag"_ls));
            result.lang =
                fromJson<QString>(_json.value("lang"_ls));
            result.data =
                fromJson<GetPushersJob::PusherData>(_json.value("data"_ls));

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

static const auto GetPushersJobName = QStringLiteral("GetPushersJob");

GetPushersJob::GetPushersJob()
    : BaseJob(HttpVerb::Get, GetPushersJobName,
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
    d->pushers = fromJson<QVector<Pusher>>(json.value("pushers"_ls));
    return Success;
}

namespace QMatrixClient
{
    // Converters

    QJsonObject toJson(const PostPusherJob::PusherData& pod)
    {
        QJsonObject _json;
        addParam<IfNotEmpty>(_json, QStringLiteral("url"), pod.url);
        addParam<IfNotEmpty>(_json, QStringLiteral("format"), pod.format);
        return _json;
    }
} // namespace QMatrixClient

static const auto PostPusherJobName = QStringLiteral("PostPusherJob");

PostPusherJob::PostPusherJob(const QString& pushkey, const QString& kind, const QString& appId, const QString& appDisplayName, const QString& deviceDisplayName, const QString& lang, const PusherData& data, const QString& profileTag, bool append)
    : BaseJob(HttpVerb::Post, PostPusherJobName,
        basePath % "/pushers/set")
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

