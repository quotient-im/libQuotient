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
    addExpectedKey(u"global"_s);
}

QUrl GetPushRulesGlobalJob::makeRequestUrl(const HomeserverData& hsData)
{
    return BaseJob::makeRequestUrl(hsData, makePath("/_matrix/client/v3", "/pushrules/global"));
}

GetPushRulesGlobalJob::GetPushRulesGlobalJob()
    : BaseJob(HttpVerb::Get, u"GetPushRulesGlobalJob"_s,
              makePath("/_matrix/client/v3", "/pushrules/global"))
{}

QUrl GetPushRuleJob::makeRequestUrl(const HomeserverData& hsData, const QString& kind,
                                    const QString& ruleId)
{
    return BaseJob::makeRequestUrl(hsData, makePath("/_matrix/client/v3", "/pushrules/global/",
                                                    kind, "/", ruleId));
}

GetPushRuleJob::GetPushRuleJob(const QString& kind, const QString& ruleId)
    : BaseJob(HttpVerb::Get, u"GetPushRuleJob"_s,
              makePath("/_matrix/client/v3", "/pushrules/global/", kind, "/", ruleId))
{}

QUrl DeletePushRuleJob::makeRequestUrl(const HomeserverData& hsData, const QString& kind,
                                       const QString& ruleId)
{
    return BaseJob::makeRequestUrl(hsData, makePath("/_matrix/client/v3", "/pushrules/global/",
                                                    kind, "/", ruleId));
}

DeletePushRuleJob::DeletePushRuleJob(const QString& kind, const QString& ruleId)
    : BaseJob(HttpVerb::Delete, u"DeletePushRuleJob"_s,
              makePath("/_matrix/client/v3", "/pushrules/global/", kind, "/", ruleId))
{}

auto queryToSetPushRule(const QString& before, const QString& after)
{
    QUrlQuery _q;
    addParam<IfNotEmpty>(_q, u"before"_s, before);
    addParam<IfNotEmpty>(_q, u"after"_s, after);
    return _q;
}

SetPushRuleJob::SetPushRuleJob(const QString& kind, const QString& ruleId,
                               const QVector<QVariant>& actions, const QString& before,
                               const QString& after, const QVector<PushCondition>& conditions,
                               const QString& pattern)
    : BaseJob(HttpVerb::Put, u"SetPushRuleJob"_s,
              makePath("/_matrix/client/v3", "/pushrules/global/", kind, "/", ruleId),
              queryToSetPushRule(before, after))
{
    QJsonObject _dataJson;
    addParam<>(_dataJson, "actions"_L1, actions);
    addParam<IfNotEmpty>(_dataJson, "conditions"_L1, conditions);
    addParam<IfNotEmpty>(_dataJson, "pattern"_L1, pattern);
    setRequestData({ _dataJson });
}

QUrl IsPushRuleEnabledJob::makeRequestUrl(const HomeserverData& hsData, const QString& kind,
                                          const QString& ruleId)
{
    return BaseJob::makeRequestUrl(hsData, makePath("/_matrix/client/v3", "/pushrules/global/",
                                                    kind, "/", ruleId, "/enabled"));
}

IsPushRuleEnabledJob::IsPushRuleEnabledJob(const QString& kind, const QString& ruleId)
    : BaseJob(HttpVerb::Get, u"IsPushRuleEnabledJob"_s,
              makePath("/_matrix/client/v3", "/pushrules/global/", kind, "/", ruleId, "/enabled"))
{
    addExpectedKey(u"enabled"_s);
}

SetPushRuleEnabledJob::SetPushRuleEnabledJob(const QString& kind, const QString& ruleId,
                                             bool enabled)
    : BaseJob(HttpVerb::Put, u"SetPushRuleEnabledJob"_s,
              makePath("/_matrix/client/v3", "/pushrules/global/", kind, "/", ruleId, "/enabled"))
{
    QJsonObject _dataJson;
    addParam<>(_dataJson, "enabled"_L1, enabled);
    setRequestData({ _dataJson });
}

QUrl GetPushRuleActionsJob::makeRequestUrl(const HomeserverData& hsData, const QString& kind,
                                           const QString& ruleId)
{
    return BaseJob::makeRequestUrl(hsData, makePath("/_matrix/client/v3", "/pushrules/global/",
                                                    kind, "/", ruleId, "/actions"));
}

GetPushRuleActionsJob::GetPushRuleActionsJob(const QString& kind, const QString& ruleId)
    : BaseJob(HttpVerb::Get, u"GetPushRuleActionsJob"_s,
              makePath("/_matrix/client/v3", "/pushrules/global/", kind, "/", ruleId, "/actions"))
{
    addExpectedKey(u"actions"_s);
}

SetPushRuleActionsJob::SetPushRuleActionsJob(const QString& kind, const QString& ruleId,
                                             const QVector<QVariant>& actions)
    : BaseJob(HttpVerb::Put, u"SetPushRuleActionsJob"_s,
              makePath("/_matrix/client/v3", "/pushrules/global/", kind, "/", ruleId, "/actions"))
{
    QJsonObject _dataJson;
    addParam<>(_dataJson, "actions"_L1, actions);
    setRequestData({ _dataJson });
}
