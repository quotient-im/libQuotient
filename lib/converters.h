// SPDX-FileCopyrightText: 2017 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "util.h"

#include <QtCore/QDate>
#include <QtCore/QJsonArray> // Includes <QtCore/QJsonValue>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QSet>
#include <QtCore/QUrlQuery>
#include <QtCore/QVector>

#include <vector>

class QVariant;

namespace Quotient {
template <typename T>
struct JsonObjectConverter {
    static void dumpTo(QJsonObject& jo, const T& pod) { jo = pod.toJson(); }
    static void fillFrom(const QJsonObject& jo, T& pod) { pod = T(jo); }
};

template <typename T>
struct JsonConverter {
    static QJsonObject dump(const T& pod)
    {
        QJsonObject jo;
        JsonObjectConverter<T>::dumpTo(jo, pod);
        return jo;
    }
    static T doLoad(const QJsonObject& jo)
    {
        T pod;
        JsonObjectConverter<T>::fillFrom(jo, pod);
        return pod;
    }
    static T load(const QJsonValue& jv) { return doLoad(jv.toObject()); }
    static T load(const QJsonDocument& jd) { return doLoad(jd.object()); }
};

template <typename T>
inline auto toJson(const T& pod)
{
    return JsonConverter<T>::dump(pod);
}

inline auto toJson(const QJsonObject& jo) { return jo; }

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

template <typename T>
struct TrivialJsonDumper {
    // Works for: QJsonValue (and all things it can consume),
    // QJsonObject, QJsonArray
    static auto dump(const T& val) { return val; }
};

template <>
struct JsonConverter<bool> : public TrivialJsonDumper<bool> {
    static auto load(const QJsonValue& jv) { return jv.toBool(); }
};

template <>
struct JsonConverter<int> : public TrivialJsonDumper<int> {
    static auto load(const QJsonValue& jv) { return jv.toInt(); }
};

template <>
struct JsonConverter<double> : public TrivialJsonDumper<double> {
    static auto load(const QJsonValue& jv) { return jv.toDouble(); }
};

template <>
struct JsonConverter<float> : public TrivialJsonDumper<float> {
    static auto load(const QJsonValue& jv) { return float(jv.toDouble()); }
};

template <>
struct JsonConverter<qint64> : public TrivialJsonDumper<qint64> {
    static auto load(const QJsonValue& jv) { return qint64(jv.toDouble()); }
};

template <>
struct JsonConverter<QString> : public TrivialJsonDumper<QString> {
    static auto load(const QJsonValue& jv) { return jv.toString(); }
};

template <>
struct JsonConverter<QDateTime> {
    static auto dump(const QDateTime& val) = delete; // not provided yet
    static auto load(const QJsonValue& jv)
    {
        return QDateTime::fromMSecsSinceEpoch(fromJson<qint64>(jv), Qt::UTC);
    }
};

template <>
struct JsonConverter<QDate> {
    static auto dump(const QDate& val) = delete; // not provided yet
    static auto load(const QJsonValue& jv)
    {
        return fromJson<QDateTime>(jv).date();
    }
};

template <>
struct JsonConverter<QUrl> {
    static auto load(const QJsonValue& jv)
    {
        // QT_NO_URL_CAST_FROM_STRING makes this a bit more verbose
        QUrl url;
        url.setUrl(jv.toString());
        return url;
    }
    static auto dump(const QUrl& url)
    {
        return url.toString(QUrl::FullyEncoded);
    }
};

template <>
struct JsonConverter<QJsonArray> : public TrivialJsonDumper<QJsonArray> {
    static auto load(const QJsonValue& jv) { return jv.toArray(); }
};

template <>
struct JsonConverter<QVariant> {
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
    static void dumpTo(QJsonArray& ar, const VectorT& vals)
    {
        for (const auto& v : vals)
            ar.push_back(toJson(v));
    }
    static auto dump(const VectorT& vals)
    {
        QJsonArray ja;
        dumpTo(ja, vals);
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
            json.insert(toJson(e), QJsonObject {});
    }
    static auto fillFrom(const QJsonObject& json, QSet<QString>& s)
    {
        s.reserve(s.size() + json.size());
        for (auto it = json.begin(); it != json.end(); ++it)
            s.insert(it.key());
        return s;
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
        h.reserve(jo.size());
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

template <>
struct JsonConverter<QVariantHash> {
    static QJsonObject dump(const QVariantHash& vh);
    static QVariantHash load(const QJsonValue& jv);
};

// Conditional insertion into a QJsonObject

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
    struct AddNode<ValT, false, decltype(std::declval<ValT>().isEmpty())> {
        template <typename ContT, typename ForwardedT>
        static void impl(ContT& container, const QString& key,
                         ForwardedT&& value)
        {
            if (!value.isEmpty())
                addTo(container, key, std::forward<ForwardedT>(value));
        }
    };

    // This one unfolds Omittable<> (also only when Force is false)
    template <typename ValT>
    struct AddNode<Omittable<ValT>, false> {
        template <typename ContT, typename OmittableT>
        static void impl(ContT& container, const QString& key,
                         const OmittableT& value)
        {
            if (value)
                addTo(container, key, *value);
        }
    };
} // namespace _impl

static constexpr bool IfNotEmpty = false;

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
} // namespace Quotient
