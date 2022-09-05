#pragma once

#include "converters.h"

namespace Quotient {

namespace EventContent {
    template <typename T, const QLatin1String& KeyStr>
    struct SingleKeyValue {
        // NOLINTBEGIN(google-explicit-constructor): that check should learn
        //                                           about explicit(false)
        QUO_IMPLICIT SingleKeyValue(const T& v = {})
            : value { v }
        {}
        QUO_IMPLICIT SingleKeyValue(T&& v)
            : value { std::move(v) }
        {}
        // NOLINTEND(google-explicit-constructor)
        T value;
    };
} // namespace EventContent

template <typename ValueT, const QLatin1String& KeyStr>
struct JsonConverter<EventContent::SingleKeyValue<ValueT, KeyStr>> {
    using content_type = EventContent::SingleKeyValue<ValueT, KeyStr>;
    static content_type load(const QJsonValue& jv)
    {
        return { fromJson<ValueT>(jv.toObject().value(JsonKey)) };
    }
    static QJsonObject dump(const content_type& c)
    {
        return { { JsonKey, toJson(c.value) } };
    }
    static inline const auto JsonKey = toSnakeCase(KeyStr);
};
} // namespace Quotient
