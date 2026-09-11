// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#pragma once

#include <Quotient/converters.h>

namespace Quotient {
//! Definition of valid values for a field.
struct QUOTIENT_API FieldType
{
    //! A regular expression for validation of a field's value. This may be relatively
    //! coarse to verify the value as the application service providing this protocol
    //! may apply additional validation or filtering.
    QString regexp;

    //! A placeholder serving as a valid example of the field value.
    QString placeholder;
};

template <>
struct JsonObjectConverter<FieldType>
{
    static void dumpTo(QJsonObject &jo, const FieldType &pod)
    {
        addParam(jo, "regexp"_L1, pod.regexp);
        addParam(jo, "placeholder"_L1, pod.placeholder);
    }
    static void fillFrom(const QJsonObject &jo, FieldType &pod)
    {
        fillFromJson(jo.value("regexp"_L1), pod.regexp);
        fillFromJson(jo.value("placeholder"_L1), pod.placeholder);
    }
};

} // namespace Quotient
