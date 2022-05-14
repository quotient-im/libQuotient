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

class QVariant;

namespace Quotient {
template <typename T>
struct JsonObjectConverter {
    // To be implemented in specialisations
    static void dumpTo(QJsonObject&, const T&) = delete;
    static void fillFrom(const QJsonObject&, T&) = delete;
};

namespace _impl {
    template <typename T, typename = void>
    struct JsonExporter {
        static QJsonObject dump(const T& data)
        {
            QJsonObject jo;
            JsonObjectConverter<T>::dumpTo(jo, data);
            return jo;
        }
    };

    template <typename T>
    struct JsonExporter<
        T, std::enable_if_t<std::is_invocable_v<decltype(&T::toJson), T>>> {
        static auto dump(const T& data) { return data.toJson(); }
    };
}

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
struct JsonConverter : _impl::JsonExporter<T> {
    // Unfortunately, if constexpr doesn't work with dump() and T::toJson
    // because trying to check invocability of T::toJson hits a hard
    // (non-SFINAE) compilation error if the member is not there. Hence a bit
    // more verbose SFINAE construct in _impl::JsonExporter.

    static T doLoad(const QJsonObject& jo)
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
    static T load(const QJsonValue& jv) { return doLoad(jv.toObject()); }
    static T load(const QJsonDocument& jd) { return doLoad(jd.object()); }
};

template <typename T,
          typename = std::enable_if_t<!std::is_constructible_v<QJsonValue, T>>>
inline auto toJson(const T& pod)
// -> can return anything from which QJsonValue or, in some cases, QJsonDocument
//    is constructible
{
    return JsonConverter<T>::dump(pod);
}

inline auto toJson(const QJsonObject& jo) { return jo; }
inline auto toJson(const QJsonValue& jv) { return jv; }

template <typename T>
inline void fillJson(QJsonObject& json, const T& data)
{
    JsonObjectConverter<T>::dumpTo(json, data);
}

template <typename T>
inline T fromJson(const QJsonValue& jv)
{
    return JsonConverter<T>::load(jv);
}

template<>
inline QJsonValue fromJson(const QJsonValue& jv) { return jv; }

template <typename T>
inline T fromJson(const QJsonDocument& jd)
{
    return JsonConverter<T>::load(jd);
}

// Convenience fromJson() overloads that deduce T instead of requiring
// the coder to explicitly type it. They still enforce the
// overwrite-everything semantics of fromJson(), unlike fillFromJson()

template <typename T>
inline void fromJson(const QJsonValue& jv, T& pod)
{
    pod = jv.isUndefined() ? T() : fromJson<T>(jv);
}

template <typename T>
inline void fromJson(const QJsonDocument& jd, T& pod)
{
    pod = fromJson<T>(jd);
}

template <typename T>
inline void fillFromJson(const QJsonValue& jv, T& pod)
{
    if (jv.isObject())
        JsonObjectConverter<T>::fillFrom(jv.toObject(), pod);
    else if (!jv.isUndefined())
        pod = fromJson<T>(jv);
}

// JsonConverter<> specialisations

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

//! Use fromJson<QString> and use toLatin1()/toUtf8()/... to make QByteArray
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

inline QJsonValue toJson(const QDate& val) {
    return toJson(
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
        QDateTime(val)
#else
        val.startOfDay()
#endif
    );
}
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

template <typename T>
struct JsonConverter<Omittable<T>> {
    static QJsonValue dump(const Omittable<T>& from)
    {
        return from.has_value() ? toJson(from.value()) : QJsonValue();
    }
    static Omittable<T> load(const QJsonValue& jv)
    {
        if (jv.isUndefined())
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
        for (const auto& i : ja)
            vect.push_back(fromJson<T>(i));
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
        for (auto it = jo.begin(); it != jo.end(); ++it)
            h[it.key()] = fromJson<typename HashMapT::mapped_type>(it.value());
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
