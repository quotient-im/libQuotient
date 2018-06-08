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

GetPushRulesJob::GetPushRulesJob()
    : BaseJob(HttpVerb::Get, "GetPushRulesJob",
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
    if (!json.contains("global"))
        return { JsonParseError,
            "The key 'global' not found in the response" };
    d->global = fromJson<PushRuleset>(json.value("global"));
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

GetPushRuleJob::GetPushRuleJob(const QString& scope, const QString& kind, const QString& ruleId)
    : BaseJob(HttpVerb::Get, "GetPushRuleJob",
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
    auto json = data.object();
    if (!json.contains("data"))
        return { JsonParseError,
            "The key 'data' not found in the response" };
    d->data = fromJson<PushRule>(json.value("data"));
    return Success;
}

QUrl DeletePushRuleJob::makeRequestUrl(QUrl baseUrl, const QString& scope, const QString& kind, const QString& ruleId)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
            basePath % "/pushrules/" % scope % "/" % kind % "/" % ruleId);
}

DeletePushRuleJob::DeletePushRuleJob(const QString& scope, const QString& kind, const QString& ruleId)
    : BaseJob(HttpVerb::Delete, "DeletePushRuleJob",
        basePath % "/pushrules/" % scope % "/" % kind % "/" % ruleId)
{
}

BaseJob::Query queryToSetPushRule(const QString& before, const QString& after)
{
    BaseJob::Query _q;
    addToQuery<IfNotEmpty>(_q, "before", before);
    addToQuery<IfNotEmpty>(_q, "after", after);
    return _q;
}

SetPushRuleJob::SetPushRuleJob(const QString& scope, const QString& kind, const QString& ruleId, const QStringList& actions, const QString& before, const QString& after, const QVector<PushCondition>& conditions, const QString& pattern)
    : BaseJob(HttpVerb::Put, "SetPushRuleJob",
        basePath % "/pushrules/" % scope % "/" % kind % "/" % ruleId,
        queryToSetPushRule(before, after))
{
    QJsonObject _data;
    addToJson<>(_data, "actions", actions);
    addToJson<IfNotEmpty>(_data, "conditions", conditions);
    addToJson<IfNotEmpty>(_data, "pattern", pattern);
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

IsPushRuleEnabledJob::IsPushRuleEnabledJob(const QString& scope, const QString& kind, const QString& ruleId)
    : BaseJob(HttpVerb::Get, "IsPushRuleEnabledJob",
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
    if (!json.contains("enabled"))
        return { JsonParseError,
            "The key 'enabled' not found in the response" };
    d->enabled = fromJson<bool>(json.value("enabled"));
    return Success;
}

SetPushRuleEnabledJob::SetPushRuleEnabledJob(const QString& scope, const QString& kind, const QString& ruleId, bool enabled)
    : BaseJob(HttpVerb::Put, "SetPushRuleEnabledJob",
        basePath % "/pushrules/" % scope % "/" % kind % "/" % ruleId % "/enabled")
{
    QJsonObject _data;
    addToJson<>(_data, "enabled", enabled);
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

GetPushRuleActionsJob::GetPushRuleActionsJob(const QString& scope, const QString& kind, const QString& ruleId)
    : BaseJob(HttpVerb::Get, "GetPushRuleActionsJob",
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
    if (!json.contains("actions"))
        return { JsonParseError,
            "The key 'actions' not found in the response" };
    d->actions = fromJson<QStringList>(json.value("actions"));
    return Success;
}

SetPushRuleActionsJob::SetPushRuleActionsJob(const QString& scope, const QString& kind, const QString& ruleId, const QStringList& actions)
    : BaseJob(HttpVerb::Put, "SetPushRuleActionsJob",
        basePath % "/pushrules/" % scope % "/" % kind % "/" % ruleId % "/actions")
{
    QJsonObject _data;
    addToJson<>(_data, "actions", actions);
    setRequestData(_data);
}

