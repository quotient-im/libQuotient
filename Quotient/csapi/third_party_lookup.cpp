// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "third_party_lookup.h"

using namespace Quotient;

QUrl GetProtocolsJob::makeRequestUrl(const HomeserverData& hsData)
{
    return BaseJob::makeRequestUrl(hsData, makePath("/_matrix/client/v3", "/thirdparty/protocols"));
}

GetProtocolsJob::GetProtocolsJob()
    : BaseJob(HttpVerb::Get, u"GetProtocolsJob"_s,
              makePath("/_matrix/client/v3", "/thirdparty/protocols"))
{}

QUrl GetProtocolMetadataJob::makeRequestUrl(const HomeserverData& hsData, const QString& protocol)
{
    return BaseJob::makeRequestUrl(hsData, makePath("/_matrix/client/v3", "/thirdparty/protocol/",
                                                    protocol));
}

GetProtocolMetadataJob::GetProtocolMetadataJob(const QString& protocol)
    : BaseJob(HttpVerb::Get, u"GetProtocolMetadataJob"_s,
              makePath("/_matrix/client/v3", "/thirdparty/protocol/", protocol))
{}

auto queryToQueryLocationByProtocol(const QString& searchFields)
{
    QUrlQuery _q;
    addParam<IfNotEmpty>(_q, u"searchFields"_s, searchFields);
    return _q;
}

QUrl QueryLocationByProtocolJob::makeRequestUrl(const HomeserverData& hsData,
                                                const QString& protocol, const QString& searchFields)
{
    return BaseJob::makeRequestUrl(hsData,
                                   makePath("/_matrix/client/v3", "/thirdparty/location/", protocol),
                                   queryToQueryLocationByProtocol(searchFields));
}

QueryLocationByProtocolJob::QueryLocationByProtocolJob(const QString& protocol,
                                                       const QString& searchFields)
    : BaseJob(HttpVerb::Get, u"QueryLocationByProtocolJob"_s,
              makePath("/_matrix/client/v3", "/thirdparty/location/", protocol),
              queryToQueryLocationByProtocol(searchFields))
{}

auto queryToQueryUserByProtocol(const QHash<QString, QString>& fields)
{
    QUrlQuery _q;
    addParam<IfNotEmpty>(_q, u"fields"_s, fields);
    return _q;
}

QUrl QueryUserByProtocolJob::makeRequestUrl(const HomeserverData& hsData, const QString& protocol,
                                            const QHash<QString, QString>& fields)
{
    return BaseJob::makeRequestUrl(hsData,
                                   makePath("/_matrix/client/v3", "/thirdparty/user/", protocol),
                                   queryToQueryUserByProtocol(fields));
}

QueryUserByProtocolJob::QueryUserByProtocolJob(const QString& protocol,
                                               const QHash<QString, QString>& fields)
    : BaseJob(HttpVerb::Get, u"QueryUserByProtocolJob"_s,
              makePath("/_matrix/client/v3", "/thirdparty/user/", protocol),
              queryToQueryUserByProtocol(fields))
{}

auto queryToQueryLocationByAlias(const QString& alias)
{
    QUrlQuery _q;
    addParam<>(_q, u"alias"_s, alias);
    return _q;
}

QUrl QueryLocationByAliasJob::makeRequestUrl(const HomeserverData& hsData, const QString& alias)
{
    return BaseJob::makeRequestUrl(hsData, makePath("/_matrix/client/v3", "/thirdparty/location"),
                                   queryToQueryLocationByAlias(alias));
}

QueryLocationByAliasJob::QueryLocationByAliasJob(const QString& alias)
    : BaseJob(HttpVerb::Get, u"QueryLocationByAliasJob"_s,
              makePath("/_matrix/client/v3", "/thirdparty/location"),
              queryToQueryLocationByAlias(alias))
{}

auto queryToQueryUserByID(const QString& userid)
{
    QUrlQuery _q;
    addParam<>(_q, u"userid"_s, userid);
    return _q;
}

QUrl QueryUserByIDJob::makeRequestUrl(const HomeserverData& hsData, const QString& userid)
{
    return BaseJob::makeRequestUrl(hsData, makePath("/_matrix/client/v3", "/thirdparty/user"),
                                   queryToQueryUserByID(userid));
}

QueryUserByIDJob::QueryUserByIDJob(const QString& userid)
    : BaseJob(HttpVerb::Get, u"QueryUserByIDJob"_s,
              makePath("/_matrix/client/v3", "/thirdparty/user"), queryToQueryUserByID(userid))
{}
