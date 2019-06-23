/******************************************************************************
 * Copyright (C) 2017 Kitsune Ral <kitsune-ral@users.sf.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

#pragma once

#include "util.h"

#include <QtCore/QDate>
#include <QtCore/QJsonArray> // Includes <QtCore/QJsonValue>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QSet>
#include <QtCore/QUrlQuery>
#include <QtCore/QVector>

#include <unordered_map>
#include <vector>
#if 0 // Waiting for C++17
#    include <experimental/optional>

template <typename T>
using optional = std::experimental::optional<T>;
#endif

// Enable std::unordered_map<QString, T>
namespace std
{
template <>
struct hash<QString>
{
    size_t operator()(const QString& s) const Q_DECL_NOEXCEPT
    {
        return qHash(s
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
                     ,
                     uint(qGlobalQHashSeed())
#endif
        );
    }
};
} // namespace std

class QVariant;

namespace QMatrixClient
{
template <typename T>
struct JsonObjectConverter
{
    static void dumpTo(QJsonObject& jo, const T& pod) { jo = pod; }
    static void fillFrom(const QJsonObject& jo, T& pod) { pod = jo; }
};

template <typename T>
struct JsonConverter
{
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

template <typename T>
inline auto fillJson(QJsonObject& json, const T& data)
{
    JsonObjectConverter<T>::dumpTo(json, data);
}

template <typename T>
inline auto fromJson(const QJsonValue& jv)
{
    return JsonConverter<T>::load(jv);
}

template <typename T>
inline T fromJson(const QJsonDocument& jd)
{
    return JsonConverter<T>::load(jd);
}

template <typename T>
inline void fromJson(const QJsonValue& jv, T& pod)
{
    if (!jv.isUndefined())
        pod = fromJson<T>(jv);
}

template <typename T>
inline void fromJson(const QJsonDocument& jd, T& pod)
{
    pod = fromJson<T>(jd);
}

// Unfolds Omittable<>
template <typename T>
inline void fromJson(const QJsonValue& jv, Omittable<T>& pod)
{
    if (jv.isUndefined())
        pod = none;
    else
        pod = fromJson<T>(jv);
}

template <typename T>
inline void fillFromJson(const QJsonValue& jv, T& pod)
{
    if (jv.isObject())
        JsonObjectConverter<T>::fillFrom(jv.toObject(), pod);
}

// JsonConverter<> specialisations

template <typename T>
struct TrivialJsonDumper
{
    // Works for: QJsonValue (and all things it can consume),
    // QJsonObject, QJsonArray
    static auto dump(const T& val) { return val; }
};

template <>
struct JsonConverter<bool> : public TrivialJsonDumper<bool>
{
    static auto load(const QJsonValue& jv) { return jv.toBool(); }
};

template <>
struct JsonConverter<int> : public TrivialJsonDumper<int>
{
    static auto load(const QJsonValue& jv) { return jv.toInt(); }
};

template <>
struct JsonConverter<double> : public TrivialJsonDumper<double>
{
    static auto load(const QJsonValue& jv) { return jv.toDouble(); }
};

template <>
struct JsonConverter<float> : public TrivialJsonDumper<float>
{
    static auto load(const QJsonValue& jv) { return float(jv.toDouble()); }
};

template <>
struct JsonConverter<qint64> : public TrivialJsonDumper<qint64>
{
    static auto load(const QJsonValue& jv) { return qint64(jv.toDouble()); }
};

template <>
struct JsonConverter<QString> : public TrivialJsonDumper<QString>
{
    static auto load(const QJsonValue& jv) { return jv.toString(); }
};

template <>
struct JsonConverter<QDateTime>
{
    static auto dump(const QDateTime& val) = delete; // not provided yet
    static auto load(const QJsonValue& jv)
    {
        return QDateTime::fromMSecsSinceEpoch(fromJson<qint64>(jv), Qt::UTC);
    }
};

template <>
struct JsonConverter<QDate>
{
    static auto dump(const QDate& val) = delete; // not provided yet
    static auto load(const QJsonValue& jv)
    {
        return fromJson<QDateTime>(jv).date();
    }
};

template <>
struct JsonConverter<QJsonArray> : public TrivialJsonDumper<QJsonArray>
{
    static auto load(const QJsonValue& jv) { return jv.toArray(); }
};

template <>
struct JsonConverter<QByteArray>
{
    static QString dump(const QByteArray& ba) { return ba.constData(); }
    static auto load(const QJsonValue& jv)
    {
        return fromJson<QString>(jv).toLatin1();
    }
};

template <>
struct JsonConverter<QVariant>
{
    static QJsonValue dump(const QVariant& v);
    static QVariant load(const QJsonValue& jv);
};

template <typename VectorT, typename T = typename VectorT::value_type>
struct JsonArrayConverter
{
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
struct JsonConverter<std::vector<T>> : public JsonArrayConverter<std::vector<T>>
{};

template <typename T>
struct JsonConverter<QVector<T>> : public JsonArrayConverter<QVector<T>>
{};

template <typename T>
struct JsonConverter<QList<T>> : public JsonArrayConverter<QList<T>>
{};

template <>
struct JsonConverter<QStringList> : public JsonConverter<QList<QString>>
{
    static auto dump(const QStringList& sl)
    {
        return QJsonArray::fromStringList(sl);
    }
};

template <>
struct JsonObjectConverter<QSet<QString>>
{
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
struct HashMapFromJson
{
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

template <typename T>
struct JsonObjectConverter<std::unordered_map<QString, T>>
    : public HashMapFromJson<std::unordered_map<QString, T>>
{};

template <typename T>
struct JsonObjectConverter<QHash<QString, T>>
    : public HashMapFromJson<QHash<QString, T>>
{};

// We could use std::conditional<> below but QT_VERSION* macros in C++ code
// cause (kinda valid but useless and noisy) compiler warnings about
// bitwise operations on signed integers; so use the preprocessor for now.
using variant_map_t =
#if (QT_VERSION >= QT_VERSION_CHECK(5, 5, 0))
    QVariantHash;
#else
    QVariantMap;
#endif
template <>
struct JsonConverter<variant_map_t>
{
    static QJsonObject dump(const variant_map_t& vh);
    static QVariantHash load(const QJsonValue& jv);
};

// Conditional insertion into a QJsonObject

namespace _impl
{
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

    inline void addTo(QUrlQuery& q, const QString& k, const QStringList& vals)
    {
        for (const auto& v : vals)
            q.addQueryItem(k, v);
    }

    inline void addTo(QUrlQuery& q, const QString&, const QJsonObject& vals)
    {
        for (auto it = vals.begin(); it != vals.end(); ++it)
            q.addQueryItem(it.key(), it.value().toString());
    }

    // This one is for types that don't have isEmpty()
    template <typename ValT, bool Force = true, typename = bool>
    struct AddNode
    {
        template <typename ContT, typename ForwardedT>
        static void impl(ContT& container, const QString& key,
                         ForwardedT&& value)
        {
            addTo(container, key, std::forward<ForwardedT>(value));
        }
    };

    // This one is for types that have isEmpty()
    template <typename ValT>
    struct AddNode<ValT, false, decltype(std::declval<ValT>().isEmpty())>
    {
        template <typename ContT, typename ForwardedT>
        static void impl(ContT& container, const QString& key,
                         ForwardedT&& value)
        {
            if (!value.isEmpty())
                AddNode<ValT>::impl(container, key,
                                    std::forward<ForwardedT>(value));
        }
    };

    // This is a special one that unfolds Omittable<>
    template <typename ValT, bool Force>
    struct AddNode<Omittable<ValT>, Force>
    {
        template <typename ContT, typename OmittableT>
        static void impl(ContT& container, const QString& key,
                         const OmittableT& value)
        {
            if (!value.omitted())
                AddNode<ValT>::impl(container, key, value.value());
            else if (Force) // Edge case, no value but must put something
                AddNode<ValT>::impl(container, key, QString {});
        }
    };

#if 0
        // This is a special one that unfolds optional<>
        template <typename ValT, bool Force>
        struct AddNode<optional<ValT>, Force>
        {
            template <typename ContT, typename OptionalT>
            static void impl(ContT& container,
                             const QString& key, const OptionalT& value)
            {
                if (value)
                    AddNode<ValT>::impl(container, key, value.value());
                else if (Force) // Edge case, no value but must put something
                    AddNode<ValT>::impl(container, key, QString{});
            }
        };
#endif

} // namespace _impl

static constexpr bool IfNotEmpty = false;

template <bool Force = true, typename ContT, typename ValT>
inline void addParam(ContT& container, const QString& key, ValT&& value)
{
    _impl::AddNode<std::decay_t<ValT>, Force>::impl(container, key,
                                                    std::forward<ValT>(value));
}
} // namespace QMatrixClient
