/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once



#include "converters.h"

namespace QMatrixClient
{
    // Data structures

    struct PushCondition
    {
        QString kind;
        QString key;
        QString pattern;
        QString is;
    };

    QJsonObject toJson(const PushCondition& pod);

    template <> struct FromJson<PushCondition>
    {
        PushCondition operator()(const QJsonValue& jv);
    };

} // namespace QMatrixClient
