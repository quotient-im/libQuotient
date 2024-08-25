// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "pusher.h"

using namespace Quotient;

QUrl GetPushersJob::makeRequestUrl(const HomeserverData& hsData)
{
    return BaseJob::makeRequestUrl(hsData, makePath("/_matrix/client/v3", "/pushers"));
}

GetPushersJob::GetPushersJob()
    : BaseJob(HttpVerb::Get, u"GetPushersJob"_s, makePath("/_matrix/client/v3", "/pushers"))
{}

PostPusherJob::PostPusherJob(const QString& pushkey, const QString& kind, const QString& appId,
                             const QString& appDisplayName, const QString& deviceDisplayName,
                             const QString& profileTag, const QString& lang,
                             const std::optional<PusherData>& data, std::optional<bool> append)
    : BaseJob(HttpVerb::Post, u"PostPusherJob"_s, makePath("/_matrix/client/v3", "/pushers/set"))
{
    QJsonObject _dataJson;
    addParam<>(_dataJson, "pushkey"_L1, pushkey);
    addParam<>(_dataJson, "kind"_L1, kind);
    addParam<>(_dataJson, "app_id"_L1, appId);
    addParam<IfNotEmpty>(_dataJson, "app_display_name"_L1, appDisplayName);
    addParam<IfNotEmpty>(_dataJson, "device_display_name"_L1, deviceDisplayName);
    addParam<IfNotEmpty>(_dataJson, "profile_tag"_L1, profileTag);
    addParam<IfNotEmpty>(_dataJson, "lang"_L1, lang);
    addParam<IfNotEmpty>(_dataJson, "data"_L1, data);
    addParam<IfNotEmpty>(_dataJson, "append"_L1, append);
    setRequestData({ _dataJson });
}
