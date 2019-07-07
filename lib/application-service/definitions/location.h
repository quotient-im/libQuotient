/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "converters.h"

#include <QtCore/QJsonObject>

namespace Quotient
{

// Data structures

struct ThirdPartyLocation
{
    /// An alias for a matrix room.
    QString alias;
    /// The protocol ID that the third party location is a part of.
    QString protocol;
    /// Information used to identify this third party location.
    QJsonObject fields;
};

template <>
struct JsonObjectConverter<ThirdPartyLocation>
{
    static void dumpTo(QJsonObject& jo, const ThirdPartyLocation& pod);
    static void fillFrom(const QJsonObject& jo, ThirdPartyLocation& pod);
};

} // namespace Quotient
