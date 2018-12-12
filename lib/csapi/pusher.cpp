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

    template <> struct FromJsonObject<GetPushersJob::PusherData>
    {
        GetPushersJob::PusherData operator()(const QJsonObject& jo) const
        {
            GetPushersJob::PusherData result;
            result.url =
                fromJson<QString>(jo.value("url"_ls));
            result.format =
                fromJson<QString>(jo.value("format"_ls));

            return result;
        }
    };

    template <> struct FromJsonObject<GetPushersJob::Pusher>
    {
        GetPushersJob::Pusher operator()(const QJsonObject& jo) const
        {
            GetPushersJob::Pusher result;
            result.pushkey =
                fromJson<QString>(jo.value("pushkey"_ls));
            result.kind =
                fromJson<QString>(jo.value("kind"_ls));
            result.appId =
                fromJson<QString>(jo.value("app_id"_ls));
            result.appDisplayName =
                fromJson<QString>(jo.value("app_display_name"_ls));
            result.deviceDisplayName =
                fromJson<QString>(jo.value("device_display_name"_ls));
            result.profileTag =
                fromJson<QString>(jo.value("profile_tag"_ls));
            result.lang =
                fromJson<QString>(jo.value("lang"_ls));
            result.data =
                fromJson<GetPushersJob::PusherData>(jo.value("data"_ls));

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
        QJsonObject jo;
        addParam<IfNotEmpty>(jo, QStringLiteral("url"), pod.url);
        addParam<IfNotEmpty>(jo, QStringLiteral("format"), pod.format);
        return jo;
    }
} // namespace QMatrixClient

static const auto PostPusherJobName = QStringLiteral("PostPusherJob");

PostPusherJob::PostPusherJob(const QString& pushkey, const QString& kind, const QString& appId, const QString& appDisplayName, const QString& deviceDisplayName, const QString& lang, const PusherData& data, const QString& profileTag, Omittable<bool> append)
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

