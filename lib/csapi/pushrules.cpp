/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "pushrules.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

class GetPushRulesJob::Private
{
    public:
        PushRuleset global;
};

QUrl GetPushRulesJob::makeRequestUrl(QUrl baseUrl)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
            basePath % "/pushrules");
}

static const auto GetPushRulesJobName = QStringLiteral("GetPushRulesJob");

GetPushRulesJob::GetPushRulesJob()
    : BaseJob(HttpVerb::Get, GetPushRulesJobName,
        basePath % "/pushrules")
    , d(new Private)
{
}

GetPushRulesJob::~GetPushRulesJob() = default;

const PushRuleset& GetPushRulesJob::global() const
{
    return d->global;
}

BaseJob::Status GetPushRulesJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    if (!json.contains("global"_ls))
        return { JsonParseError,
            "The key 'global' not found in the response" };
    d->global = fromJson<PushRuleset>(json.value("global"_ls));
    return Success;
}

class GetPushRuleJob::Private
{
    public:
        PushRule data;
};

QUrl GetPushRuleJob::makeRequestUrl(QUrl baseUrl, const QString& scope, const QString& kind, const QString& ruleId)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
            basePath % "/pushrules/" % scope % "/" % kind % "/" % ruleId);
}

static const auto GetPushRuleJobName = QStringLiteral("GetPushRuleJob");

GetPushRuleJob::GetPushRuleJob(const QString& scope, const QString& kind, const QString& ruleId)
    : BaseJob(HttpVerb::Get, GetPushRuleJobName,
        basePath % "/pushrules/" % scope % "/" % kind % "/" % ruleId)
    , d(new Private)
{
}

GetPushRuleJob::~GetPushRuleJob() = default;

const PushRule& GetPushRuleJob::data() const
{
    return d->data;
}

BaseJob::Status GetPushRuleJob::parseJson(const QJsonDocument& data)
{
    d->data = fromJson<PushRule>(data);
    return Success;
}

QUrl DeletePushRuleJob::makeRequestUrl(QUrl baseUrl, const QString& scope, const QString& kind, const QString& ruleId)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
            basePath % "/pushrules/" % scope % "/" % kind % "/" % ruleId);
}

static const auto DeletePushRuleJobName = QStringLiteral("DeletePushRuleJob");

DeletePushRuleJob::DeletePushRuleJob(const QString& scope, const QString& kind, const QString& ruleId)
    : BaseJob(HttpVerb::Delete, DeletePushRuleJobName,
        basePath % "/pushrules/" % scope % "/" % kind % "/" % ruleId)
{
}

BaseJob::Query queryToSetPushRule(const QString& before, const QString& after)
{
    BaseJob::Query _q;
    addParam<IfNotEmpty>(_q, QStringLiteral("before"), before);
    addParam<IfNotEmpty>(_q, QStringLiteral("after"), after);
    return _q;
}

static const auto SetPushRuleJobName = QStringLiteral("SetPushRuleJob");

SetPushRuleJob::SetPushRuleJob(const QString& scope, const QString& kind, const QString& ruleId, const QStringList& actions, const QString& before, const QString& after, const QVector<PushCondition>& conditions, const QString& pattern)
    : BaseJob(HttpVerb::Put, SetPushRuleJobName,
        basePath % "/pushrules/" % scope % "/" % kind % "/" % ruleId,
        queryToSetPushRule(before, after))
{
    QJsonObject _data;
    addParam<>(_data, QStringLiteral("actions"), actions);
    addParam<IfNotEmpty>(_data, QStringLiteral("conditions"), conditions);
    addParam<IfNotEmpty>(_data, QStringLiteral("pattern"), pattern);
    setRequestData(_data);
}

class IsPushRuleEnabledJob::Private
{
    public:
        bool enabled;
};

QUrl IsPushRuleEnabledJob::makeRequestUrl(QUrl baseUrl, const QString& scope, const QString& kind, const QString& ruleId)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
            basePath % "/pushrules/" % scope % "/" % kind % "/" % ruleId % "/enabled");
}

static const auto IsPushRuleEnabledJobName = QStringLiteral("IsPushRuleEnabledJob");

IsPushRuleEnabledJob::IsPushRuleEnabledJob(const QString& scope, const QString& kind, const QString& ruleId)
    : BaseJob(HttpVerb::Get, IsPushRuleEnabledJobName,
        basePath % "/pushrules/" % scope % "/" % kind % "/" % ruleId % "/enabled")
    , d(new Private)
{
}

IsPushRuleEnabledJob::~IsPushRuleEnabledJob() = default;

bool IsPushRuleEnabledJob::enabled() const
{
    return d->enabled;
}

BaseJob::Status IsPushRuleEnabledJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    if (!json.contains("enabled"_ls))
        return { JsonParseError,
            "The key 'enabled' not found in the response" };
    d->enabled = fromJson<bool>(json.value("enabled"_ls));
    return Success;
}

static const auto SetPushRuleEnabledJobName = QStringLiteral("SetPushRuleEnabledJob");

SetPushRuleEnabledJob::SetPushRuleEnabledJob(const QString& scope, const QString& kind, const QString& ruleId, bool enabled)
    : BaseJob(HttpVerb::Put, SetPushRuleEnabledJobName,
        basePath % "/pushrules/" % scope % "/" % kind % "/" % ruleId % "/enabled")
{
    QJsonObject _data;
    addParam<>(_data, QStringLiteral("enabled"), enabled);
    setRequestData(_data);
}

class GetPushRuleActionsJob::Private
{
    public:
        QStringList actions;
};

QUrl GetPushRuleActionsJob::makeRequestUrl(QUrl baseUrl, const QString& scope, const QString& kind, const QString& ruleId)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
            basePath % "/pushrules/" % scope % "/" % kind % "/" % ruleId % "/actions");
}

static const auto GetPushRuleActionsJobName = QStringLiteral("GetPushRuleActionsJob");

GetPushRuleActionsJob::GetPushRuleActionsJob(const QString& scope, const QString& kind, const QString& ruleId)
    : BaseJob(HttpVerb::Get, GetPushRuleActionsJobName,
        basePath % "/pushrules/" % scope % "/" % kind % "/" % ruleId % "/actions")
    , d(new Private)
{
}

GetPushRuleActionsJob::~GetPushRuleActionsJob() = default;

const QStringList& GetPushRuleActionsJob::actions() const
{
    return d->actions;
}

BaseJob::Status GetPushRuleActionsJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    if (!json.contains("actions"_ls))
        return { JsonParseError,
            "The key 'actions' not found in the response" };
    d->actions = fromJson<QStringList>(json.value("actions"_ls));
    return Success;
}

static const auto SetPushRuleActionsJobName = QStringLiteral("SetPushRuleActionsJob");

SetPushRuleActionsJob::SetPushRuleActionsJob(const QString& scope, const QString& kind, const QString& ruleId, const QStringList& actions)
    : BaseJob(HttpVerb::Put, SetPushRuleActionsJobName,
        basePath % "/pushrules/" % scope % "/" % kind % "/" % ruleId % "/actions")
{
    QJsonObject _data;
    addParam<>(_data, QStringLiteral("actions"), actions);
    setRequestData(_data);
}

