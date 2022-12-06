// SPDX-FileCopyrightText: 2017 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "omittable.h"
#include "util.h"

#include <QtCore/QDate>
#include <QtCore/QJsonArray> // Includes <QtCore/QJsonValue>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QSet>
#include <QtCore/QUrlQuery>
#include <QtCore/QVector>

#include <type_traits>
#include <vector>
#include <variant>

class QVariant;

namespace Quotient {
template <typename T>
struct JsonObjectConverter {
    // To be implemented in specialisations
    static void dumpTo(QJsonObject&, const T&) = delete;
    static void fillFrom(const QJsonObject&, T&) = delete;
};

template <typename PodT, typename JsonT>
PodT fromJson(const JsonT&);

template <typename T>
struct JsonObjectUnpacker {
    // By default, revert to fromJson() so that one could provide a single
    // fromJson<T, QJsonObject> specialisation instead of specialising
    // the entire JsonConverter; if a different type of JSON value is needed
    // (e.g., an array), specialising JsonConverter is inevitable
    static T load(const QJsonValue& jv) { return fromJson<T>(jv.toObject()); }
    static T load(const QJsonDocument& jd) { return fromJson<T>(jd.object()); }
};

//! \brief The switchboard for extra conversion algorithms behind from/toJson
//!
//! This template is mainly intended for partial conversion specialisations
//! since from/toJson are functions and cannot be partially specialised.
//! Another case for JsonConverter is to insulate types that can be constructed
//! from basic types - namely, QVariant and QUrl can be directly constructed
//! from QString and having an overload or specialisation for those leads to
//! ambiguity between these and QJsonValue. For trivial (converting
//! QJsonObject/QJsonValue) and most simple cases such as primitive types or
//! QString this class is not needed.
//!
//! Do NOT call the functions of this class directly unless you know what you're
//! doing; and do not try to specialise basic things unless you're really sure
//! that they are not supported and it's not feasible to support those by means
//! of overloading toJson() and specialising fromJson().
template <typename T>
struct JsonConverter : JsonObjectUnpacker<T> {
    // Unfortunately, if constexpr doesn't work with dump() and T::toJson
    // because trying to check invocability of T::toJson hits a hard
    // (non-SFINAE) compilation error if the member is not there. Hence a bit
    // more verbose SFINAE construct in _impl::JsonExporter.
    static auto dump(const T& data)
    {
        if constexpr (requires() { data.toJson(); })
            return data.toJson();
        else {
            QJsonObject jo;
            JsonObjectConverter<T>::dumpTo(jo, data);
            return jo;
        }
    }

    using JsonObjectUnpacker<T>::load;
    static T load(const QJsonObject& jo)
    {
        // 'else' below are required to suppress code generation for unused
        // branches - 'return' is not enough
        if constexpr (std::is_same_v<T, QJsonObject>)
            return jo;
        else if constexpr (std::is_constructible_v<T, QJsonObject>)
            return T(jo);
        else {
            T pod;
            JsonObjectConverter<T>::fillFrom(jo, pod);
            return pod;
        }
    }
};

template <typename T>
inline auto toJson(const T& pod)
// -> can return anything from which QJsonValue or, in some cases, QJsonDocument
//    is constructible
{
    if constexpr (std::is_constructible_v<QJsonValue, T>)
        return pod; // No-op if QJsonValue can be directly constructed
    else
        return JsonConverter<T>::dump(pod);
}

template <typename T>
inline void fillJson(QJsonObject& json, const T& data)
{
    JsonObjectConverter<T>::dumpTo(json, data);
}

template <typename PodT, typename JsonT>
inline PodT fromJson(const JsonT& json)
{
    // JsonT here can be whatever the respective JsonConverter specialisation
    // accepts but by default it's QJsonValue, QJsonDocument, or QJsonObject
    return JsonConverter<PodT>::load(json);
}

// Convenience fromJson() overload that deduces PodT instead of requiring
// the coder to explicitly type it. It still enforces the
// overwrite-everything semantics of fromJson(), unlike fillFromJson()

template <typename JsonT, typename PodT>
inline void fromJson(const JsonT& json, PodT& pod)
{
    pod = fromJson<PodT>(json);
}

template <typename T>
inline void fillFromJson(const QJsonValue& jv, T& pod)
{
    if constexpr (requires() { JsonObjectConverter<T>::fillFrom({}, pod); }) {
        JsonObjectConverter<T>::fillFrom(jv.toObject(), pod);
        return;
    } else if (!jv.isUndefined())
        pod = fromJson<T>(jv);
}

namespace _impl {
    void warnUnknownEnumValue(const QString& stringValue,
                              const char* enumTypeName);
    void reportEnumOutOfBounds(uint32_t v, const char* enumTypeName);
}

//! \brief Facility string-to-enum converter
//!
//! This is to simplify enum loading from JSON - just specialise
//! Quotient::fromJson() and call this function from it, passing (aside from
//! the JSON value for the enum - that must be a string, not an int) any
//! iterable container of string'y values (const char*, QLatin1String, etc.)
//! matching respective enum values, 0-based.
//! \sa enumToJsonString
template <typename EnumT, typename EnumStringValuesT>
EnumT enumFromJsonString(const QString& s, const EnumStringValuesT& enumValues,
                         EnumT defaultValue)
{
    static_assert(std::is_unsigned_v<std::underlying_type_t<EnumT>>);
    if (const auto it = std::find(cbegin(enumValues), cend(enumValues), s);
        it != cend(enumValues))
        return EnumT(it - cbegin(enumValues));

    if (!s.isEmpty())
        _impl::warnUnknownEnumValue(s, qt_getEnumName(EnumT()));
    return defaultValue;
}

//! \brief Facility enum-to-string converter
//!
//! This does the same as enumFromJsonString, the other way around.
//! \note The source enumeration must not have gaps in values, or \p enumValues
//!       has to match those gaps (i.e., if the source enumeration is defined
//!       as <tt>{ Value1 = 1, Value2 = 3, Value3 = 5 }</tt> then \p enumValues
//!       should be defined as <tt>{ "", "Value1", "", "Value2", "", "Value3"
//!       }</tt> (mind the gap at value 0, in particular).
//! \sa enumFromJsonString
template <typename EnumT, typename EnumStringValuesT>
QString enumToJsonString(EnumT v, const EnumStringValuesT& enumValues)
{
    static_assert(std::is_unsigned_v<std::underlying_type_t<EnumT>>);
    if (v < size(enumValues))
        return enumValues[v];

    _impl::reportEnumOutOfBounds(static_cast<uint32_t>(v),
                                 qt_getEnumName(EnumT()));
    Q_ASSERT(false);
    return {};
}

//! \brief Facility converter for flags
//!
//! This is very similar to enumFromJsonString, except that the target
//! enumeration is assumed to be of a 'flag' kind - i.e. its values must be
//! a power-of-two sequence starting from 1, without gaps, so exactly 1,2,4,8,16
//! and so on.
//! \note Unlike enumFromJsonString, the values start from 1 and not from 0,
//!       with 0 being used for an invalid value by default.
//! \note This function does not support flag combinations.
//! \sa QUO_DECLARE_FLAGS, QUO_DECLARE_FLAGS_NS
template <typename FlagT, typename FlagStringValuesT>
FlagT flagFromJsonString(const QString& s, const FlagStringValuesT& flagValues,
                         FlagT defaultValue = FlagT(0U))
{
    // Enums based on signed integers don't make much sense for flag types
    static_assert(std::is_unsigned_v<std::underlying_type_t<FlagT>>);
    if (const auto it = std::find(cbegin(flagValues), cend(flagValues), s);
        it != cend(flagValues))
        return FlagT(1U << (it - cbegin(flagValues)));

    if (!s.isEmpty())
        _impl::warnUnknownEnumValue(s, qt_getEnumName(FlagT()));
    return defaultValue;
}

template <typename FlagT, typename FlagStringValuesT>
QString flagToJsonString(FlagT v, const FlagStringValuesT& flagValues)
{
    static_assert(std::is_unsigned_v<std::underlying_type_t<FlagT>>);
    if (const auto offset =
            qCountTrailingZeroBits(std::underlying_type_t<FlagT>(v));
        offset < size(flagValues)) //
    {
        return flagValues[offset];
    }

    _impl::reportEnumOutOfBounds(static_cast<uint32_t>(v),
                                 qt_getEnumName(FlagT()));
    Q_ASSERT(false);
    return {};
}

// Specialisations

template<>
inline bool fromJson(const QJsonValue& jv) { return jv.toBool(); }

template <>
inline int fromJson(const QJsonValue& jv) { return jv.toInt(); }

template <>
inline double fromJson(const QJsonValue& jv) { return jv.toDouble(); }

template <>
inline float fromJson(const QJsonValue& jv) { return float(jv.toDouble()); }

template <>
inline qint64 fromJson(const QJsonValue& jv) { return qint64(jv.toDouble()); }

template <>
inline QString fromJson(const QJsonValue& jv) { return jv.toString(); }

//! Use fromJson<QString> and then toLatin1()/toUtf8()/... to make QByteArray
//!
//! QJsonValue can only convert to QString and there's ambiguity whether
//! conversion to QByteArray should use (fast but very limited) toLatin1() or
//! (all encompassing and conforming to the JSON spec but slow) toUtf8().
template <>
inline QByteArray fromJson(const QJsonValue& jv) = delete;

template <>
inline QJsonArray fromJson(const QJsonValue& jv) { return jv.toArray(); }

template <>
inline QJsonArray fromJson(const QJsonDocument& jd) { return jd.array(); }

inline QJsonValue toJson(const QDateTime& val)
{
    return val.isValid() ? val.toMSecsSinceEpoch() : QJsonValue();
}
template <>
inline QDateTime fromJson(const QJsonValue& jv)
{
    return QDateTime::fromMSecsSinceEpoch(fromJson<qint64>(jv), Qt::UTC);
}

inline QJsonValue toJson(const QDate& val) { return toJson(val.startOfDay()); }
template <>
inline QDate fromJson(const QJsonValue& jv)
{
    return fromJson<QDateTime>(jv).date();
}

// Insulate QVariant and QUrl conversions into JsonConverter so that they don't
// interfere with toJson(const QJsonValue&) over QString, since both types are
// constructible from QString (even if QUrl requires explicit construction).

template <>
struct JsonConverter<QUrl> {
    static auto load(const QJsonValue& jv)
    {
        return QUrl(jv.toString());
    }
    static auto dump(const QUrl& url)
    {
        return url.toString(QUrl::FullyEncoded);
    }
};

template <>
struct QUOTIENT_API JsonConverter<QVariant> {
    static QJsonValue dump(const QVariant& v);
    static QVariant load(const QJsonValue& jv);
};

template <typename... Ts>
inline QJsonValue toJson(const std::variant<Ts...>& v)
{
    // std::visit requires all overloads to return the same type - and
    // QJsonValue is a perfect candidate for that same type (assuming that
    // variants never occur on the top level in Matrix API)
    return std::visit(
        [](const auto& value) { return QJsonValue { toJson(value) }; }, v);
}

template <typename T>
struct JsonConverter<std::variant<QString, T>> {
    static std::variant<QString, T> load(const QJsonValue& jv)
    {
        if (jv.isString())
            return fromJson<QString>(jv);
        return fromJson<T>(jv);
    }
};

template <typename T>
struct JsonConverter<Omittable<T>> {
    static QJsonValue dump(const Omittable<T>& from)
    {
        return from.has_value() ? toJson(*from) : QJsonValue();
    }
    static Omittable<T> load(const QJsonValue& jv)
    {
        if (jv.isUndefined() || jv.isNull())
            return none;
        return fromJson<T>(jv);
    }
};

template <typename VectorT, typename T = typename VectorT::value_type>
struct JsonArrayConverter {
    static auto dump(const VectorT& vals)
    {
        QJsonArray ja;
        for (const auto& v : vals)
            ja.push_back(toJson(v));
        return ja;
    }
    static auto load(const QJsonArray& ja)
    {
        VectorT vect;
        vect.reserve(typename VectorT::size_type(ja.size()));
        // NB: Make sure to pass QJsonValue to fromJson<> so that it could
        // hit the correct overload and not fall back to the generic fromJson
        // that treats everything as an object. See also the explanation in
        // the commit introducing these lines.
        for (const QJsonValue v : ja)
            vect.push_back(fromJson<T>(v));
        return vect;
    }
    static auto load(const QJsonValue& jv) { return load(jv.toArray()); }
    static auto load(const QJsonDocument& jd) { return load(jd.array()); }
};

template <typename T>
struct JsonConverter<std::vector<T>>
    : public JsonArrayConverter<std::vector<T>> {};

#if QT_VERSION_MAJOR < 6 // QVector is an alias of QList in Qt6 but not in Qt 5
template <typename T>
struct JsonConverter<QVector<T>> : public JsonArrayConverter<QVector<T>> {};
#endif

template <typename T>
struct JsonConverter<QList<T>> : public JsonArrayConverter<QList<T>> {};

template <>
struct JsonConverter<QStringList> : public JsonArrayConverter<QStringList> {
    static auto dump(const QStringList& sl)
    {
        return QJsonArray::fromStringList(sl);
    }
};

template <>
struct JsonObjectConverter<QSet<QString>> {
    static void dumpTo(QJsonObject& json, const QSet<QString>& s)
    {
        for (const auto& e : s)
            json.insert(e, QJsonObject {});
    }
    static void fillFrom(const QJsonObject& json, QSet<QString>& s)
    {
        s.reserve(s.size() + json.size());
        for (auto it = json.begin(); it != json.end(); ++it)
            s.insert(it.key());
    }
};

template <typename HashMapT>
struct HashMapFromJson {
    static void dumpTo(QJsonObject& json, const HashMapT& hashMap)
    {
        for (auto it = hashMap.begin(); it != hashMap.end(); ++it)
            json.insert(it.key(), toJson(it.value()));
    }
    static void fillFrom(const QJsonObject& jo, HashMapT& h)
    {
        h.reserve(h.size() + jo.size());
        // NB: the QJsonValue cast below is for the same reason as in
        // JsonArrayConverter
        for (auto it = jo.begin(); it != jo.end(); ++it)
            h[it.key()] = fromJson<typename HashMapT::mapped_type>(
                QJsonValue(it.value()));
    }
};

template <typename T, typename HashT>
struct JsonObjectConverter<std::unordered_map<QString, T, HashT>>
    : public HashMapFromJson<std::unordered_map<QString, T, HashT>> {};

template <typename T>
struct JsonObjectConverter<QHash<QString, T>>
    : public HashMapFromJson<QHash<QString, T>> {};

QJsonObject QUOTIENT_API toJson(const QVariantHash& vh);
template <>
QVariantHash QUOTIENT_API fromJson(const QJsonValue& jv);

// Conditional insertion into a QJsonObject

constexpr bool IfNotEmpty = false;

namespace _impl {
    template <typename ValT>
    inline void addTo(QJsonObject& o, const QString& k, ValT&& v)
    {
        o.insert(k, toJson(v));
    }

    template <typename ValT>
    inline void addTo(QUrlQuery& q, const QString& k, ValT&& v)
    {
        q.addQueryItem(k, QStringLiteral("%1").arg(v));
    }

    // OpenAPI is entirely JSON-based, which means representing bools as
    // textual true/false, rather than 1/0.
    inline void addTo(QUrlQuery& q, const QString& k, bool v)
    {
        q.addQueryItem(k, v ? QStringLiteral("true") : QStringLiteral("false"));
    }

    inline void addTo(QUrlQuery& q, const QString& k, const QUrl& v)
    {
        q.addQueryItem(k, v.toEncoded());
    }

    inline void addTo(QUrlQuery& q, const QString& k, const QStringList& vals)
    {
        for (const auto& v : vals)
            q.addQueryItem(k, v);
    }

    // This one is for types that don't have isEmpty() and for all types
    // when Force is true
    template <typename ValT, bool Force = true, typename = bool>
    struct AddNode {
        template <typename ContT, typename ForwardedT>
        static void impl(ContT& container, const QString& key,
                         ForwardedT&& value)
        {
            addTo(container, key, std::forward<ForwardedT>(value));
        }
    };

    // This one is for types that have isEmpty() when Force is false
    template <typename ValT>
    struct AddNode<ValT, IfNotEmpty, decltype(std::declval<ValT>().isEmpty())> {
        template <typename ContT, typename ForwardedT>
        static void impl(ContT& container, const QString& key,
                         ForwardedT&& value)
        {
            if (!value.isEmpty())
                addTo(container, key, std::forward<ForwardedT>(value));
        }
    };

    // This one unfolds Omittable<> (also only when IfNotEmpty is requested)
    template <typename ValT>
    struct AddNode<Omittable<ValT>, IfNotEmpty> {
        template <typename ContT, typename OmittableT>
        static void impl(ContT& container, const QString& key,
                         const OmittableT& value)
        {
            if (value)
                addTo(container, key, *value);
        }
    };
} // namespace _impl

/*! Add a key-value pair to QJsonObject or QUrlQuery
 *
 * Adds a key-value pair(s) specified by \p key and \p value to
 * \p container, optionally (in case IfNotEmpty is passed for the first
 * template parameter) taking into account the value "emptiness".
 * With IfNotEmpty, \p value is NOT added to the container if and only if:
 * - it has a method `isEmpty()` and `value.isEmpty() == true`, or
 * - it's an `Omittable<>` and `value.omitted() == true`.
 *
 * If \p container is a QUrlQuery, an attempt to fit \p value into it is
 * made as follows:
 * - if \p value is a QJsonObject, \p key is ignored and pairs from \p value
 *   are copied to \p container, assuming that the value in each pair
 *   is a string;
 * - if \p value is a QStringList, it is "exploded" into a list of key-value
 *   pairs with key equal to \p key and value taken from each list item;
 * - if \p value is a bool, its OpenAPI (i.e. JSON) representation is added
 *   to the query (`true` or `false`, respectively).
 *
 * \tparam Force add the pair even if the value is empty. This is true
 *               by default; passing IfNotEmpty or false for this parameter
 *               enables emptiness checks as described above
 */
template <bool Force = true, typename ContT, typename ValT>
inline void addParam(ContT& container, const QString& key, ValT&& value)
{
    _impl::AddNode<std::decay_t<ValT>, Force>::impl(container, key,
                                                    std::forward<ValT>(value));
}

// This is a facility function to convert camelCase method/variable names
// used throughout Quotient to snake_case JSON keys - see usage in
// single_key_value.h and event.h (DEFINE_CONTENT_GETTER macro).
inline auto toSnakeCase(QLatin1String s)
{
    QString result { s };
    for (auto it = result.begin(); it != result.end(); ++it)
        if (it->isUpper()) {
            const auto offset = static_cast<int>(it - result.begin());
            result.insert(offset, '_'); // NB: invalidates iterators
            it = result.begin() + offset + 1;
            *it = it->toLower();
        }
    return result;
}
} // namespace Quotient
