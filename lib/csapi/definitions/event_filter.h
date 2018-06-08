/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once


#include "converters.h"

#include "converters.h"

namespace QMatrixClient
{
    // Data structures

    struct Filter
    {
        Omittable<int> limit;
        QStringList notSenders;
        QStringList notTypes;
        QStringList senders;
        QStringList types;
    };

    QJsonObject toJson(const Filter& pod);

    template <> struct FromJson<Filter>
    {
        Filter operator()(const QJsonValue& jv);
    };

} // namespace QMatrixClient
