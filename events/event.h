/******************************************************************************
 * Copyright (C) 2015 Felix Rohrbach <kde@fxrh.de>
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

#ifndef QMATRIXCLIENT_EVENT_H
#define QMATRIXCLIENT_EVENT_H

#include <algorithm>

#include <QtCore/QString>
#include <QtCore/QDateTime>
#include <QtCore/QJsonObject>
#include <QtCore/QVector>

class QJsonArray;

namespace QMatrixClient
{
    enum class EventType
    {
        RoomMessage, RoomName, RoomAliases, RoomCanonicalAlias,
        RoomMember, RoomTopic, Typing, Receipt, Unknown
    };
    
    class Event
    {
        public:
            explicit Event(EventType type);
            Event(Event&) = delete;
            virtual ~Event();
            
            EventType type() const;
            QString id() const;
            QDateTime timestamp() const;
            QString roomId() const;
            QString senderId() const;
            // only for debug purposes!
            QString originalJson() const;

            static Event* fromJson(const QJsonObject& obj);
            
        protected:
            bool parseJson(const QJsonObject& obj);
        
        private:
            class Private;
            Private* d;
    };
    using Events = QVector<Event*>;

    Events eventsFromJson(const QJsonArray& contents);

    /**
     * Finds a place in the timeline where a new event/message could be inserted.
     * @return an iterator to an item with the earliest timestamp after
     * the one of 'item'; or timeline.end(), if all events are earlier
     */
    template <class ItemT, class ContT>
    typename ContT::iterator
    findInsertionPos(ContT & timeline, const ItemT *item)
    {
        return std::lower_bound (timeline.begin(), timeline.end(), item,
            [](const typename ContT::value_type a, const ItemT * b) {
                // FIXME: We should not order the message list by origin timestamp.
                // Rather, an order of receiving should be used (which actually
                // poses a question on whether this method is needed at all -
                // or we'd just prepend and append, depending on whether we
                // received something from /sync or from /messages.
                return a->timestamp() < b->timestamp();
            }
        );
    }

    /**
     * @brief Lookup a value by a key in a varargs list
     *
     * The below overloaded function template takes the value of its first
     * argument (selector) as a key and searches for it in the key-value map
     * passed in a varargs list (every next pair of arguments forms a key-value
     * pair). If a match is found, the respective value is returned; otherwise,
     * the last value (fallback) is returned.
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

#endif // QMATRIXCLIENT_EVENT_H
