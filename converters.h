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

namespace QMatrixClient
{
    // This catches anything implicitly convertible to QJsonValue/Object/Array
    inline QJsonValue toJson(const QJsonValue& val) { return val; }
    inline QJsonObject toJson(const QJsonObject& o) { return o; }
    inline QJsonArray toJson(const QJsonArray& arr) { return arr; }

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

    inline QJsonValue toJson(const QByteArray& bytes)
    {
        return QJsonValue(bytes.constData());
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
        bool operator()(const QJsonValue& jv) const { return jv.toBool(); }
    };

    template <> struct FromJson<int>
    {
        int operator()(const QJsonValue& jv) const { return jv.toInt(); }
    };

    template <> struct FromJson<double>
    {
        double operator()(const QJsonValue& jv) const { return jv.toDouble(); }
    };

    template <> struct FromJson<qint64>
    {
        qint64 operator()(const QJsonValue& jv) const { return qint64(jv.toDouble()); }
    };

    template <> struct FromJson<QString>
    {
        QString operator()(const QJsonValue& jv) const { return jv.toString(); }
    };

    template <> struct FromJson<QDateTime>
    {
        QDateTime operator()(const QJsonValue& jv) const
        {
            return QDateTime::fromMSecsSinceEpoch(fromJson<qint64>(jv), Qt::UTC);
        }
    };

    template <> struct FromJson<QDate>
    {
        QDate operator()(const QJsonValue& jv) const
        {
            return fromJson<QDateTime>(jv).date();
        }
    };

    template <> struct FromJson<QJsonObject>
    {
        QJsonObject operator()(const QJsonValue& jv) const
        {
            return jv.toObject();
        }
    };

    template <> struct FromJson<QJsonArray>
    {
        QJsonArray operator()(const QJsonValue& jv) const
        {
            return jv.toArray();
        }
    };

    template <typename T> struct FromJson<QVector<T>>
    {
        QVector<T> operator()(const QJsonValue& jv) const
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
        QList<T> operator()(const QJsonValue& jv) const
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
        inline QByteArray operator()(const QJsonValue& jv) const
        {
            return fromJson<QString>(jv).toLatin1();
        }
    };

    template <typename T> struct FromJson<QHash<QString, T>>
    {
        QHash<QString, T> operator()(const QJsonValue& jv) const
        {
            const auto json = jv.toObject();
            QHash<QString, T> h; h.reserve(json.size());
            for (auto it = json.begin(); it != json.end(); ++it)
                h.insert(it.key(), fromJson<T>(it.value()));
            return h;
        }
    };
}  // namespace QMatrixClient
