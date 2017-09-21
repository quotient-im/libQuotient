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

#include <QtCore/QJsonValue>
#include <QtCore/QJsonArray>
#include <QtCore/QDate>
#include <QtCore/QVariant>

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
    inline T fromJson(const QJsonValue& jv)
    {
        return QVariant(jv).value<T>();
    }

    template <>
    inline int fromJson<int>(const QJsonValue& jv)
    {
        return jv.toInt();
    }

    template <>
    inline qint64 fromJson<qint64>(const QJsonValue& jv)
    {
        return static_cast<qint64>(jv.toDouble());
    }

    template <>
    inline double fromJson<double>(const QJsonValue& jv)
    {
        return jv.toDouble();
    }

    template <>
    inline QString fromJson<QString>(const QJsonValue& jv)
    {
        return jv.toString();
    }

    template <>
    inline QDateTime fromJson<QDateTime>(const QJsonValue& jv)
    {
        return QDateTime::fromMSecsSinceEpoch(fromJson<qint64>(jv), Qt::UTC);
    }

    template <>
    inline QDate fromJson<QDate>(const QJsonValue& jv)
    {
        return fromJson<QDateTime>(jv).date();
    }
}  // namespace QMatrixClient
