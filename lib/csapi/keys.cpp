/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "keys.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace Quotient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

class UploadKeysJob::Private {
public:
    QHash<QString, int> oneTimeKeyCounts;
};

UploadKeysJob::UploadKeysJob(const Omittable<DeviceKeys>& deviceKeys,
                             const QHash<QString, QVariant>& oneTimeKeys)
    : BaseJob(HttpVerb::Post, QStringLiteral("UploadKeysJob"),
              basePath % "/keys/upload")
    , d(new Private)
{
    QJsonObject _data;
    addParam<IfNotEmpty>(_data, QStringLiteral("device_keys"), deviceKeys);
    addParam<IfNotEmpty>(_data, QStringLiteral("one_time_keys"), oneTimeKeys);
    setRequestData(_data);
}

UploadKeysJob::~UploadKeysJob() = default;

const QHash<QString, int>& UploadKeysJob::oneTimeKeyCounts() const
{
    return d->oneTimeKeyCounts;
}

BaseJob::Status UploadKeysJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    if (!json.contains("one_time_key_counts"_ls))
        return { IncorrectResponse,
                 "The key 'one_time_key_counts' not found in the response" };
    fromJson(json.value("one_time_key_counts"_ls), d->oneTimeKeyCounts);

    return Success;
}

// Converters
namespace Quotient {

template <>
struct JsonObjectConverter<QueryKeysJob::UnsignedDeviceInfo> {
    static void fillFrom(const QJsonObject& jo,
                         QueryKeysJob::UnsignedDeviceInfo& result)
    {
        fromJson(jo.value("device_display_name"_ls), result.deviceDisplayName);
    }
};

template <>
struct JsonObjectConverter<QueryKeysJob::DeviceInformation> {
    static void fillFrom(const QJsonObject& jo,
                         QueryKeysJob::DeviceInformation& result)
    {
        fillFromJson<DeviceKeys>(jo, result);
        fromJson(jo.value("unsigned"_ls), result.unsignedData);
    }
};

} // namespace Quotient

class QueryKeysJob::Private {
public:
    QHash<QString, QJsonObject> failures;
    QHash<QString, QHash<QString, DeviceInformation>> deviceKeys;
};

QueryKeysJob::QueryKeysJob(const QHash<QString, QStringList>& deviceKeys,
                           Omittable<int> timeout, const QString& token)
    : BaseJob(HttpVerb::Post, QStringLiteral("QueryKeysJob"),
              basePath % "/keys/query")
    , d(new Private)
{
    QJsonObject _data;
    addParam<IfNotEmpty>(_data, QStringLiteral("timeout"), timeout);
    addParam<>(_data, QStringLiteral("device_keys"), deviceKeys);
    addParam<IfNotEmpty>(_data, QStringLiteral("token"), token);
    setRequestData(_data);
}

QueryKeysJob::~QueryKeysJob() = default;

const QHash<QString, QJsonObject>& QueryKeysJob::failures() const
{
    return d->failures;
}

const QHash<QString, QHash<QString, QueryKeysJob::DeviceInformation>>&
QueryKeysJob::deviceKeys() const
{
    return d->deviceKeys;
}

BaseJob::Status QueryKeysJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    fromJson(json.value("failures"_ls), d->failures);
    fromJson(json.value("device_keys"_ls), d->deviceKeys);

    return Success;
}

class ClaimKeysJob::Private {
public:
    QHash<QString, QJsonObject> failures;
    QHash<QString, QHash<QString, QVariant>> oneTimeKeys;
};

ClaimKeysJob::ClaimKeysJob(
    const QHash<QString, QHash<QString, QString>>& oneTimeKeys,
    Omittable<int> timeout)
    : BaseJob(HttpVerb::Post, QStringLiteral("ClaimKeysJob"),
              basePath % "/keys/claim")
    , d(new Private)
{
    QJsonObject _data;
    addParam<IfNotEmpty>(_data, QStringLiteral("timeout"), timeout);
    addParam<>(_data, QStringLiteral("one_time_keys"), oneTimeKeys);
    setRequestData(_data);
}

ClaimKeysJob::~ClaimKeysJob() = default;

const QHash<QString, QJsonObject>& ClaimKeysJob::failures() const
{
    return d->failures;
}

const QHash<QString, QHash<QString, QVariant>>& ClaimKeysJob::oneTimeKeys() const
{
    return d->oneTimeKeys;
}

BaseJob::Status ClaimKeysJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    fromJson(json.value("failures"_ls), d->failures);
    fromJson(json.value("one_time_keys"_ls), d->oneTimeKeys);

    return Success;
}

class GetKeysChangesJob::Private {
public:
    QStringList changed;
    QStringList left;
};

BaseJob::Query queryToGetKeysChanges(const QString& from, const QString& to)
{
    BaseJob::Query _q;
    addParam<>(_q, QStringLiteral("from"), from);
    addParam<>(_q, QStringLiteral("to"), to);
    return _q;
}

QUrl GetKeysChangesJob::makeRequestUrl(QUrl baseUrl, const QString& from,
                                       const QString& to)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   basePath % "/keys/changes",
                                   queryToGetKeysChanges(from, to));
}

GetKeysChangesJob::GetKeysChangesJob(const QString& from, const QString& to)
    : BaseJob(HttpVerb::Get, QStringLiteral("GetKeysChangesJob"),
              basePath % "/keys/changes", queryToGetKeysChanges(from, to))
    , d(new Private)
{}

GetKeysChangesJob::~GetKeysChangesJob() = default;

const QStringList& GetKeysChangesJob::changed() const { return d->changed; }

const QStringList& GetKeysChangesJob::left() const { return d->left; }

BaseJob::Status GetKeysChangesJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    fromJson(json.value("changed"_ls), d->changed);
    fromJson(json.value("left"_ls), d->left);

    return Success;
}
