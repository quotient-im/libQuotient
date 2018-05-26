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
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#pragma once

#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray> // Includes <QtCore/QJsonValue>
#include <QtCore/QDate>

#include <unordered_map>
#include <vector>
#if 0 // Waiting for C++17
#include <experimental/optional>

template <typename T>
using optional = std::experimental::optional<T>;
#endif

// Enable std::unordered_map<QString, T>
namespace std
{
    template <> struct hash<QString>
    {
        size_t operator()(const QString& s) const Q_DECL_NOEXCEPT
        {
            return qHash(s, uint(qGlobalQHashSeed()));
        }
    };
}

namespace QMatrixClient
{
    // This catches anything implicitly convertible to QJsonValue/Object/Array
    inline QJsonValue toJson(const QJsonValue& val) { return val; }
    inline QJsonObject toJson(const QJsonObject& o) { return o; }
    inline QJsonArray toJson(const QJsonArray& arr) { return arr; }
    // Special-case QStrings so that we could use isEmpty() on them
    // (see _impl::AddNote<> below)
    inline QString toJson(const QString& s) { return s; }

    template <typename T>
    inline QJsonArray toJson(const std::vector<T>& vals)
    {
        QJsonArray ar;
        for (const auto& v: vals)
            ar.push_back(toJson(v));
        return ar;
    }

    template <typename T>
    inline QJsonArray toJson(const QVector<T>& vals)
    {
        QJsonArray ar;
        for (const auto& v: vals)
            ar.push_back(toJson(v));
        return ar;
    }

    inline QJsonArray toJson(const QStringList& strings)
    {
        return QJsonArray::fromStringList(strings);
    }

    inline QString toJson(const QByteArray& bytes)
    {
        return bytes.constData();
    }

    template <typename T>
    inline QJsonObject toJson(const QHash<QString, T>& hashMap)
    {
        QJsonObject json;
        for (auto it = hashMap.begin(); it != hashMap.end(); ++it)
            json.insert(it.key(), toJson(it.value()));
        return json;
    }

    template <typename T>
    inline QJsonObject toJson(const std::unordered_map<QString, T>& hashMap)
    {
        QJsonObject json;
        for (auto it = hashMap.begin(); it != hashMap.end(); ++it)
            json.insert(it.key(), toJson(it.value()));
        return json;
    }

#if 0
    template <typename T>
    inline auto toJson(const optional<T>& optVal)
    {
        if (optVal)
            return toJson(optVal.value());

        return decltype(toJson(std::declval<T>()))();
    }
#endif

    template <typename T>
    struct FromJson
    {
        T operator()(const QJsonValue& jv) const { return static_cast<T>(jv); }
    };

    template <typename T>
    inline T fromJson(const QJsonValue& jv)
    {
        return FromJson<T>()(jv);
    }

    template <> struct FromJson<bool>
    {
        auto operator()(const QJsonValue& jv) const { return jv.toBool(); }
    };

    template <> struct FromJson<int>
    {
        auto operator()(const QJsonValue& jv) const { return jv.toInt(); }
    };

    template <> struct FromJson<double>
    {
        auto operator()(const QJsonValue& jv) const { return jv.toDouble(); }
    };

    template <> struct FromJson<qint64>
    {
        auto operator()(const QJsonValue& jv) const { return qint64(jv.toDouble()); }
    };

    template <> struct FromJson<QString>
    {
        auto operator()(const QJsonValue& jv) const { return jv.toString(); }
    };

    template <> struct FromJson<QDateTime>
    {
        auto operator()(const QJsonValue& jv) const
        {
            return QDateTime::fromMSecsSinceEpoch(fromJson<qint64>(jv), Qt::UTC);
        }
    };

    template <> struct FromJson<QDate>
    {
        auto operator()(const QJsonValue& jv) const
        {
            return fromJson<QDateTime>(jv).date();
        }
    };

    template <> struct FromJson<QJsonObject>
    {
        auto operator()(const QJsonValue& jv) const
        {
            return jv.toObject();
        }
    };

    template <> struct FromJson<QJsonArray>
    {
        auto operator()(const QJsonValue& jv) const
        {
            return jv.toArray();
        }
    };

    template <typename T> struct FromJson<std::vector<T>>
    {
        auto operator()(const QJsonValue& jv) const
        {
            using size_type = typename std::vector<T>::size_type;
            const auto jsonArray = jv.toArray();
            std::vector<T> vect; vect.resize(size_type(jsonArray.size()));
            std::transform(jsonArray.begin(), jsonArray.end(),
                           vect.begin(), FromJson<T>());
            return vect;
        }
    };

    template <typename T> struct FromJson<QVector<T>>
    {
        auto operator()(const QJsonValue& jv) const
        {
            const auto jsonArray = jv.toArray();
            QVector<T> vect; vect.resize(jsonArray.size());
            std::transform(jsonArray.begin(), jsonArray.end(),
                           vect.begin(), FromJson<T>());
            return vect;
        }
    };

    template <typename T> struct FromJson<QList<T>>
    {
        auto operator()(const QJsonValue& jv) const
        {
            const auto jsonArray = jv.toArray();
            QList<T> sl; sl.reserve(jsonArray.size());
            std::transform(jsonArray.begin(), jsonArray.end(),
                           std::back_inserter(sl), FromJson<T>());
            return sl;
        }
    };

    template <> struct FromJson<QStringList> : FromJson<QList<QString>> { };

    template <> struct FromJson<QByteArray>
    {
        auto operator()(const QJsonValue& jv) const
        {
            return fromJson<QString>(jv).toLatin1();
        }
    };

    template <typename T> struct FromJson<QHash<QString, T>>
    {
        auto operator()(const QJsonValue& jv) const
        {
            const auto json = jv.toObject();
            QHash<QString, T> h; h.reserve(json.size());
            for (auto it = json.begin(); it != json.end(); ++it)
                h.insert(it.key(), fromJson<T>(it.value()));
            return h;
        }
    };

    template <typename T> struct FromJson<std::unordered_map<QString, T>>
    {
        auto operator()(const QJsonValue& jv) const
        {
            const auto json = jv.toObject();
            std::unordered_map<QString, T> h; h.reserve(size_t(json.size()));
            for (auto it = json.begin(); it != json.end(); ++it)
                h.insert(std::make_pair(it.key(), fromJson<T>(it.value())));
            return h;
        }
    };

    // Conditional insertion into a QJsonObject

    namespace _impl
    {
        // This one is for types that don't have isEmpty()
        template <typename JsonT, typename = bool>
        struct AddNode
        {
            static void impl(QJsonObject& o, QString key, JsonT&& value)
            {
                o.insert(std::move(key), std::forward<JsonT>(value));
            }
        };

        // This one is for types that have isEmpty()
        template <typename JsonT>
        struct AddNode<JsonT, decltype(std::declval<JsonT>().isEmpty())>
        {
            static void impl(QJsonObject& o, QString key, JsonT&& value)
            {
                if (!value.isEmpty())
                    o.insert(std::move(key), std::forward<JsonT>(value));
            }
        };
    }

    static constexpr bool IfNotEmpty = false;

    template <bool Force = true, typename ValT = void>
    inline void addToJson(QJsonObject& o, QString key, ValT&& value)
    {
        auto&& json = toJson(std::forward<ValT>(value));
        // QJsonValue doesn't have isEmpty and consumes all other types
        // (QString, QJsonObject etc.).
        _impl::AddNode<std::conditional_t<Force, QJsonValue, decltype(json)>>
                ::impl(o, std::move(key), std::move(json));
    }

    /** Construct an "omitted" value of a given type
     * This is a workaround for the time being while we're not C++17 and
     * cannot use `optional`.
     */
    template <typename T>
    inline T omitted()
    {
        T val;
        val.omitted = true;
        return val;
    }
}  // namespace QMatrixClient
