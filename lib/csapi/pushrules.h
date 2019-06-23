/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "converters.h"

#include "csapi/definitions/push_condition.h"
#include "csapi/definitions/push_rule.h"
#include "csapi/definitions/push_ruleset.h"

#include "jobs/basejob.h"

#include <QtCore/QVector>

namespace QMatrixClient
{

// Operations

/// Retrieve all push rulesets.
/*!
 * Retrieve all push rulesets for this user. Clients can "drill-down" on
 * the rulesets by suffixing a ``scope`` to this path e.g.
 * ``/pushrules/global/``. This will return a subset of this data under the
 * specified key e.g. the ``global`` key.
 */
class GetPushRulesJob : public BaseJob
{
public:
    explicit GetPushRulesJob();

    /*! Construct a URL without creating a full-fledged job object
     *
     * This function can be used when a URL for
     * GetPushRulesJob is necessary but the job
     * itself isn't.
     */
    static QUrl makeRequestUrl(QUrl baseUrl);

    ~GetPushRulesJob() override;

    // Result properties

    /// The global ruleset.
    const PushRuleset& global() const;

protected:
    Status parseJson(const QJsonDocument& data) override;

private:
    class Private;
    QScopedPointer<Private> d;
};

/// Retrieve a push rule.
/*!
 * Retrieve a single specified push rule.
 */
class GetPushRuleJob : public BaseJob
{
public:
    /*! Retrieve a push rule.
     * \param scope
     *   ``global`` to specify global rules.
     * \param kind
     *   The kind of rule
     * \param ruleId
     *   The identifier for the rule.
     */
    explicit GetPushRuleJob(const QString& scope, const QString& kind,
                            const QString& ruleId);

    /*! Construct a URL without creating a full-fledged job object
     *
     * This function can be used when a URL for
     * GetPushRuleJob is necessary but the job
     * itself isn't.
     */
    static QUrl makeRequestUrl(QUrl baseUrl, const QString& scope,
                               const QString& kind, const QString& ruleId);

    ~GetPushRuleJob() override;

    // Result properties

    /// The push rule.
    const PushRule& data() const;

protected:
    Status parseJson(const QJsonDocument& data) override;

private:
    class Private;
    QScopedPointer<Private> d;
};

/// Delete a push rule.
/*!
 * This endpoint removes the push rule defined in the path.
 */
class DeletePushRuleJob : public BaseJob
{
public:
    /*! Delete a push rule.
     * \param scope
     *   ``global`` to specify global rules.
     * \param kind
     *   The kind of rule
     * \param ruleId
     *   The identifier for the rule.
     */
    explicit DeletePushRuleJob(const QString& scope, const QString& kind,
                               const QString& ruleId);

    /*! Construct a URL without creating a full-fledged job object
     *
     * This function can be used when a URL for
     * DeletePushRuleJob is necessary but the job
     * itself isn't.
     */
    static QUrl makeRequestUrl(QUrl baseUrl, const QString& scope,
                               const QString& kind, const QString& ruleId);
};

/// Add or change a push rule.
/*!
 * This endpoint allows the creation, modification and deletion of pushers
 * for this user ID. The behaviour of this endpoint varies depending on the
 * values in the JSON body.
 *
 * When creating push rules, they MUST be enabled by default.
 */
class SetPushRuleJob : public BaseJob
{
public:
    /*! Add or change a push rule.
     * \param scope
     *   ``global`` to specify global rules.
     * \param kind
     *   The kind of rule
     * \param ruleId
     *   The identifier for the rule.
     * \param actions
     *   The action(s) to perform when the conditions for this rule are met.
     * \param before
     *   Use 'before' with a ``rule_id`` as its value to make the new rule the
     *   next-most important rule with respect to the given user defined rule.
     *   It is not possible to add a rule relative to a predefined server rule.
     * \param after
     *   This makes the new rule the next-less important rule relative to the
     *   given user defined rule. It is not possible to add a rule relative
     *   to a predefined server rule.
     * \param conditions
     *   The conditions that must hold true for an event in order for a
     *   rule to be applied to an event. A rule with no conditions
     *   always matches. Only applicable to ``underride`` and ``override``
     * rules. \param pattern Only applicable to ``content`` rules. The
     * glob-style pattern to match against.
     */
    explicit SetPushRuleJob(const QString& scope, const QString& kind,
                            const QString& ruleId, const QStringList& actions,
                            const QString& before = {},
                            const QString& after = {},
                            const QVector<PushCondition>& conditions = {},
                            const QString& pattern = {});
};

/// Get whether a push rule is enabled
/*!
 * This endpoint gets whether the specified push rule is enabled.
 */
class IsPushRuleEnabledJob : public BaseJob
{
public:
    /*! Get whether a push rule is enabled
     * \param scope
     *   Either ``global`` or ``device/<profile_tag>`` to specify global
     *   rules or device rules for the given ``profile_tag``.
     * \param kind
     *   The kind of rule
     * \param ruleId
     *   The identifier for the rule.
     */
    explicit IsPushRuleEnabledJob(const QString& scope, const QString& kind,
                                  const QString& ruleId);

    /*! Construct a URL without creating a full-fledged job object
     *
     * This function can be used when a URL for
     * IsPushRuleEnabledJob is necessary but the job
     * itself isn't.
     */
    static QUrl makeRequestUrl(QUrl baseUrl, const QString& scope,
                               const QString& kind, const QString& ruleId);

    ~IsPushRuleEnabledJob() override;

    // Result properties

    /// Whether the push rule is enabled or not.
    bool enabled() const;

protected:
    Status parseJson(const QJsonDocument& data) override;

private:
    class Private;
    QScopedPointer<Private> d;
};

/// Enable or disable a push rule.
/*!
 * This endpoint allows clients to enable or disable the specified push rule.
 */
class SetPushRuleEnabledJob : public BaseJob
{
public:
    /*! Enable or disable a push rule.
     * \param scope
     *   ``global`` to specify global rules.
     * \param kind
     *   The kind of rule
     * \param ruleId
     *   The identifier for the rule.
     * \param enabled
     *   Whether the push rule is enabled or not.
     */
    explicit SetPushRuleEnabledJob(const QString& scope, const QString& kind,
                                   const QString& ruleId, bool enabled);
};

/// The actions for a push rule
/*!
 * This endpoint get the actions for the specified push rule.
 */
class GetPushRuleActionsJob : public BaseJob
{
public:
    /*! The actions for a push rule
     * \param scope
     *   Either ``global`` or ``device/<profile_tag>`` to specify global
     *   rules or device rules for the given ``profile_tag``.
     * \param kind
     *   The kind of rule
     * \param ruleId
     *   The identifier for the rule.
     */
    explicit GetPushRuleActionsJob(const QString& scope, const QString& kind,
                                   const QString& ruleId);

    /*! Construct a URL without creating a full-fledged job object
     *
     * This function can be used when a URL for
     * GetPushRuleActionsJob is necessary but the job
     * itself isn't.
     */
    static QUrl makeRequestUrl(QUrl baseUrl, const QString& scope,
                               const QString& kind, const QString& ruleId);

    ~GetPushRuleActionsJob() override;

    // Result properties

    /// The action(s) to perform for this rule.
    const QStringList& actions() const;

protected:
    Status parseJson(const QJsonDocument& data) override;

private:
    class Private;
    QScopedPointer<Private> d;
};

/// Set the actions for a push rule.
/*!
 * This endpoint allows clients to change the actions of a push rule.
 * This can be used to change the actions of builtin rules.
 */
class SetPushRuleActionsJob : public BaseJob
{
public:
    /*! Set the actions for a push rule.
     * \param scope
     *   ``global`` to specify global rules.
     * \param kind
     *   The kind of rule
     * \param ruleId
     *   The identifier for the rule.
     * \param actions
     *   The action(s) to perform for this rule.
     */
    explicit SetPushRuleActionsJob(const QString& scope, const QString& kind,
                                   const QString& ruleId,
                                   const QStringList& actions);
};

} // namespace QMatrixClient
