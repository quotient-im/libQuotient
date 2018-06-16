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
#include <QtCore/QUrlQuery>

#include <unordered_map>
#include <vector>
#include <functional>
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
            return qHash(s
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
                         , uint(qGlobalQHashSeed())
#endif
                         );
        }
    };
}

class QVariant;

namespace QMatrixClient
{
    struct NoneTag {};
    constexpr NoneTag none {};

    /** A crude substitute for `optional` while we're not C++17
     *
     * Only works with default-constructible types.
     */
    template <typename T>
    class Omittable
    {
        public:
            explicit Omittable() : Omittable(none) { }
            Omittable(NoneTag) : _omitted(true) { }
            Omittable(const T& val) : _value(val), _omitted(false) { }
            Omittable(T&& val) : _value(std::move(val)), _omitted(false) { }
            Omittable<T>& operator=(const T& val)
            {
                _value = val;
                _omitted = false;
                return *this;
            }
            Omittable<T>& operator=(T&& val)
            {
                _value = std::move(val);
                _omitted = false;
                return *this;
            }

            bool omitted() const { return _omitted; }
            const T& value() const { return _value; }
            T& value() { return _value; }
            T&& release() { _omitted = true; return std::move(_value); }

            operator bool() const { return !omitted(); }

        private:
            T _value;
            bool _omitted;
    };

    // This catches anything implicitly convertible to QJsonValue/Object/Array
    inline auto toJson(const QJsonValue& val) { return val; }
    inline auto toJson(const QJsonObject& o) { return o; }
    inline auto toJson(const QJsonArray& arr) { return arr; }
    // Special-case QStrings and bools to avoid ambiguity between QJsonValue
    // and QVariant (also, QString.isEmpty() is used in _impl::AddNode<> below)
    inline auto toJson(const QString& s) { return s; }

    inline QJsonArray toJson(const QStringList& strings)
    {
        return QJsonArray::fromStringList(strings);
    }

    inline QString toJson(const QByteArray& bytes)
    {
        return bytes.constData();
    }

    // QVariant is outrageously omnivorous - it consumes whatever is not
    // exactly matching the signature of other toJson overloads. The trick
    // below disables implicit conversion to QVariant through its numerous
    // non-explicit constructors.
    QJsonValue variantToJson(const QVariant& v);
    template <typename T>
    inline auto toJson(T&& var)
        -> std::enable_if_t<std::is_same<std::decay_t<T>, QVariant>::value,
                            QJsonValue>
    {
        return variantToJson(var);
    }
    QJsonObject toJson(const QMap<QString, QVariant>& map);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 5, 0))
    QJsonObject toJson(const QHash<QString, QVariant>& hMap);
#endif

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

    template <> struct FromJson<QByteArray>
    {
        auto operator()(const QJsonValue& jv) const
        {
            return fromJson<QString>(jv).toLatin1();
        }
    };

    template <> struct FromJson<QVariant>
    {
        QVariant operator()(const QJsonValue& jv) const;
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

    template <> struct FromJson<QMap<QString, QVariant>>
    {
        QMap<QString, QVariant> operator()(const QJsonValue& jv) const;
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

#if (QT_VERSION >= QT_VERSION_CHECK(5, 5, 0))
    template <> struct FromJson<QHash<QString, QVariant>>
    {
        QHash<QString, QVariant> operator()(const QJsonValue& jv) const;
    };
#endif

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
        template <typename ValT>
        inline void addTo(QJsonObject& o, const QString& k, ValT&& v)
        { o.insert(k, toJson(v)); }

        template <typename ValT>
        inline void addTo(QUrlQuery& q, const QString& k, ValT&& v)
        { q.addQueryItem(k, QString("%1").arg(v)); }

        // OpenAPI is entirely JSON-based, which means representing bools as
        // textual true/false, rather than 1/0.
        inline void addTo(QUrlQuery& q, const QString& k, bool v)
        {
            q.addQueryItem(k, v ? QStringLiteral("true")
                                : QStringLiteral("false"));
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
        struct AddNode<ValT, false,
                       decltype(std::declval<ValT>().isEmpty())>
        {
            template <typename ContT, typename ForwardedT>
            static void impl(ContT& container, const QString& key,
                             ForwardedT&& value)
            {
                if (!value.isEmpty())
                    AddNode<ValT>::impl(container,
                                        key, std::forward<ForwardedT>(value));
            }
        };

        // This is a special one that unfolds Omittable<>
        template <typename ValT, bool Force>
        struct AddNode<Omittable<ValT>, Force>
        {
            template <typename ContT, typename OmittableT>
            static void impl(ContT& container,
                             const QString& key, const OmittableT& value)
            {
                if (!value.omitted())
                    AddNode<ValT>::impl(container, key, value.value());
                else if (Force) // Edge case, no value but must put something
                    AddNode<ValT>::impl(container, key, QString{});
            }
        };
    }  // namespace _impl

    static constexpr bool IfNotEmpty = false;

    template <bool Force = true, typename ContT, typename ValT>
    inline void addParam(ContT& container, const QString& key, ValT&& value)
    {
        _impl::AddNode<std::decay_t<ValT>, Force>
                ::impl(container, key, std::forward<ValT>(value));
    }
}  // namespace QMatrixClient
