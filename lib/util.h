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

#include <QtCore/QPointer>
#if (QT_VERSION < QT_VERSION_CHECK(5, 5, 0))
#include <QtCore/QMetaEnum>
#include <QtCore/QDebug>
#endif

#include <functional>
#include <memory>

#if __cplusplus >= 201703L
#define FALLTHROUGH [[fallthrough]]
#elif __has_cpp_attribute(clang::fallthrough)
#define FALLTHROUGH [[clang::fallthrough]]
#elif __has_cpp_attribute(gnu::fallthrough)
#define FALLTHROUGH [[gnu::fallthrough]]
#else
#define FALLTHROUGH // -fallthrough
#endif

// Along the lines of Q_DISABLE_COPY
#define DISABLE_MOVE(_ClassName) \
    _ClassName(_ClassName&&) Q_DECL_EQ_DELETE; \
    _ClassName& operator=(_ClassName&&) Q_DECL_EQ_DELETE;

#if QT_VERSION < QT_VERSION_CHECK(5, 7, 0)
// Copy-pasted from Qt 5.10
template <typename T>
Q_DECL_CONSTEXPR typename std::add_const<T>::type &qAsConst(T &t) Q_DECL_NOTHROW { return t; }
// prevent rvalue arguments:
template <typename T>
static void qAsConst(const T &&) Q_DECL_EQ_DELETE;
#endif

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

    /** static_cast<> for unique_ptr's */
    template <typename T1, typename PtrT2>
    inline auto unique_ptr_cast(PtrT2&& p)
    {
        return std::unique_ptr<T1>(static_cast<T1*>(p.release()));
    }

    struct NoneTag {};
    constexpr NoneTag none {};

    /** A crude substitute for `optional` while we're not C++17
     *
     * Only works with default-constructible types.
     */
    template <typename T>
    class Omittable
    {
            static_assert(!std::is_reference<T>::value,
                "You cannot make an Omittable<> with a reference type");
        public:
            using value_type = std::decay_t<T>;

            explicit Omittable() : Omittable(none) { }
            Omittable(NoneTag) : _value(value_type()), _omitted(true) { }
            Omittable(const value_type& val) : _value(val) { }
            Omittable(value_type&& val) : _value(std::move(val)) { }
            Omittable<T>& operator=(const value_type& val)
            {
                _value = val;
                _omitted = false;
                return *this;
            }
            Omittable<T>& operator=(value_type&& val)
            {
                _value = std::move(val);
                _omitted = false;
                return *this;
            }

            bool omitted() const { return _omitted; }
            const value_type& value() const
            {
                Q_ASSERT(!_omitted);
                return _value;
            }
            value_type& editValue()
            {
                _omitted = false;
                return _value;
            }
            value_type&& release() { _omitted = true; return std::move(_value); }

            operator value_type&() & { return editValue(); }
            const value_type* operator->() const & { return &value(); }
            value_type* operator->() & { return &editValue(); }
            const value_type& operator*() const & { return value(); }
            value_type& operator*() & { return editValue(); }

        private:
            T _value;
            bool _omitted = false;
    };

    namespace _impl {
        template <typename AlwaysVoid, typename> struct fn_traits;
    }

    /** Determine traits of an arbitrary function/lambda/functor
     * Doesn't work with generic lambdas and function objects that have
     * operator() overloaded.
     * \sa https://stackoverflow.com/questions/7943525/is-it-possible-to-figure-out-the-parameter-type-and-return-type-of-a-lambda#7943765
     */
    template <typename T>
    struct function_traits : public _impl::fn_traits<void, T> {};

    // Specialisation for a function
    template <typename ReturnT, typename... ArgTs>
    struct function_traits<ReturnT(ArgTs...)>
    {
        static constexpr auto is_callable = true;
        using return_type = ReturnT;
        using arg_types = std::tuple<ArgTs..., void>;
        static constexpr auto arg_number = std::tuple_size<arg_types>::value - 1;
    };

    namespace _impl {
        template <typename AlwaysVoid, typename T>
        struct fn_traits
        {
            static constexpr auto is_callable = false;
        };

        template <typename T>
        struct fn_traits<decltype(void(T::operator())), T>
            : public fn_traits<void, decltype(T::operator())>
        { }; // A generic function object that has (non-overloaded) operator()

        // Specialisation for a member function
        template <typename ReturnT, typename ClassT, typename... ArgTs>
        struct fn_traits<void, ReturnT(ClassT::*)(ArgTs...)>
            : function_traits<ReturnT(ArgTs...)>
        { };

        // Specialisation for a const member function
        template <typename ReturnT, typename ClassT, typename... ArgTs>
        struct fn_traits<void, ReturnT(ClassT::*)(ArgTs...) const>
            : function_traits<ReturnT(ArgTs...)>
        { };
    }  // namespace _impl

    template <typename FnT>
    using fn_return_t = typename function_traits<FnT>::return_type;

    template <typename FnT, int ArgN = 0>
    using fn_arg_t =
        std::tuple_element_t<ArgN, typename function_traits<FnT>::arg_types>;

    template <typename R, typename FnT>
    constexpr bool returns()
    {
        return std::is_same<fn_return_t<FnT>, R>::value;
    }

    // Poor-man's is_invokable
    template <typename T>
    constexpr auto is_callable_v = function_traits<T>::is_callable;

    inline auto operator"" _ls(const char* s, std::size_t size)
    {
        return QLatin1String(s, int(size));
    }

    /** An abstraction over a pair of iterators
     * This is a very basic range type over a container with iterators that
     * are at least ForwardIterators. Inspired by Ranges TS.
     */
    template <typename ArrayT>
    class Range
    {
            // Looking forward for Ranges TS to produce something (in C++23?..)
            using iterator = typename ArrayT::iterator;
            using const_iterator = typename ArrayT::const_iterator;
            using size_type = typename ArrayT::size_type;
        public:
            Range(ArrayT& arr) : from(std::begin(arr)), to(std::end(arr)) { }
            Range(iterator from, iterator to) : from(from), to(to) { }

            size_type size() const
            {
                Q_ASSERT(std::distance(from, to) >= 0);
                return size_type(std::distance(from, to));
            }
            bool empty() const { return from == to; }
            const_iterator begin() const { return from; }
            const_iterator end() const { return to; }
            iterator begin() { return from; }
            iterator end() { return to; }

        private:
            iterator from;
            iterator to;
    };

    /** A replica of std::find_first_of that returns a pair of iterators
     *
     * Convenient for cases when you need to know which particular "first of"
     * [sFirst, sLast) has been found in [first, last).
     */
    template<typename InputIt, typename ForwardIt, typename Pred>
    inline std::pair<InputIt, ForwardIt> findFirstOf(
            InputIt first, InputIt last, ForwardIt sFirst, ForwardIt sLast,
            Pred pred)
    {
        for (; first != last; ++first)
            for (auto it = sFirst; it != sLast; ++it)
                if (pred(*first, *it))
                    return std::make_pair(first, it);

        return std::make_pair(last, sLast);
    }

    /** A guard pointer that disconnects an interested object upon destruction
     * It's almost QPointer<> except that you have to initialise it with one
     * more additional parameter - a pointer to a QObject that will be
     * disconnected from signals of the underlying pointer upon the guard's
     * destruction.
     */
    template <typename T>
    class ConnectionsGuard : public QPointer<T>
    {
        public:
            ConnectionsGuard(T* publisher, QObject* subscriber)
                : QPointer<T>(publisher), subscriber(subscriber)
            { }
            ~ConnectionsGuard()
            {
                if (*this)
                    (*this)->disconnect(subscriber);
            }
            ConnectionsGuard(ConnectionsGuard&&) = default;
            ConnectionsGuard& operator=(ConnectionsGuard&&) = default;
            Q_DISABLE_COPY(ConnectionsGuard)
            using QPointer<T>::operator=;

        private:
            QObject* subscriber;
    };

    /** Pretty-prints plain text into HTML
     * This includes HTML escaping of <,>,",& and URLs linkification.
     */
    QString prettyPrint(const QString& plainText);

    /** Return a path to cache directory after making sure that it exists
     * The returned path has a trailing slash, clients don't need to append it.
     * \param dir path to cache directory relative to the standard cache path
     */
    QString cacheLocation(const QString& dirName);
}  // namespace QMatrixClient

