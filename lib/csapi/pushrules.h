/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"

#include "csapi/definitions/push_ruleset.h"
#include "converters.h"
#include "csapi/definitions/push_rule.h"
#include <QtCore/QVector>
#include "csapi/definitions/push_condition.h"

namespace QMatrixClient
{
    // Operations

    class GetPushRulesJob : public BaseJob
    {
        public:
            explicit GetPushRulesJob();

            /** Construct a URL out of baseUrl and usual parameters passed to
             * GetPushRulesJob. This function can be used when
             * a URL for GetPushRulesJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl);

            ~GetPushRulesJob() override;

            // Result properties

            const PushRuleset& global() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };

    class GetPushRuleJob : public BaseJob
    {
        public:
            explicit GetPushRuleJob(const QString& scope, const QString& kind, const QString& ruleId);

            /** Construct a URL out of baseUrl and usual parameters passed to
             * GetPushRuleJob. This function can be used when
             * a URL for GetPushRuleJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl, const QString& scope, const QString& kind, const QString& ruleId);

            ~GetPushRuleJob() override;

            // Result properties

            const PushRule& data() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };

    class DeletePushRuleJob : public BaseJob
    {
        public:
            explicit DeletePushRuleJob(const QString& scope, const QString& kind, const QString& ruleId);

            /** Construct a URL out of baseUrl and usual parameters passed to
             * DeletePushRuleJob. This function can be used when
             * a URL for DeletePushRuleJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl, const QString& scope, const QString& kind, const QString& ruleId);

    };

    class SetPushRuleJob : public BaseJob
    {
        public:
            explicit SetPushRuleJob(const QString& scope, const QString& kind, const QString& ruleId, const QStringList& actions, const QString& before = {}, const QString& after = {}, const QVector<PushCondition>& conditions = {}, const QString& pattern = {});
    };

    class IsPushRuleEnabledJob : public BaseJob
    {
        public:
            explicit IsPushRuleEnabledJob(const QString& scope, const QString& kind, const QString& ruleId);

            /** Construct a URL out of baseUrl and usual parameters passed to
             * IsPushRuleEnabledJob. This function can be used when
             * a URL for IsPushRuleEnabledJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl, const QString& scope, const QString& kind, const QString& ruleId);

            ~IsPushRuleEnabledJob() override;

            // Result properties

            bool enabled() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };

    class SetPushRuleEnabledJob : public BaseJob
    {
        public:
            explicit SetPushRuleEnabledJob(const QString& scope, const QString& kind, const QString& ruleId, bool enabled);
    };

    class GetPushRuleActionsJob : public BaseJob
    {
        public:
            explicit GetPushRuleActionsJob(const QString& scope, const QString& kind, const QString& ruleId);

            /** Construct a URL out of baseUrl and usual parameters passed to
             * GetPushRuleActionsJob. This function can be used when
             * a URL for GetPushRuleActionsJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl, const QString& scope, const QString& kind, const QString& ruleId);

            ~GetPushRuleActionsJob() override;

            // Result properties

            const QStringList& actions() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };

    class SetPushRuleActionsJob : public BaseJob
    {
        public:
            explicit SetPushRuleActionsJob(const QString& scope, const QString& kind, const QString& ruleId, const QStringList& actions);
    };
} // namespace QMatrixClient
