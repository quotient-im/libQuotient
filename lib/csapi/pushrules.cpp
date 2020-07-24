/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "pushrules.h"

#include <QtCore/QStringBuilder>

using namespace Quotient;

QUrl GetPushRulesJob::makeRequestUrl(QUrl baseUrl)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   QStringLiteral("/_matrix/client/r0")
                                       % "/pushrules");
}

GetPushRulesJob::GetPushRulesJob()
    : BaseJob(HttpVerb::Get, QStringLiteral("GetPushRulesJob"),
              QStringLiteral("/_matrix/client/r0") % "/pushrules")
{
    addExpectedKey("global");
}

QUrl GetPushRuleJob::makeRequestUrl(QUrl baseUrl, const QString& scope,
                                    const QString& kind, const QString& ruleId)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   QStringLiteral("/_matrix/client/r0")
                                       % "/pushrules/" % scope % "/" % kind
                                       % "/" % ruleId);
}

GetPushRuleJob::GetPushRuleJob(const QString& scope, const QString& kind,
                               const QString& ruleId)
    : BaseJob(HttpVerb::Get, QStringLiteral("GetPushRuleJob"),
              QStringLiteral("/_matrix/client/r0") % "/pushrules/" % scope % "/"
                  % kind % "/" % ruleId)
{}

QUrl DeletePushRuleJob::makeRequestUrl(QUrl baseUrl, const QString& scope,
                                       const QString& kind,
                                       const QString& ruleId)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   QStringLiteral("/_matrix/client/r0")
                                       % "/pushrules/" % scope % "/" % kind
                                       % "/" % ruleId);
}

DeletePushRuleJob::DeletePushRuleJob(const QString& scope, const QString& kind,
                                     const QString& ruleId)
    : BaseJob(HttpVerb::Delete, QStringLiteral("DeletePushRuleJob"),
              QStringLiteral("/_matrix/client/r0") % "/pushrules/" % scope % "/"
                  % kind % "/" % ruleId)
{}

auto queryToSetPushRule(const QString& before, const QString& after)
{
    BaseJob::Query _q;
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
              QStringLiteral("/_matrix/client/r0") % "/pushrules/" % scope % "/"
                  % kind % "/" % ruleId,
              queryToSetPushRule(before, after))
{
    QJsonObject _data;
    addParam<>(_data, QStringLiteral("actions"), actions);
    addParam<IfNotEmpty>(_data, QStringLiteral("conditions"), conditions);
    addParam<IfNotEmpty>(_data, QStringLiteral("pattern"), pattern);
    setRequestData(std::move(_data));
}

QUrl IsPushRuleEnabledJob::makeRequestUrl(QUrl baseUrl, const QString& scope,
                                          const QString& kind,
                                          const QString& ruleId)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   QStringLiteral("/_matrix/client/r0")
                                       % "/pushrules/" % scope % "/" % kind
                                       % "/" % ruleId % "/enabled");
}

IsPushRuleEnabledJob::IsPushRuleEnabledJob(const QString& scope,
                                           const QString& kind,
                                           const QString& ruleId)
    : BaseJob(HttpVerb::Get, QStringLiteral("IsPushRuleEnabledJob"),
              QStringLiteral("/_matrix/client/r0") % "/pushrules/" % scope % "/"
                  % kind % "/" % ruleId % "/enabled")
{
    addExpectedKey("enabled");
}

SetPushRuleEnabledJob::SetPushRuleEnabledJob(const QString& scope,
                                             const QString& kind,
                                             const QString& ruleId, bool enabled)
    : BaseJob(HttpVerb::Put, QStringLiteral("SetPushRuleEnabledJob"),
              QStringLiteral("/_matrix/client/r0") % "/pushrules/" % scope % "/"
                  % kind % "/" % ruleId % "/enabled")
{
    QJsonObject _data;
    addParam<>(_data, QStringLiteral("enabled"), enabled);
    setRequestData(std::move(_data));
}

QUrl GetPushRuleActionsJob::makeRequestUrl(QUrl baseUrl, const QString& scope,
                                           const QString& kind,
                                           const QString& ruleId)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   QStringLiteral("/_matrix/client/r0")
                                       % "/pushrules/" % scope % "/" % kind
                                       % "/" % ruleId % "/actions");
}

GetPushRuleActionsJob::GetPushRuleActionsJob(const QString& scope,
                                             const QString& kind,
                                             const QString& ruleId)
    : BaseJob(HttpVerb::Get, QStringLiteral("GetPushRuleActionsJob"),
              QStringLiteral("/_matrix/client/r0") % "/pushrules/" % scope % "/"
                  % kind % "/" % ruleId % "/actions")
{
    addExpectedKey("actions");
}

SetPushRuleActionsJob::SetPushRuleActionsJob(const QString& scope,
                                             const QString& kind,
                                             const QString& ruleId,
                                             const QVector<QVariant>& actions)
    : BaseJob(HttpVerb::Put, QStringLiteral("SetPushRuleActionsJob"),
              QStringLiteral("/_matrix/client/r0") % "/pushrules/" % scope % "/"
                  % kind % "/" % ruleId % "/actions")
{
    QJsonObject _data;
    addParam<>(_data, QStringLiteral("actions"), actions);
    setRequestData(std::move(_data));
}
