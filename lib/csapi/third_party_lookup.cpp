/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "third_party_lookup.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

class GetProtocolsJob::Private
{
    public:
        QHash<QString, ThirdPartyProtocol> data;
};

QUrl GetProtocolsJob::makeRequestUrl(QUrl baseUrl)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
            basePath % "/thirdparty/protocols");
}

static const auto GetProtocolsJobName = QStringLiteral("GetProtocolsJob");

GetProtocolsJob::GetProtocolsJob()
    : BaseJob(HttpVerb::Get, GetProtocolsJobName,
        basePath % "/thirdparty/protocols", false)
    , d(new Private)
{
}

GetProtocolsJob::~GetProtocolsJob() = default;

const QHash<QString, ThirdPartyProtocol>& GetProtocolsJob::data() const
{
    return d->data;
}

BaseJob::Status GetProtocolsJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    if (!json.contains("data"_ls))
        return { JsonParseError,
            "The key 'data' not found in the response" };
    d->data = fromJson<QHash<QString, ThirdPartyProtocol>>(json.value("data"_ls));
    return Success;
}

class GetProtocolMetadataJob::Private
{
    public:
        ThirdPartyProtocol data;
};

QUrl GetProtocolMetadataJob::makeRequestUrl(QUrl baseUrl, const QString& protocol)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
            basePath % "/thirdparty/protocol/" % protocol);
}

static const auto GetProtocolMetadataJobName = QStringLiteral("GetProtocolMetadataJob");

GetProtocolMetadataJob::GetProtocolMetadataJob(const QString& protocol)
    : BaseJob(HttpVerb::Get, GetProtocolMetadataJobName,
        basePath % "/thirdparty/protocol/" % protocol, false)
    , d(new Private)
{
}

GetProtocolMetadataJob::~GetProtocolMetadataJob() = default;

const ThirdPartyProtocol& GetProtocolMetadataJob::data() const
{
    return d->data;
}

BaseJob::Status GetProtocolMetadataJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    if (!json.contains("data"_ls))
        return { JsonParseError,
            "The key 'data' not found in the response" };
    d->data = fromJson<ThirdPartyProtocol>(json.value("data"_ls));
    return Success;
}

class QueryLocationByProtocolJob::Private
{
    public:
        QVector<ThirdPartyLocation> data;
};

BaseJob::Query queryToQueryLocationByProtocol(const QString& searchFields)
{
    BaseJob::Query _q;
    addParam<IfNotEmpty>(_q, QStringLiteral("searchFields"), searchFields);
    return _q;
}

QUrl QueryLocationByProtocolJob::makeRequestUrl(QUrl baseUrl, const QString& protocol, const QString& searchFields)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
            basePath % "/thirdparty/location/" % protocol,
            queryToQueryLocationByProtocol(searchFields));
}

static const auto QueryLocationByProtocolJobName = QStringLiteral("QueryLocationByProtocolJob");

QueryLocationByProtocolJob::QueryLocationByProtocolJob(const QString& protocol, const QString& searchFields)
    : BaseJob(HttpVerb::Get, QueryLocationByProtocolJobName,
        basePath % "/thirdparty/location/" % protocol,
        queryToQueryLocationByProtocol(searchFields),
        {}, false)
    , d(new Private)
{
}

QueryLocationByProtocolJob::~QueryLocationByProtocolJob() = default;

const QVector<ThirdPartyLocation>& QueryLocationByProtocolJob::data() const
{
    return d->data;
}

BaseJob::Status QueryLocationByProtocolJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    if (!json.contains("data"_ls))
        return { JsonParseError,
            "The key 'data' not found in the response" };
    d->data = fromJson<QVector<ThirdPartyLocation>>(json.value("data"_ls));
    return Success;
}

class QueryUserByProtocolJob::Private
{
    public:
        QVector<ThirdPartyUser> data;
};

BaseJob::Query queryToQueryUserByProtocol(const QString& fields)
{
    BaseJob::Query _q;
    addParam<IfNotEmpty>(_q, QStringLiteral("fields..."), fields);
    return _q;
}

QUrl QueryUserByProtocolJob::makeRequestUrl(QUrl baseUrl, const QString& protocol, const QString& fields)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
            basePath % "/thirdparty/user/" % protocol,
            queryToQueryUserByProtocol(fields));
}

static const auto QueryUserByProtocolJobName = QStringLiteral("QueryUserByProtocolJob");

QueryUserByProtocolJob::QueryUserByProtocolJob(const QString& protocol, const QString& fields)
    : BaseJob(HttpVerb::Get, QueryUserByProtocolJobName,
        basePath % "/thirdparty/user/" % protocol,
        queryToQueryUserByProtocol(fields),
        {}, false)
    , d(new Private)
{
}

QueryUserByProtocolJob::~QueryUserByProtocolJob() = default;

const QVector<ThirdPartyUser>& QueryUserByProtocolJob::data() const
{
    return d->data;
}

BaseJob::Status QueryUserByProtocolJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    if (!json.contains("data"_ls))
        return { JsonParseError,
            "The key 'data' not found in the response" };
    d->data = fromJson<QVector<ThirdPartyUser>>(json.value("data"_ls));
    return Success;
}

class QueryLocationByAliasJob::Private
{
    public:
        QVector<ThirdPartyLocation> data;
};

BaseJob::Query queryToQueryLocationByAlias(const QString& alias)
{
    BaseJob::Query _q;
    addParam<>(_q, QStringLiteral("alias"), alias);
    return _q;
}

QUrl QueryLocationByAliasJob::makeRequestUrl(QUrl baseUrl, const QString& alias)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
            basePath % "/thirdparty/location",
            queryToQueryLocationByAlias(alias));
}

static const auto QueryLocationByAliasJobName = QStringLiteral("QueryLocationByAliasJob");

QueryLocationByAliasJob::QueryLocationByAliasJob(const QString& alias)
    : BaseJob(HttpVerb::Get, QueryLocationByAliasJobName,
        basePath % "/thirdparty/location",
        queryToQueryLocationByAlias(alias),
        {}, false)
    , d(new Private)
{
}

QueryLocationByAliasJob::~QueryLocationByAliasJob() = default;

const QVector<ThirdPartyLocation>& QueryLocationByAliasJob::data() const
{
    return d->data;
}

BaseJob::Status QueryLocationByAliasJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    if (!json.contains("data"_ls))
        return { JsonParseError,
            "The key 'data' not found in the response" };
    d->data = fromJson<QVector<ThirdPartyLocation>>(json.value("data"_ls));
    return Success;
}

class QueryUserByIDJob::Private
{
    public:
        QVector<ThirdPartyUser> data;
};

BaseJob::Query queryToQueryUserByID(const QString& userid)
{
    BaseJob::Query _q;
    addParam<>(_q, QStringLiteral("userid"), userid);
    return _q;
}

QUrl QueryUserByIDJob::makeRequestUrl(QUrl baseUrl, const QString& userid)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
            basePath % "/thirdparty/user",
            queryToQueryUserByID(userid));
}

static const auto QueryUserByIDJobName = QStringLiteral("QueryUserByIDJob");

QueryUserByIDJob::QueryUserByIDJob(const QString& userid)
    : BaseJob(HttpVerb::Get, QueryUserByIDJobName,
        basePath % "/thirdparty/user",
        queryToQueryUserByID(userid),
        {}, false)
    , d(new Private)
{
}

QueryUserByIDJob::~QueryUserByIDJob() = default;

const QVector<ThirdPartyUser>& QueryUserByIDJob::data() const
{
    return d->data;
}

BaseJob::Status QueryUserByIDJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    if (!json.contains("data"_ls))
        return { JsonParseError,
            "The key 'data' not found in the response" };
    d->data = fromJson<QVector<ThirdPartyUser>>(json.value("data"_ls));
    return Success;
}

