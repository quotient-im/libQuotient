/******************************************************************************
 * Copyright (C) 2015 Kitsune Ral <kitsune-ral@users.sf.net>
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
#include <QtCore/QVariant>

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QPair>
#include <QtCore/QDateTime>
#include <QtCore/QUrl>

namespace QMatrixClient
{
    /**
     * This wrapper around QJsonValue adds the following features:
     * - Seamless creation from QJson* classes
     * - to() template method that extracts a value of a desired type;
     *   the default implementation is rather heavy, involving QVariant's
     *   capabilities; specialisations are supposed to use available
     *   toInt/toString/etc. instead (see example specialisations after the class).
     * - Overloaded operator[] that can be used for indexed access to a pair
     *   in a QJsonObject or an element in a QJsonArray, depending on the type
     *   of the passed index.
     *
     * The main purpose of the class is to make code that navigates through
     * a JSON tree more concise, in particular saving on intermediate
     * toObject()/toArray() calls.
     */
    class JsonValue : public QJsonValue
    {
        public:
            JsonValue(const QJsonValue &source) : QJsonValue(source) { }

            // Implies QJsonValueRef and anything castable to it
            template <class SourceValT>
            explicit JsonValue(const SourceValT &source) : QJsonValue(source) { }

            template <typename T>
            T to() const
            {
                return QVariant(*this).value<T>();
            }

            JsonValue operator[](QJsonObject::key_type key) const
            {
                return JsonValue(toObject()[key]);
            }
            JsonValue operator[](QJsonArray::size_type i) const
            {
                return JsonValue(toArray()[i]);
            }
    };

    // A couple of examples how to specialise JsonValue::to() in your code
    template <>
    inline QUrl JsonValue::to() const
    {
        return QUrl(toString());
    }

    template <>
    inline QDateTime JsonValue::to() const
    {
        return QDateTime::fromMSecsSinceEpoch( qint64(toDouble()) );
    }

    /**
     * This is a simple wrapper around a key-value pair stored in QJsonObject
     * with key() and value() accessors available in addition to QPair's native
     * interface.
     */
    class JsonPair : public QPair<QString, JsonValue>
    {
        public:
            JsonPair(QString k, QJsonValue v) : QPair( k,JsonValue(v) ) { }
            explicit JsonPair(const QJsonObject::const_iterator& it)
                : QPair( it.key(), it.value() ) { }

            QString key() const { return first; }
            JsonValue value() const { return second; }
    };

    /**
     * This wrapper around QJsonObject adds the following features:
     * - allows to quickly start navigation using a chain of JsonValue::operator[]'s
     * - provides a const_iterator that references JsonPair's instead of
     *   QJsonValue's (@see adjust_iterator<>), therefore allowing to deal
     *   with keys, not only values.
     *
     * It overloads several inherited methods to return its own version
     * of const_iterator instead of QJsonObject::const_iterator. Since these
     * are not virtual, the class is NOT polymorphic and is not intended for
     * passing by reference/pointer.
     */
    class JsonObject : public QJsonObject
    {
        public:
            JsonObject(const QJsonObject& o) : QJsonObject(o) { }

            JsonObject(const JsonValue& n) : QJsonObject(n.toObject()) { }

            explicit JsonObject(const QJsonDocument& data)
                : QJsonObject (data.object()) { }

            JsonValue operator[] (const QString &key) const { return value(key); }

            template <typename ContT>
            bool containsAll(const ContT& keyList) const
            {
                return std::all_of(keyList.begin(), keyList.end(), &JsonObject::contains);
            }

            // A special overload for std::initializer_list because it's not
            // automatically deduced
            template <typename T>
            bool containsAll(const std::initializer_list<T>& keyList) const
            {
                return containsAll(keyList);
            }

            // STL compatibility
            using value_type = JsonPair;
            using key_type = JsonPair::first_type;
            using mapped_type = JsonPair::second_type;

        private:
            /**
             * This template class is used to change the behaviour of
             * QJsonObject iterators to be more compliant with ordinary hashmap
             * iterators. In particular, it changes the referenced item
             * (the one returned by operator*) from QJsonValue to a key-value
             * pair encapsulated into a JsonPair class. Respectively, key()
             * and value() are not provided by the iterator but rather by the
             * value_type class. This allows to use a range-based for statement
             * to iterate over key-value pairs (otherwise you have to resort
             * to the old for syntax if you need access to keys as well).
             *
             * This template is further specialised for const_iterator but
             * is also suitable as a base for a read-write iterator class.
             */
            template <class IterT>
            class adjust_iterator : public IterT
            {
                public:
                    /**
                     * This class is only needed to keep a pointer to
                     * a temporary JsonPair that is returned by the iterator's
                     * operator->(). Thanks to this class, you can write,
                     * e.g., iter->value() to get a JsonValue from the iterator
                     * It behaves as an "unmovable" pointer: the only thing
                     * you can do with it is dereferencing by either
                     * operator*() or operator->().
                     */
                    class JsonPairHolder
                    {
                        public:
                            JsonPairHolder(IterT& it) : obj(it) { }

                            JsonPair operator*() const { return obj; }
                            const JsonPair* operator->() const { return &obj; }
                        protected:
                            JsonPair obj;
                    };

                    using base_type = IterT;
                    using value_type = JsonObject::value_type;
                    using pointer = JsonPairHolder;

                    // Constructors
                    using base_type::base_type;
                    adjust_iterator(base_type base) : base_type(base) { }

                    value_type operator*() const { return value_type(*this); }
                    pointer operator->() const { return pointer(*this); }
            };

        public:
            using const_iterator = adjust_iterator<QJsonObject::const_iterator>;

            // STL style
            const_iterator begin() const { return QJsonObject::begin(); }
            const_iterator constBegin() const { return QJsonObject::constBegin(); }
            const_iterator end() const { return QJsonObject::end(); }
            const_iterator constEnd() const { return QJsonObject::constEnd(); }

            // more Qt
            typedef const_iterator ConstIterator;
            const_iterator find(const QString &key) const { return QJsonObject::find(key); }
            const_iterator constFind(const QString &key) const { return QJsonObject::constFind(key); }
    };
}
