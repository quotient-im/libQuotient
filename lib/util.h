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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

#pragma once

#include <QtCore/QLatin1String>
#include <QtCore/QHashFunctions>

#include <functional>
#include <memory>
#include <unordered_map>

// Along the lines of Q_DISABLE_COPY - the upstream version comes in Qt 5.13
#define DISABLE_MOVE(_ClassName)               \
    _ClassName(_ClassName&&) Q_DECL_EQ_DELETE; \
    _ClassName& operator=(_ClassName&&) Q_DECL_EQ_DELETE;

namespace Quotient {
/// An equivalent of std::hash for QTypes to enable std::unordered_map<QType, ...>
template <typename T>
struct HashQ {
    size_t operator()(const T& s) const Q_DECL_NOEXCEPT
    {
        return qHash(s, uint(qGlobalQHashSeed()));
    }
};
/// A wrapper around std::unordered_map compatible with types that have qHash
template <typename KeyT, typename ValT>
using UnorderedMap = std::unordered_map<KeyT, ValT, HashQ<KeyT>>;

struct NoneTag {};
constexpr NoneTag none {};

/** A crude substitute for `optional` while we're not C++17
 *
 * Only works with default-constructible types.
 */
template <typename T>
class Omittable {
    static_assert(!std::is_reference<T>::value,
                  "You cannot make an Omittable<> with a reference type");

public:
    using value_type = std::decay_t<T>;

    explicit Omittable() : Omittable(none) {}
    Omittable(NoneTag) : _value(value_type()), _omitted(true) {}
    Omittable(const value_type& val) : _value(val) {}
    Omittable(value_type&& val) : _value(std::move(val)) {}
    Omittable<T>& operator=(const value_type& val)
    {
        _value = val;
        _omitted = false;
        return *this;
    }
    Omittable<T>& operator=(value_type&& val)
    {
        // For some reason GCC complains about -Wmaybe-uninitialized
        // in the context of using Omittable<bool> with converters.h;
        // though the logic looks very much benign (GCC bug???)
        _value = std::move(val);
        _omitted = false;
        return *this;
    }

    bool operator==(const value_type& rhs) const
    {
        return !omitted() && value() == rhs;
    }
    friend bool operator==(const value_type& lhs,
                           const Omittable<value_type>& rhs)
    {
        return rhs == lhs;
    }
    bool operator!=(const value_type& rhs) const { return !operator==(rhs); }
    friend bool operator!=(const value_type& lhs,
                           const Omittable<value_type>& rhs)
    {
        return !(rhs == lhs);
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
    /// Merge the value from another Omittable
    /// \return true if \p other is not omitted and the value of
    ///         the current Omittable was different (or omitted);
    ///         in other words, if the current Omittable has changed;
    ///         false otherwise
    template <typename T1>
    auto merge(const Omittable<T1>& other)
        -> std::enable_if_t<std::is_convertible<T1, T>::value, bool>
    {
        if (other.omitted() || (!_omitted && _value == other.value()))
            return false;
        _omitted = false;
        _value = other.value();
        return true;
    }
    value_type&& release()
    {
        _omitted = true;
        return std::move(_value);
    }

    const value_type* operator->() const& { return &value(); }
    value_type* operator->() & { return &editValue(); }
    const value_type& operator*() const& { return value(); }
    value_type& operator*() & { return editValue(); }

private:
    T _value;
    bool _omitted = false;
};

namespace _impl {
    template <typename AlwaysVoid, typename>
    struct fn_traits;
}

/// Determine traits of an arbitrary function/lambda/functor
/*!
 * Doesn't work with generic lambdas and function objects that have
 * operator() overloaded.
 * \sa
 * https://stackoverflow.com/questions/7943525/is-it-possible-to-figure-out-the-parameter-type-and-return-type-of-a-lambda#7943765
 */
template <typename T>
struct function_traits : public _impl::fn_traits<void, T> {};

// Specialisation for a function
template <typename ReturnT, typename... ArgTs>
struct function_traits<ReturnT(ArgTs...)> {
    static constexpr auto is_callable = true;
    using return_type = ReturnT;
    using arg_types = std::tuple<ArgTs...>;
    using function_type = std::function<ReturnT(ArgTs...)>;
    static constexpr auto arg_number = std::tuple_size<arg_types>::value;
};

namespace _impl {
    template <typename AlwaysVoid, typename T>
    struct fn_traits {
        static constexpr auto is_callable = false;
    };

    template <typename T>
    struct fn_traits<decltype(void(&T::operator())), T>
        : public fn_traits<void, decltype(&T::operator())> {
    }; // A generic function object that has (non-overloaded) operator()

    // Specialisation for a member function
    template <typename ReturnT, typename ClassT, typename... ArgTs>
    struct fn_traits<void, ReturnT (ClassT::*)(ArgTs...)>
        : function_traits<ReturnT(ArgTs...)> {};

    // Specialisation for a const member function
    template <typename ReturnT, typename ClassT, typename... ArgTs>
    struct fn_traits<void, ReturnT (ClassT::*)(ArgTs...) const>
        : function_traits<ReturnT(ArgTs...)> {};
} // namespace _impl

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
class Range {
    // Looking forward for Ranges TS to produce something (in C++23?..)
    using iterator = typename ArrayT::iterator;
    using const_iterator = typename ArrayT::const_iterator;
    using size_type = typename ArrayT::size_type;

public:
    Range(ArrayT& arr) : from(std::begin(arr)), to(std::end(arr)) {}
    Range(iterator from, iterator to) : from(from), to(to) {}

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
template <typename InputIt, typename ForwardIt, typename Pred>
inline std::pair<InputIt, ForwardIt> findFirstOf(InputIt first, InputIt last,
                                                 ForwardIt sFirst,
                                                 ForwardIt sLast, Pred pred)
{
    for (; first != last; ++first)
        for (auto it = sFirst; it != sLast; ++it)
            if (pred(*first, *it))
                return std::make_pair(first, it);

    return std::make_pair(last, sLast);
}

/** Convert what looks like a URL or a Matrix ID to an HTML hyperlink */
void linkifyUrls(QString& htmlEscapedText);

/** Sanitize the text before showing in HTML
 *
 * This does toHtmlEscaped() and removes Unicode BiDi marks.
 */
QString sanitized(const QString& plainText);

/** Pretty-print plain text into HTML
 *
 * This includes HTML escaping of <,>,",& and calling linkifyUrls()
 */
QString prettyPrint(const QString& plainText);

/** Return a path to cache directory after making sure that it exists
 *
 * The returned path has a trailing slash, clients don't need to append it.
 * \param dir path to cache directory relative to the standard cache path
 */
QString cacheLocation(const QString& dirName);

/** Hue color component of based of the hash of the string.
 *
 * The implementation is based on XEP-0392:
 * https://xmpp.org/extensions/xep-0392.html
 * Naming and range are the same as QColor's hueF method:
 * https://doc.qt.io/qt-5/qcolor.html#integer-vs-floating-point-precision
 */
qreal stringToHueF(const QString& s);

/** Extract the serverpart from MXID */
QString serverPart(const QString& mxId);
} // namespace Quotient
/// \deprecated Use namespace Quotient instead
namespace QMatrixClient = Quotient;
