/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "pushrules.h"

using namespace Quotient;

QUrl GetPushRulesJob::makeRequestUrl(QUrl baseUrl)
{
    return BaseJob::makeRequestUrl(
        std::move(baseUrl), makePath("/_matrix/client/v3", "/pushrules"));
}

GetPushRulesJob::GetPushRulesJob()
    : BaseJob(HttpVerb::Get, QStringLiteral("GetPushRulesJob"),
              makePath("/_matrix/client/v3", "/pushrules"))
{
    addExpectedKey("global");
}

QUrl GetPushRuleJob::makeRequestUrl(QUrl baseUrl, const QString& scope,
                                    const QString& kind, const QString& ruleId)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   makePath("/_matrix/client/v3", "/pushrules/",
                                            scope, "/", kind, "/", ruleId));
}

GetPushRuleJob::GetPushRuleJob(const QString& scope, const QString& kind,
                               const QString& ruleId)
    : BaseJob(HttpVerb::Get, QStringLiteral("GetPushRuleJob"),
              makePath("/_matrix/client/v3", "/pushrules/", scope, "/", kind,
                       "/", ruleId))
{}

QUrl DeletePushRuleJob::makeRequestUrl(QUrl baseUrl, const QString& scope,
                                       const QString& kind,
                                       const QString& ruleId)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   makePath("/_matrix/client/v3", "/pushrules/",
                                            scope, "/", kind, "/", ruleId));
}

DeletePushRuleJob::DeletePushRuleJob(const QString& scope, const QString& kind,
                                     const QString& ruleId)
    : BaseJob(HttpVerb::Delete, QStringLiteral("DeletePushRuleJob"),
              makePath("/_matrix/client/v3", "/pushrules/", scope, "/", kind,
                       "/", ruleId))
{}

auto queryToSetPushRule(const QString& before, const QString& after)
{
    QUrlQuery _q;
    addParam<IfNotEmpty>(_q, QStringLiteral("before"), before);
    addParam<IfNotEmpty>(_q, QStringLiteral("after"), after);
    return _q;
}

SetPushRuleJob::SetPushRuleJob(const QString& scope, const QString& kind,
                               const QString& ruleId,
                               const QVector<QVariant>& actions,
                               const QString& before, const QString& after,
                               const QVector<PushCondition>& conditions,
                               const QString& pattern)
    : BaseJob(HttpVerb::Put, QStringLiteral("SetPushRuleJob"),
              makePath("/_matrix/client/v3", "/pushrules/", scope, "/", kind,
                       "/", ruleId),
              queryToSetPushRule(before, after))
{
    QJsonObject _dataJson;
    addParam<>(_dataJson, QStringLiteral("actions"), actions);
    addParam<IfNotEmpty>(_dataJson, QStringLiteral("conditions"), conditions);
    addParam<IfNotEmpty>(_dataJson, QStringLiteral("pattern"), pattern);
    setRequestData({ _dataJson });
}

QUrl IsPushRuleEnabledJob::makeRequestUrl(QUrl baseUrl, const QString& scope,
                                          const QString& kind,
                                          const QString& ruleId)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   makePath("/_matrix/client/v3", "/pushrules/",
                                            scope, "/", kind, "/", ruleId,
                                            "/enabled"));
}

IsPushRuleEnabledJob::IsPushRuleEnabledJob(const QString& scope,
                                           const QString& kind,
                                           const QString& ruleId)
    : BaseJob(HttpVerb::Get, QStringLiteral("IsPushRuleEnabledJob"),
              makePath("/_matrix/client/v3", "/pushrules/", scope, "/", kind,
                       "/", ruleId, "/enabled"))
{
    addExpectedKey("enabled");
}

SetPushRuleEnabledJob::SetPushRuleEnabledJob(const QString& scope,
                                             const QString& kind,
                                             const QString& ruleId, bool enabled)
    : BaseJob(HttpVerb::Put, QStringLiteral("SetPushRuleEnabledJob"),
              makePath("/_matrix/client/v3", "/pushrules/", scope, "/", kind,
                       "/", ruleId, "/enabled"))
{
    QJsonObject _dataJson;
    addParam<>(_dataJson, QStringLiteral("enabled"), enabled);
    setRequestData({ _dataJson });
}

QUrl GetPushRuleActionsJob::makeRequestUrl(QUrl baseUrl, const QString& scope,
                                           const QString& kind,
                                           const QString& ruleId)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   makePath("/_matrix/client/v3", "/pushrules/",
                                            scope, "/", kind, "/", ruleId,
                                            "/actions"));
}

GetPushRuleActionsJob::GetPushRuleActionsJob(const QString& scope,
                                             const QString& kind,
                                             const QString& ruleId)
    : BaseJob(HttpVerb::Get, QStringLiteral("GetPushRuleActionsJob"),
              makePath("/_matrix/client/v3", "/pushrules/", scope, "/", kind,
                       "/", ruleId, "/actions"))
{
    addExpectedKey("actions");
}

SetPushRuleActionsJob::SetPushRuleActionsJob(const QString& scope,
                                             const QString& kind,
                                             const QString& ruleId,
                                             const QVector<QVariant>& actions)
    : BaseJob(HttpVerb::Put, QStringLiteral("SetPushRuleActionsJob"),
              makePath("/_matrix/client/v3", "/pushrules/", scope, "/", kind,
                       "/", ruleId, "/actions"))
{
    QJsonObject _dataJson;
    addParam<>(_dataJson, QStringLiteral("actions"), actions);
    setRequestData({ _dataJson });
}
