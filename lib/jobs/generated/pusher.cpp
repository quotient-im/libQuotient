/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "pusher.h"

#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

GetPushersJob::PusherData::operator QJsonObject() const
{
    QJsonObject o;
    o.insert("url", toJson(url));
    
    return o;
}
namespace QMatrixClient
{
    template <> struct FromJson<GetPushersJob::PusherData>
    {
        GetPushersJob::PusherData operator()(QJsonValue jv)
        {
            QJsonObject o = jv.toObject();
            GetPushersJob::PusherData result;
            result.url =
            fromJson<QString>(o.value("url"));
            
            return result;
        }
    };
} // namespace QMatrixClient

GetPushersJob::Pusher::operator QJsonObject() const
{
    QJsonObject o;
    o.insert("pushkey", toJson(pushkey));
    o.insert("kind", toJson(kind));
    o.insert("app_id", toJson(appId));
    o.insert("app_display_name", toJson(appDisplayName));
    o.insert("device_display_name", toJson(deviceDisplayName));
    o.insert("profile_tag", toJson(profileTag));
    o.insert("lang", toJson(lang));
    o.insert("data", toJson(data));
    
    return o;
}
namespace QMatrixClient
{
    template <> struct FromJson<GetPushersJob::Pusher>
    {
        GetPushersJob::Pusher operator()(QJsonValue jv)
        {
            QJsonObject o = jv.toObject();
            GetPushersJob::Pusher result;
            result.pushkey =
            fromJson<QString>(o.value("pushkey"));
            result.kind =
            fromJson<QString>(o.value("kind"));
            result.appId =
            fromJson<QString>(o.value("app_id"));
            result.appDisplayName =
            fromJson<QString>(o.value("app_display_name"));
            result.deviceDisplayName =
            fromJson<QString>(o.value("device_display_name"));
            result.profileTag =
            fromJson<QString>(o.value("profile_tag"));
            result.lang =
            fromJson<QString>(o.value("lang"));
            result.data =
            fromJson<GetPushersJob::PusherData>(o.value("data"));
            
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
    return BaseJob::makeRequestUrl(baseUrl,
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

PostPusherJob::PusherData::operator QJsonObject() const
{
    QJsonObject o;
    o.insert("url", toJson(url));
    
    return o;
}
namespace QMatrixClient
{
    template <> struct FromJson<PostPusherJob::PusherData>
    {
        PostPusherJob::PusherData operator()(QJsonValue jv)
        {
            QJsonObject o = jv.toObject();
            PostPusherJob::PusherData result;
            result.url =
            fromJson<QString>(o.value("url"));
            
            return result;
        }
    };
} // namespace QMatrixClient

PostPusherJob::PostPusherJob(const QString& pushkey, const QString& kind, const QString& appId, const QString& appDisplayName, const QString& deviceDisplayName, const QString& lang, const PusherData& data, const QString& profileTag, bool append)
    : BaseJob(HttpVerb::Post, "PostPusherJob",
        basePath % "/pushers/set")
{
    QJsonObject _data;
    _data.insert("pushkey", toJson(pushkey));
    _data.insert("kind", toJson(kind));
    _data.insert("app_id", toJson(appId));
    _data.insert("app_display_name", toJson(appDisplayName));
    _data.insert("device_display_name", toJson(deviceDisplayName));
    if (!profileTag.isEmpty())
        _data.insert("profile_tag", toJson(profileTag));
    _data.insert("lang", toJson(lang));
    _data.insert("data", toJson(data));
    _data.insert("append", toJson(append));
    setRequestData(_data);
}

