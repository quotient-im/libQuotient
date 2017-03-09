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

/**
 * @file logging_util.h - a collection of utilities to facilitate debug logging.
 */

#pragma once

#include <QtCore/QDebug>

namespace QMatrixClient
{

    // QDebug manipulators

    using QDebugManip = QDebug (*)(QDebug);

    /**
     * @brief QDebug manipulator to setup the stream for JSON output.
     *
     * Originally made to encapsulate the change in QDebug behavior in Qt 5.4
     * and the respective addition of QDebug::noquote().
     * Together with the operator<<() helper, the proposed usage is
     * (similar to std:: I/O manipulators):
     *
     * @example qDebug() << formatJson << json_object; // (QJsonObject, etc.)
     */
    static QDebugManip formatJson = [](QDebug debug_object) {
    #if QT_VERSION < QT_VERSION_CHECK(5, 4, 0)
            return debug_object;
    #else
            return debug_object.noquote();
    #endif
        };

    /**
     * @brief A helper operator to facilitate usage of formatJson (and possibly
     * other manipulators)
     *
     * @param debug_object to output the json to
     * @param qdm a QDebug manipulator
     * @return a copy of debug_object that has its mode altered by qdm
     */
    inline QDebug operator<< (QDebug debug_object, QDebugManip qdm)
    {
        return qdm(debug_object);
    }

    /**
     * @brief A crude wrapper around a container of pointers that owns pointers
     * to contained objects
     *
     * Similar to vector<unique_ptr<>>, upon deletion, this wrapper
     * will delete all events contained in it. This wrapper can be used
     * over Qt containers, which are incompatible with unique_ptr and even
     * with QScopedPointer (which is the reason of its creation).
     */
    template <typename ContainerT>
    class Owning : public ContainerT
    {
        public:
            Owning() = default;
#if defined(_MSC_VER) && _MSC_VER < 1900
            // Workaround: Dangerous (auto_ptr style) copy constructor because
            // in case of Owning< QVector<> > VS2013 (unnecessarily) instantiates
            // QVector<>::toList() which instantiates QList< Owning<> > which
            // requires the contained object to have a copy constructor.
            Owning(Owning& other) : ContainerT(other.release()) { }
            Owning(Owning&& other) : ContainerT(other.release()) { }
#else
            Owning(Owning&) = delete;
            Owning(Owning&& other) = default;
#endif
            Owning& operator=(Owning&& other)
            {
                assign(other.release());
                return *this;
            }

            ~Owning() { cleanup(); }

            void assign(ContainerT&& other)
            {
                if (&other == this)
                    return;
                cleanup();
                ContainerT::operator=(other);
            }

            /**
             * @brief returns the underlying container and releases the ownership
             *
             * Acts similar to unique_ptr::release.
             */
            ContainerT release()
            {
                ContainerT c;
                ContainerT::swap(c);
                return c;
            }
        private:
            void cleanup() { for (auto e: *this) delete e; }
    };

    /**
     * @brief Lookup a value by a key in a varargs list
     *
     * This function template takes the value of its first argument (selector)
     * as a key and searches for it in the key-value map passed in a varargs list
     * (every next pair of arguments forms a key-value pair). If a match is found,
     * the respective value is returned; if no pairs matched, the last value
     * (fallback) is returned.
     *
     * All options should be of the same type or implicitly castable to the
     * type of the first option. Note that pointers to methods of different
     * classes are of different object types, in particular.
     *
     * Below is an example of usage to select a parser depending on contents of
     * a JSON object:
     * {@code
     *  auto parser = lookup(obj.value["type"].toString(),
     *                      "type1", fn1,
     *                      "type2", fn2,
     *                      fallbackFn);
     *  parser(obj);
     * }
     *
     * The implementation is based on tail recursion; every recursion step
     * removes 2 arguments (match and option). There's no selector value for the
     * fallback option (the last one); therefore, the total number of lookup()
     * arguments should be even: selector + n key-value pairs + fallback
     *
     * @note Beware of calling lookup() with a <code>const char*</code> selector
     * (the first parameter) - most likely it won't do what you expect because
     * of shallow comparison.
     */
    template <typename ValueT, typename SelectorT, typename KeyT, typename... Ts>
    ValueT lookup(SelectorT selector, KeyT key, ValueT value, Ts... remainingMapping)
    {
        if( selector == key )
            return value;

        // Drop the failed key-value pair and recurse with 2 arguments less.
        return lookup(selector, remainingMapping...);
    }

    template <typename SelectorT, typename ValueT>
    ValueT lookup(SelectorT/*unused*/, ValueT fallback)
    {
        return fallback;
    }

}

