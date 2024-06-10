// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "keys.h"

using namespace Quotient;

UploadKeysJob::UploadKeysJob(const std::optional<DeviceKeys>& deviceKeys,
                             const OneTimeKeys& oneTimeKeys, const OneTimeKeys& fallbackKeys)
    : BaseJob(HttpVerb::Post, QStringLiteral("UploadKeysJob"),
              makePath("/_matrix/client/v3", "/keys/upload"))
{
    QJsonObject _dataJson;
    addParam<IfNotEmpty>(_dataJson, QStringLiteral("device_keys"), deviceKeys);
    addParam<IfNotEmpty>(_dataJson, QStringLiteral("one_time_keys"), oneTimeKeys);
    addParam<IfNotEmpty>(_dataJson, QStringLiteral("fallback_keys"), fallbackKeys);
    setRequestData({ _dataJson });
    addExpectedKey("one_time_key_counts");
}

QueryKeysJob::QueryKeysJob(const QHash<UserId, QStringList>& deviceKeys, std::optional<int> timeout)
    : BaseJob(HttpVerb::Post, QStringLiteral("QueryKeysJob"),
              makePath("/_matrix/client/v3", "/keys/query"))
{
    QJsonObject _dataJson;
    addParam<IfNotEmpty>(_dataJson, QStringLiteral("timeout"), timeout);
    addParam<>(_dataJson, QStringLiteral("device_keys"), deviceKeys);
    setRequestData({ _dataJson });
}

ClaimKeysJob::ClaimKeysJob(const QHash<UserId, QHash<QString, QString>>& oneTimeKeys,
                           std::optional<int> timeout)
    : BaseJob(HttpVerb::Post, QStringLiteral("ClaimKeysJob"),
              makePath("/_matrix/client/v3", "/keys/claim"))
{
    QJsonObject _dataJson;
    addParam<IfNotEmpty>(_dataJson, QStringLiteral("timeout"), timeout);
    addParam<>(_dataJson, QStringLiteral("one_time_keys"), oneTimeKeys);
    setRequestData({ _dataJson });
    addExpectedKey("one_time_keys");
}

auto queryToGetKeysChanges(const QString& from, const QString& to)
{
    QUrlQuery _q;
    addParam<>(_q, QStringLiteral("from"), from);
    addParam<>(_q, QStringLiteral("to"), to);
    return _q;
}

QUrl GetKeysChangesJob::makeRequestUrl(QUrl baseUrl, const QString& from, const QString& to)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   makePath("/_matrix/client/v3", "/keys/changes"),
                                   queryToGetKeysChanges(from, to));
}

GetKeysChangesJob::GetKeysChangesJob(const QString& from, const QString& to)
    : BaseJob(HttpVerb::Get, QStringLiteral("GetKeysChangesJob"),
              makePath("/_matrix/client/v3", "/keys/changes"), queryToGetKeysChanges(from, to))
{}
