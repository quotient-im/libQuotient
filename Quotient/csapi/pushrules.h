// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#pragma once

#include <Quotient/csapi/definitions/push_condition.h>
#include <Quotient/csapi/definitions/push_rule.h>
#include <Quotient/csapi/definitions/push_ruleset.h>

#include <Quotient/jobs/basejob.h>

namespace Quotient {

//! \brief Retrieve all push rulesets.
//!
//! Retrieve all push rulesets for this user. Clients can "drill-down" on
//! the rulesets by suffixing a `scope` to this path e.g.
//! `/pushrules/global/`. This will return a subset of this data under the
//! specified key e.g. the `global` key.
class QUOTIENT_API GetPushRulesJob : public BaseJob {
public:
    explicit GetPushRulesJob();

    //! \brief Construct a URL without creating a full-fledged job object
    //!
    //! This function can be used when a URL for GetPushRulesJob
    //! is necessary but the job itself isn't.
    static QUrl makeRequestUrl(QUrl baseUrl);

    // Result properties

    //! The global ruleset.
    PushRuleset global() const { return loadFromJson<PushRuleset>("global"_ls); }
};

//! \brief Retrieve a push rule.
//!
//! Retrieve a single specified push rule.
class QUOTIENT_API GetPushRuleJob : public BaseJob {
public:
    //! \param scope
    //!   `global` to specify global rules.
    //!
    //! \param kind
    //!   The kind of rule
    //!
    //! \param ruleId
    //!   The identifier for the rule.
    explicit GetPushRuleJob(const QString& scope, const QString& kind, const QString& ruleId);

    //! \brief Construct a URL without creating a full-fledged job object
    //!
    //! This function can be used when a URL for GetPushRuleJob
    //! is necessary but the job itself isn't.
    static QUrl makeRequestUrl(QUrl baseUrl, const QString& scope, const QString& kind,
                               const QString& ruleId);

    // Result properties

    //! The specific push rule. This will also include keys specific to the
    //! rule itself such as the rule's `actions` and `conditions` if set.
    PushRule pushRule() const { return fromJson<PushRule>(jsonData()); }
};

//! \brief Delete a push rule.
//!
//! This endpoint removes the push rule defined in the path.
class QUOTIENT_API DeletePushRuleJob : public BaseJob {
public:
    //! \param scope
    //!   `global` to specify global rules.
    //!
    //! \param kind
    //!   The kind of rule
    //!
    //! \param ruleId
    //!   The identifier for the rule.
    explicit DeletePushRuleJob(const QString& scope, const QString& kind, const QString& ruleId);

    //! \brief Construct a URL without creating a full-fledged job object
    //!
    //! This function can be used when a URL for DeletePushRuleJob
    //! is necessary but the job itself isn't.
    static QUrl makeRequestUrl(QUrl baseUrl, const QString& scope, const QString& kind,
                               const QString& ruleId);
};

//! \brief Add or change a push rule.
//!
//! This endpoint allows the creation and modification of user defined push
//! rules.
//!
//! If a rule with the same `rule_id` already exists among rules of the same
//! kind, it is updated with the new parameters, otherwise a new rule is
//! created.
//!
//! If both `after` and `before` are provided, the new or updated rule must
//! be the next most important rule with respect to the rule identified by
//! `before`.
//!
//! If neither `after` nor `before` are provided and the rule is created, it
//! should be added as the most important user defined rule among rules of
//! the same kind.
//!
//! When creating push rules, they MUST be enabled by default.
class QUOTIENT_API SetPushRuleJob : public BaseJob {
public:
    //! \param scope
    //!   `global` to specify global rules.
    //!
    //! \param kind
    //!   The kind of rule
    //!
    //! \param ruleId
    //!   The identifier for the rule. If the string starts with a dot ("."),
    //!   the request MUST be rejected as this is reserved for server-default
    //!   rules. Slashes ("/") and backslashes ("\\") are also not allowed.
    //!
    //! \param actions
    //!   The action(s) to perform when the conditions for this rule are met.
    //!
    //! \param before
    //!   Use 'before' with a `rule_id` as its value to make the new rule the
    //!   next-most important rule with respect to the given user defined rule.
    //!   It is not possible to add a rule relative to a predefined server rule.
    //!
    //! \param after
    //!   This makes the new rule the next-less important rule relative to the
    //!   given user defined rule. It is not possible to add a rule relative
    //!   to a predefined server rule.
    //!
    //! \param conditions
    //!   The conditions that must hold true for an event in order for a
    //!   rule to be applied to an event. A rule with no conditions
    //!   always matches. Only applicable to `underride` and `override` rules.
    //!
    //! \param pattern
    //!   Only applicable to `content` rules. The glob-style pattern to match against.
    explicit SetPushRuleJob(const QString& scope, const QString& kind, const QString& ruleId,
                            const QVector<QVariant>& actions, const QString& before = {},
                            const QString& after = {}, const QVector<PushCondition>& conditions = {},
                            const QString& pattern = {});
};

//! \brief Get whether a push rule is enabled
//!
//! This endpoint gets whether the specified push rule is enabled.
class QUOTIENT_API IsPushRuleEnabledJob : public BaseJob {
public:
    //! \param scope
    //!   Either `global` or `device/<profile_tag>` to specify global
    //!   rules or device rules for the given `profile_tag`.
    //!
    //! \param kind
    //!   The kind of rule
    //!
    //! \param ruleId
    //!   The identifier for the rule.
    explicit IsPushRuleEnabledJob(const QString& scope, const QString& kind, const QString& ruleId);

    //! \brief Construct a URL without creating a full-fledged job object
    //!
    //! This function can be used when a URL for IsPushRuleEnabledJob
    //! is necessary but the job itself isn't.
    static QUrl makeRequestUrl(QUrl baseUrl, const QString& scope, const QString& kind,
                               const QString& ruleId);

    // Result properties

    //! Whether the push rule is enabled or not.
    bool enabled() const { return loadFromJson<bool>("enabled"_ls); }
};

//! \brief Enable or disable a push rule.
//!
//! This endpoint allows clients to enable or disable the specified push rule.
class QUOTIENT_API SetPushRuleEnabledJob : public BaseJob {
public:
    //! \param scope
    //!   `global` to specify global rules.
    //!
    //! \param kind
    //!   The kind of rule
    //!
    //! \param ruleId
    //!   The identifier for the rule.
    //!
    //! \param enabled
    //!   Whether the push rule is enabled or not.
    explicit SetPushRuleEnabledJob(const QString& scope, const QString& kind, const QString& ruleId,
                                   bool enabled);
};

//! \brief The actions for a push rule
//!
//! This endpoint get the actions for the specified push rule.
class QUOTIENT_API GetPushRuleActionsJob : public BaseJob {
public:
    //! \param scope
    //!   Either `global` or `device/<profile_tag>` to specify global
    //!   rules or device rules for the given `profile_tag`.
    //!
    //! \param kind
    //!   The kind of rule
    //!
    //! \param ruleId
    //!   The identifier for the rule.
    explicit GetPushRuleActionsJob(const QString& scope, const QString& kind, const QString& ruleId);

    //! \brief Construct a URL without creating a full-fledged job object
    //!
    //! This function can be used when a URL for GetPushRuleActionsJob
    //! is necessary but the job itself isn't.
    static QUrl makeRequestUrl(QUrl baseUrl, const QString& scope, const QString& kind,
                               const QString& ruleId);

    // Result properties

    //! The action(s) to perform for this rule.
    QVector<QVariant> actions() const { return loadFromJson<QVector<QVariant>>("actions"_ls); }
};

//! \brief Set the actions for a push rule.
//!
//! This endpoint allows clients to change the actions of a push rule.
//! This can be used to change the actions of builtin rules.
class QUOTIENT_API SetPushRuleActionsJob : public BaseJob {
public:
    //! \param scope
    //!   `global` to specify global rules.
    //!
    //! \param kind
    //!   The kind of rule
    //!
    //! \param ruleId
    //!   The identifier for the rule.
    //!
    //! \param actions
    //!   The action(s) to perform for this rule.
    explicit SetPushRuleActionsJob(const QString& scope, const QString& kind, const QString& ruleId,
                                   const QVector<QVariant>& actions);
};

} // namespace Quotient
