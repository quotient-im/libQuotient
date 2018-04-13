/******************************************************************************
 * Copyright (C) 2016 Kitsune Ral <kitsune-ral@users.sf.net>
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

#include <QtCore/QMetaEnum>
#include <QtCore/QDebug>

#include <functional>
#include <memory>

namespace QMatrixClient
{
    // The below enables pretty-printing of enums in logs
#if (QT_VERSION >= QT_VERSION_CHECK(5, 5, 0))
#define REGISTER_ENUM(EnumName) Q_ENUM(EnumName)
#else
    // Thanks to Olivier for spelling it and for making Q_ENUM to replace it:
    // https://woboq.com/blog/q_enum.html
#define REGISTER_ENUM(EnumName) \
    Q_ENUMS(EnumName) \
    friend QDebug operator<<(QDebug dbg, EnumName val) \
    { \
        static int enumIdx = staticMetaObject.indexOfEnumerator(#EnumName); \
        return dbg << Event::staticMetaObject.enumerator(enumIdx).valueToKey(int(val)); \
    }
#endif

    template <typename T1, typename PtrT2>
    inline auto unique_ptr_cast(PtrT2&& p)
    {
        return std::unique_ptr<T1>(static_cast<T1*>(p.release()));
    }

#if QT_VERSION < QT_VERSION_CHECK(5, 7, 0)
    // Copy-pasted from Qt 5.10
    template <typename T>
    Q_DECL_CONSTEXPR typename std::add_const<T>::type &qAsConst(T &t) Q_DECL_NOTHROW { return t; }
    // prevent rvalue arguments:
    template <typename T>
    static void qAsConst(const T &&) Q_DECL_EQ_DELETE;
#endif
}  // namespace QMatrixClient

