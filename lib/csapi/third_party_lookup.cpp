/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "third_party_lookup.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace Quotient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

class GetProtocolsJob::Private {
public:
    QHash<QString, ThirdPartyProtocol> data;
};

QUrl GetProtocolsJob::makeRequestUrl(QUrl baseUrl)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   basePath % "/thirdparty/protocols");
}

GetProtocolsJob::GetProtocolsJob()
    : BaseJob(HttpVerb::Get, QStringLiteral("GetProtocolsJob"),
              basePath % "/thirdparty/protocols")
    , d(new Private)
{}

GetProtocolsJob::~GetProtocolsJob() = default;

const QHash<QString, ThirdPartyProtocol>& GetProtocolsJob::data() const
{
    return d->data;
}

BaseJob::Status GetProtocolsJob::parseJson(const QJsonDocument& data)
{
    fromJson(data, d->data);

    return Success;
}

class GetProtocolMetadataJob::Private {
public:
    ThirdPartyProtocol data;
};

QUrl GetProtocolMetadataJob::makeRequestUrl(QUrl baseUrl,
                                            const QString& protocol)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   basePath % "/thirdparty/protocol/" % protocol);
}

GetProtocolMetadataJob::GetProtocolMetadataJob(const QString& protocol)
    : BaseJob(HttpVerb::Get, QStringLiteral("GetProtocolMetadataJob"),
              basePath % "/thirdparty/protocol/" % protocol)
    , d(new Private)
{}

GetProtocolMetadataJob::~GetProtocolMetadataJob() = default;

const ThirdPartyProtocol& GetProtocolMetadataJob::data() const
{
    return d->data;
}

BaseJob::Status GetProtocolMetadataJob::parseJson(const QJsonDocument& data)
{
    fromJson(data, d->data);

    return Success;
}

class QueryLocationByProtocolJob::Private {
public:
    QVector<ThirdPartyLocation> data;
};

BaseJob::Query queryToQueryLocationByProtocol(const QString& searchFields)
{
    BaseJob::Query _q;
    addParam<IfNotEmpty>(_q, QStringLiteral("searchFields"), searchFields);
    return _q;
}

QUrl QueryLocationByProtocolJob::makeRequestUrl(QUrl baseUrl,
                                                const QString& protocol,
                                                const QString& searchFields)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   basePath % "/thirdparty/location/" % protocol,
                                   queryToQueryLocationByProtocol(searchFields));
}

QueryLocationByProtocolJob::QueryLocationByProtocolJob(
    const QString& protocol, const QString& searchFields)
    : BaseJob(HttpVerb::Get, QStringLiteral("QueryLocationByProtocolJob"),
              basePath % "/thirdparty/location/" % protocol,
              queryToQueryLocationByProtocol(searchFields))
    , d(new Private)
{}

QueryLocationByProtocolJob::~QueryLocationByProtocolJob() = default;

const QVector<ThirdPartyLocation>& QueryLocationByProtocolJob::data() const
{
    return d->data;
}

BaseJob::Status QueryLocationByProtocolJob::parseJson(const QJsonDocument& data)
{
    fromJson(data, d->data);

    return Success;
}

class QueryUserByProtocolJob::Private {
public:
    QVector<ThirdPartyUser> data;
};

BaseJob::Query queryToQueryUserByProtocol(const QString& fields)
{
    BaseJob::Query _q;
    addParam<IfNotEmpty>(_q, QStringLiteral("fields..."), fields);
    return _q;
}

QUrl QueryUserByProtocolJob::makeRequestUrl(QUrl baseUrl,
                                            const QString& protocol,
                                            const QString& fields)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   basePath % "/thirdparty/user/" % protocol,
                                   queryToQueryUserByProtocol(fields));
}

QueryUserByProtocolJob::QueryUserByProtocolJob(const QString& protocol,
                                               const QString& fields)
    : BaseJob(HttpVerb::Get, QStringLiteral("QueryUserByProtocolJob"),
              basePath % "/thirdparty/user/" % protocol,
              queryToQueryUserByProtocol(fields))
    , d(new Private)
{}

QueryUserByProtocolJob::~QueryUserByProtocolJob() = default;

const QVector<ThirdPartyUser>& QueryUserByProtocolJob::data() const
{
    return d->data;
}

BaseJob::Status QueryUserByProtocolJob::parseJson(const QJsonDocument& data)
{
    fromJson(data, d->data);

    return Success;
}

class QueryLocationByAliasJob::Private {
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

QueryLocationByAliasJob::QueryLocationByAliasJob(const QString& alias)
    : BaseJob(HttpVerb::Get, QStringLiteral("QueryLocationByAliasJob"),
              basePath % "/thirdparty/location",
              queryToQueryLocationByAlias(alias))
    , d(new Private)
{}

QueryLocationByAliasJob::~QueryLocationByAliasJob() = default;

const QVector<ThirdPartyLocation>& QueryLocationByAliasJob::data() const
{
    return d->data;
}

BaseJob::Status QueryLocationByAliasJob::parseJson(const QJsonDocument& data)
{
    fromJson(data, d->data);

    return Success;
}

class QueryUserByIDJob::Private {
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

QueryUserByIDJob::QueryUserByIDJob(const QString& userid)
    : BaseJob(HttpVerb::Get, QStringLiteral("QueryUserByIDJob"),
              basePath % "/thirdparty/user", queryToQueryUserByID(userid))
    , d(new Private)
{}

QueryUserByIDJob::~QueryUserByIDJob() = default;

const QVector<ThirdPartyUser>& QueryUserByIDJob::data() const
{
    return d->data;
}

BaseJob::Status QueryUserByIDJob::parseJson(const QJsonDocument& data)
{
    fromJson(data, d->data);

    return Success;
}
