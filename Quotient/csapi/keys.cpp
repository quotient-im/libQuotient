// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "keys.h"

using namespace Quotient;

UploadKeysJob::UploadKeysJob(const std::optional<DeviceKeys>& deviceKeys,
                             const OneTimeKeys& oneTimeKeys, const OneTimeKeys& fallbackKeys)
    : BaseJob(HttpVerb::Post, u"UploadKeysJob"_s, makePath("/_matrix/client/v3", "/keys/upload"))
{
    QJsonObject _dataJson;
    addParam<IfNotEmpty>(_dataJson, "device_keys"_L1, deviceKeys);
    addParam<IfNotEmpty>(_dataJson, "one_time_keys"_L1, oneTimeKeys);
    addParam<IfNotEmpty>(_dataJson, "fallback_keys"_L1, fallbackKeys);
    setRequestData({ _dataJson });
    addExpectedKey(u"one_time_key_counts"_s);
}

QueryKeysJob::QueryKeysJob(const QHash<UserId, QStringList>& deviceKeys, std::optional<int> timeout)
    : BaseJob(HttpVerb::Post, u"QueryKeysJob"_s, makePath("/_matrix/client/v3", "/keys/query"))
{
    QJsonObject _dataJson;
    addParam<IfNotEmpty>(_dataJson, "timeout"_L1, timeout);
    addParam<>(_dataJson, "device_keys"_L1, deviceKeys);
    setRequestData({ _dataJson });
}

ClaimKeysJob::ClaimKeysJob(const QHash<UserId, QHash<QString, QString>>& oneTimeKeys,
                           std::optional<int> timeout)
    : BaseJob(HttpVerb::Post, u"ClaimKeysJob"_s, makePath("/_matrix/client/v3", "/keys/claim"))
{
    QJsonObject _dataJson;
    addParam<IfNotEmpty>(_dataJson, "timeout"_L1, timeout);
    addParam<>(_dataJson, "one_time_keys"_L1, oneTimeKeys);
    setRequestData({ _dataJson });
    addExpectedKey(u"one_time_keys"_s);
}

auto queryToGetKeysChanges(const QString& from, const QString& to)
{
    QUrlQuery _q;
    addParam<>(_q, u"from"_s, from);
    addParam<>(_q, u"to"_s, to);
    return _q;
}

QUrl GetKeysChangesJob::makeRequestUrl(const HomeserverData& hsData, const QString& from,
                                       const QString& to)
{
    return BaseJob::makeRequestUrl(hsData, makePath("/_matrix/client/v3", "/keys/changes"),
                                   queryToGetKeysChanges(from, to));
}

GetKeysChangesJob::GetKeysChangesJob(const QString& from, const QString& to)
    : BaseJob(HttpVerb::Get, u"GetKeysChangesJob"_s,
              makePath("/_matrix/client/v3", "/keys/changes"), queryToGetKeysChanges(from, to))
{}
