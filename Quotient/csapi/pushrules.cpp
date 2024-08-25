// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "pushrules.h"

using namespace Quotient;

QUrl GetPushRulesJob::makeRequestUrl(const HomeserverData& hsData)
{
    return BaseJob::makeRequestUrl(hsData, makePath("/_matrix/client/v3", "/pushrules"));
}

GetPushRulesJob::GetPushRulesJob()
    : BaseJob(HttpVerb::Get, u"GetPushRulesJob"_s, makePath("/_matrix/client/v3", "/pushrules"))
{
    addExpectedKey("global");
}

QUrl GetPushRuleJob::makeRequestUrl(const HomeserverData& hsData, const QString& scope,
                                    const QString& kind, const QString& ruleId)
{
    return BaseJob::makeRequestUrl(hsData, makePath("/_matrix/client/v3", "/pushrules/", scope, "/",
                                                    kind, "/", ruleId));
}

GetPushRuleJob::GetPushRuleJob(const QString& scope, const QString& kind, const QString& ruleId)
    : BaseJob(HttpVerb::Get, u"GetPushRuleJob"_s,
              makePath("/_matrix/client/v3", "/pushrules/", scope, "/", kind, "/", ruleId))
{}

QUrl DeletePushRuleJob::makeRequestUrl(const HomeserverData& hsData, const QString& scope,
                                       const QString& kind, const QString& ruleId)
{
    return BaseJob::makeRequestUrl(hsData, makePath("/_matrix/client/v3", "/pushrules/", scope, "/",
                                                    kind, "/", ruleId));
}

DeletePushRuleJob::DeletePushRuleJob(const QString& scope, const QString& kind,
                                     const QString& ruleId)
    : BaseJob(HttpVerb::Delete, u"DeletePushRuleJob"_s,
              makePath("/_matrix/client/v3", "/pushrules/", scope, "/", kind, "/", ruleId))
{}

auto queryToSetPushRule(const QString& before, const QString& after)
{
    QUrlQuery _q;
    addParam<IfNotEmpty>(_q, u"before"_s, before);
    addParam<IfNotEmpty>(_q, u"after"_s, after);
    return _q;
}

SetPushRuleJob::SetPushRuleJob(const QString& scope, const QString& kind, const QString& ruleId,
                               const QVector<QVariant>& actions, const QString& before,
                               const QString& after, const QVector<PushCondition>& conditions,
                               const QString& pattern)
    : BaseJob(HttpVerb::Put, u"SetPushRuleJob"_s,
              makePath("/_matrix/client/v3", "/pushrules/", scope, "/", kind, "/", ruleId),
              queryToSetPushRule(before, after))
{
    QJsonObject _dataJson;
    addParam<>(_dataJson, "actions"_L1, actions);
    addParam<IfNotEmpty>(_dataJson, "conditions"_L1, conditions);
    addParam<IfNotEmpty>(_dataJson, "pattern"_L1, pattern);
    setRequestData({ _dataJson });
}

QUrl IsPushRuleEnabledJob::makeRequestUrl(const HomeserverData& hsData, const QString& scope,
                                          const QString& kind, const QString& ruleId)
{
    return BaseJob::makeRequestUrl(hsData, makePath("/_matrix/client/v3", "/pushrules/", scope, "/",
                                                    kind, "/", ruleId, "/enabled"));
}

IsPushRuleEnabledJob::IsPushRuleEnabledJob(const QString& scope, const QString& kind,
                                           const QString& ruleId)
    : BaseJob(HttpVerb::Get, u"IsPushRuleEnabledJob"_s,
              makePath("/_matrix/client/v3", "/pushrules/", scope, "/", kind, "/", ruleId,
                       "/enabled"))
{
    addExpectedKey("enabled");
}

SetPushRuleEnabledJob::SetPushRuleEnabledJob(const QString& scope, const QString& kind,
                                             const QString& ruleId, bool enabled)
    : BaseJob(HttpVerb::Put, u"SetPushRuleEnabledJob"_s,
              makePath("/_matrix/client/v3", "/pushrules/", scope, "/", kind, "/", ruleId,
                       "/enabled"))
{
    QJsonObject _dataJson;
    addParam<>(_dataJson, "enabled"_L1, enabled);
    setRequestData({ _dataJson });
}

QUrl GetPushRuleActionsJob::makeRequestUrl(const HomeserverData& hsData, const QString& scope,
                                           const QString& kind, const QString& ruleId)
{
    return BaseJob::makeRequestUrl(hsData, makePath("/_matrix/client/v3", "/pushrules/", scope, "/",
                                                    kind, "/", ruleId, "/actions"));
}

GetPushRuleActionsJob::GetPushRuleActionsJob(const QString& scope, const QString& kind,
                                             const QString& ruleId)
    : BaseJob(HttpVerb::Get, u"GetPushRuleActionsJob"_s,
              makePath("/_matrix/client/v3", "/pushrules/", scope, "/", kind, "/", ruleId,
                       "/actions"))
{
    addExpectedKey("actions");
}

SetPushRuleActionsJob::SetPushRuleActionsJob(const QString& scope, const QString& kind,
                                             const QString& ruleId, const QVector<QVariant>& actions)
    : BaseJob(HttpVerb::Put, u"SetPushRuleActionsJob"_s,
              makePath("/_matrix/client/v3", "/pushrules/", scope, "/", kind, "/", ruleId,
                       "/actions"))
{
    QJsonObject _dataJson;
    addParam<>(_dataJson, "actions"_L1, actions);
    setRequestData({ _dataJson });
}
