/******************************************************************************
* Copyright (C) 2018 Kitsune Ral <kitsune-ral@users.sf.net>
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

#include <QtCore/QVariant>

#include "converters.h"

namespace QMatrixClient
{
    inline QJsonObject toJson(const QVariantMap& map)
    {
        return QJsonObject::fromVariantMap(map);
    }

#if (QT_VERSION >= QT_VERSION_CHECK(5, 5, 0))
    inline QJsonObject toJson(const QVariantHash& hMap)
    {
        return QJsonObject::fromVariantHash(hMap);
    }
#endif

    template <> struct FromJson<QVariantMap>
    {
        auto operator()(const QJsonValue& jv) const
        {
            return jv.toObject().toVariantMap();
        }
    };

#if (QT_VERSION >= QT_VERSION_CHECK(5, 5, 0))
    template <> struct FromJson<QVariantHash>
    {
        auto operator()(const QJsonValue& jv) const
        {
            return jv.toObject().toVariantHash();
        }
    };
#endif
}
