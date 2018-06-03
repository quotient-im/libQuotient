/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once


#include "lib/csapi/definitions/push_condition.h"
#include "converters.h"
#include <QtCore/QVector>
#include <QtCore/QVariant>
#include <QtCore/QJsonObject>

#include "converters.h"

namespace QMatrixClient
{
    // Data structures

    struct PushRule
    {
        QVector<QVariant> actions;
        bool isDefault;
        bool enabled;
        QString ruleId;
        QVector<PushCondition> conditions;
        QString pattern;
    };

    QJsonObject toJson(const PushRule& pod);

    template <> struct FromJson<PushRule>
    {
        PushRule operator()(const QJsonValue& jv);
    };

} // namespace QMatrixClient
