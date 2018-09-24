/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "keys.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

class UploadKeysJob::Private
{
    public:
        QHash<QString, int> oneTimeKeyCounts;
};

static const auto UploadKeysJobName = QStringLiteral("UploadKeysJob");

UploadKeysJob::UploadKeysJob(const Omittable<DeviceKeys>& deviceKeys, const QHash<QString, QVariant>& oneTimeKeys)
    : BaseJob(HttpVerb::Post, UploadKeysJobName,
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
        return { JsonParseError,
            "The key 'one_time_key_counts' not found in the response" };
    d->oneTimeKeyCounts = fromJson<QHash<QString, int>>(json.value("one_time_key_counts"_ls));
    return Success;
}

namespace QMatrixClient
{
    // Converters

    template <> struct FromJsonObject<QueryKeysJob::UnsignedDeviceInfo>
    {
        QueryKeysJob::UnsignedDeviceInfo operator()(const QJsonObject& jo) const
        {
            QueryKeysJob::UnsignedDeviceInfo result;
            result.deviceDisplayName =
                fromJson<QString>(jo.value("device_display_name"_ls));

            return result;
        }
    };

    template <> struct FromJsonObject<QueryKeysJob::DeviceInformation>
    {
        QueryKeysJob::DeviceInformation operator()(const QJsonObject& jo) const
        {
            QueryKeysJob::DeviceInformation result;
            result.unsignedData =
                fromJson<QueryKeysJob::UnsignedDeviceInfo>(jo.value("unsigned"_ls));

            return result;
        }
    };
} // namespace QMatrixClient

class QueryKeysJob::Private
{
    public:
        QHash<QString, QJsonObject> failures;
        QHash<QString, QHash<QString, DeviceInformation>> deviceKeys;
};

static const auto QueryKeysJobName = QStringLiteral("QueryKeysJob");

QueryKeysJob::QueryKeysJob(const QHash<QString, QStringList>& deviceKeys, Omittable<int> timeout, const QString& token)
    : BaseJob(HttpVerb::Post, QueryKeysJobName,
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

const QHash<QString, QHash<QString, QueryKeysJob::DeviceInformation>>& QueryKeysJob::deviceKeys() const
{
    return d->deviceKeys;
}

BaseJob::Status QueryKeysJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    d->failures = fromJson<QHash<QString, QJsonObject>>(json.value("failures"_ls));
    d->deviceKeys = fromJson<QHash<QString, QHash<QString, DeviceInformation>>>(json.value("device_keys"_ls));
    return Success;
}

class ClaimKeysJob::Private
{
    public:
        QHash<QString, QJsonObject> failures;
        QHash<QString, QHash<QString, QVariant>> oneTimeKeys;
};

static const auto ClaimKeysJobName = QStringLiteral("ClaimKeysJob");

ClaimKeysJob::ClaimKeysJob(const QHash<QString, QHash<QString, QString>>& oneTimeKeys, Omittable<int> timeout)
    : BaseJob(HttpVerb::Post, ClaimKeysJobName,
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
    d->failures = fromJson<QHash<QString, QJsonObject>>(json.value("failures"_ls));
    d->oneTimeKeys = fromJson<QHash<QString, QHash<QString, QVariant>>>(json.value("one_time_keys"_ls));
    return Success;
}

class GetKeysChangesJob::Private
{
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

QUrl GetKeysChangesJob::makeRequestUrl(QUrl baseUrl, const QString& from, const QString& to)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
            basePath % "/keys/changes",
            queryToGetKeysChanges(from, to));
}

static const auto GetKeysChangesJobName = QStringLiteral("GetKeysChangesJob");

GetKeysChangesJob::GetKeysChangesJob(const QString& from, const QString& to)
    : BaseJob(HttpVerb::Get, GetKeysChangesJobName,
        basePath % "/keys/changes",
        queryToGetKeysChanges(from, to))
    , d(new Private)
{
}

GetKeysChangesJob::~GetKeysChangesJob() = default;

const QStringList& GetKeysChangesJob::changed() const
{
    return d->changed;
}

const QStringList& GetKeysChangesJob::left() const
{
    return d->left;
}

BaseJob::Status GetKeysChangesJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    d->changed = fromJson<QStringList>(json.value("changed"_ls));
    d->left = fromJson<QStringList>(json.value("left"_ls));
    return Success;
}

