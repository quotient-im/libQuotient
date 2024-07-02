// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#pragma once

#include <Quotient/converters.h>

namespace Quotient {

struct QUOTIENT_API Tag {
    //! A number in a range `[0,1]` describing a relative
    //! position of the room under the given tag.
    std::optional<float> order{};
};

template <>
struct JsonObjectConverter<Tag> {
    static void dumpTo(QJsonObject& jo, const Tag& pod)
    {
        addParam<IfNotEmpty>(jo, QStringLiteral("order"), pod.order);
    }
    static void fillFrom(const QJsonObject& jo, Tag& pod)
    {
        fillFromJson(jo.value("order"_ls), pod.order);
    }
};

} // namespace Quotient
