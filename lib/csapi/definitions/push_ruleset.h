/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "converters.h"

#include "csapi/definitions/push_rule.h"

#include <QtCore/QVector>

namespace QMatrixClient
{

// Data structures

struct PushRuleset
{

    QVector<PushRule> content;

    QVector<PushRule> override;

    QVector<PushRule> room;

    QVector<PushRule> sender;

    QVector<PushRule> underride;
};

template <>
struct JsonObjectConverter<PushRuleset>
{
    static void dumpTo(QJsonObject& jo, const PushRuleset& pod);
    static void fillFrom(const QJsonObject& jo, PushRuleset& pod);
};

} // namespace QMatrixClient
