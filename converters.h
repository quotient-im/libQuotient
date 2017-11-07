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
    template <typename T>
    inline QJsonValue toJson(T val)
    {
        return QJsonValue(val);
    }

    template <typename T>
    inline QJsonValue toJson(const QVector<T>& vals)
    {
        QJsonArray ar;
        for (const auto& v: vals)
            ar.push_back(toJson(v));
        return ar;
    }

    inline QJsonValue toJson(const QStringList& strings)
    {
        return QJsonArray::fromStringList(strings);
    }

    template <typename T>
    struct FromJson
    {
        T operator()(QJsonValue jv) const { return static_cast<T>(jv); }
    };

    template <typename T>
    inline T fromJson(const QJsonValue& jv)
    {
        return FromJson<T>()(jv);
    }

    template <> struct FromJson<bool>
    {
        bool operator()(QJsonValue jv) const { return jv.toBool(); }
    };

    template <> struct FromJson<int>
    {
        int operator()(QJsonValue jv) const { return jv.toInt(); }
    };

    template <> struct FromJson<double>
    {
        double operator()(QJsonValue jv) const { return jv.toDouble(); }
    };

    template <> struct FromJson<qint64>
    {
        qint64 operator()(QJsonValue jv) const { return qint64(jv.toDouble()); }
    };

    template <> struct FromJson<QString>
    {
        QString operator()(QJsonValue jv) const { return jv.toString(); }
    };

    template <> struct FromJson<QDateTime>
    {
        QDateTime operator()(QJsonValue jv) const
        {
            return QDateTime::fromMSecsSinceEpoch(fromJson<qint64>(jv), Qt::UTC);
        }
    };

    template <> struct FromJson<QDate>
    {
        QDate operator()(QJsonValue jv) const
        {
            return fromJson<QDateTime>(jv).date();
        }
    };

    template <> struct FromJson<QJsonObject>
    {
        QJsonObject operator()(QJsonValue jv) const { return jv.toObject(); }
    };

    template <> struct FromJson<QJsonArray>
    {
        QJsonArray operator()(QJsonValue jv) const { return jv.toArray(); }
    };

    template <typename T> struct FromJson<QVector<T>>
    {
        QVector<T> operator()(QJsonValue jv) const
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
        QList<T> operator()(QJsonValue jv) const
        {
            const auto jsonArray = jv.toArray();
            QList<T> sl; sl.reserve(jsonArray.size());
            std::transform(jsonArray.begin(), jsonArray.end(),
                           std::back_inserter(sl), FromJson<T>());
            return sl;
        }
    };

    template <> struct FromJson<QStringList> : FromJson<QList<QString>> { };

}  // namespace QMatrixClient
