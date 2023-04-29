// SPDX-FileCopyrightText: 2018 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <optional>
#include <functional>

namespace Quotient {

template <typename T>
class Omittable;

constexpr auto none = std::nullopt;

//! \brief Lift an operation into dereferenceable types (Omittables or pointers)
//!
//! This is a more generic version of Omittable::then() that extends to
//! an arbitrary number of arguments of any type that is dereferenceable (unary
//! operator*() can be applied to it) and (explicitly or implicitly) convertible
//! to bool. This allows to streamline checking for nullptr/none before applying
//! the operation on the underlying types. \p fn is only invoked if all \p args
//! are "truthy" (i.e. <tt>(... && bool(args)) == true</tt>).
//! \param fn A callable that should accept the types stored inside
//!           Omittables/pointers passed in \p args
//! \return Always an Omittable: if \p fn returns another type, lift() wraps
//!         it in an Omittable; if \p fn returns an Omittable, that return value
//!         (or none) is returned as is.
template <typename FnT>
inline auto lift(FnT&& fn, auto&&... args)
{
    if constexpr (std::is_void_v<std::invoke_result_t<FnT, decltype(*args)...>>) {
        if ((... && bool(args)))
            std::invoke(std::forward<FnT>(fn), *args...);
    } else
        return (... && bool(args))
                   ? Omittable(std::invoke(std::forward<FnT>(fn), *args...))
                   : none;
}

/** `std::optional` with tweaks
 *
 * The tweaks are:
 * - streamlined assignment (operator=)/emplace()ment of values that can be
 *   used to implicitly construct the underlying type, including
 *   direct-list-initialisation, e.g.:
 *   \code
 *   struct S { int a; char b; }
 *   Omittable<S> o;
 *   o = { 1, 'a' }; // std::optional would require o = S { 1, 'a' }
 *   \endcode
 * - entirely deleted value(). The technical reason is that Xcode 10 doesn't
 *   have it; but besides that, value_or() or (after explicit checking)
 *   `operator*()`/`operator->()` are better alternatives within Quotient
 *   that doesn't practice throwing exceptions (as doesn't most of Qt).
 * - merge() - a soft version of operator= that only overwrites its first
 *   operand with the second one if the second one is not empty.
 * - then() and then_or() to streamline read-only interrogation in a "monadic"
 *   interface.
 */
template <typename T>
class Omittable : public std::optional<T> {
public:
    using base_type = std::optional<T>;
    using value_type = std::decay_t<T>;

    using std::optional<T>::optional;

    // Overload emplace() and operator=() to allow passing braced-init-lists
    // (the standard emplace() does direct-initialisation but
    // not direct-list-initialisation).
    using base_type::operator=;
    Omittable& operator=(const value_type& v)
    {
        base_type::operator=(v);
        return *this;
    }
    Omittable& operator=(value_type&& v)
    {
        base_type::operator=(std::move(v));
        return *this;
    }

    using base_type::emplace;
    T& emplace(const T& val) { return base_type::emplace(val); }
    T& emplace(T&& val) { return base_type::emplace(std::move(val)); }

    // Use value_or() or check (with operator! or has_value) before accessing
    // with operator-> or operator*
    // The technical reason is that Xcode 10 has incomplete std::optional
    // that has no value(); but using value() may also mean that you rely
    // on the optional throwing an exception (which is not an assumed practice
    // throughout Quotient) or that you spend unnecessary CPU cycles on
    // an extraneous has_value() check.
    auto& value() = delete;
    const auto& value() const = delete;

    //! Merge the value from another Omittable
    //! \return true if \p other is not omitted and the value of
    //!         the current Omittable was different (or omitted),
    //!         in other words, if the current Omittable has changed;
    //!         false otherwise
    template <typename T1>
    auto merge(const std::optional<T1>& other)
        -> std::enable_if_t<std::is_convertible_v<T1, T>, bool>
    {
        if (!other || (this->has_value() && **this == *other))
            return false;
        this->emplace(*other);
        return true;
    }

    // The below is inspired by the proposed std::optional monadic operations
    // (http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p0798r6.html).

    //! \brief Lift a callable into the Omittable
    //!
    //! 'Lifting', as used in functional programming, means here invoking
    //! a callable (e.g., a function) on the contents of the Omittable if it has
    //! any and wrapping the returned value (that may be of a different type T2)
    //! into a new Omittable\<T2>. If the current Omittable is empty,
    //! the invocation is skipped altogether and Omittable\<T2>{none} is
    //! returned instead.
    //! \note if \p fn already returns an Omittable (i.e., it is a 'functor',
    //!       in functional programming terms), then() will not wrap another
    //!       Omittable around but will just return what \p fn returns. The
    //!       same doesn't hold for the parameter: if \p fn accepts an Omittable
    //!       you have to wrap it in another Omittable before calling then().
    //! \return `none` if the current Omittable has `none`;
    //!         otherwise, the Omittable returned from a call to \p fn
    //! \tparam FnT a callable with \p T (or <tt>const T&</tt>)
    //!             returning Omittable<T2>, T2 is any supported type
    //! \sa then_or
    template <typename FnT>
    auto then(FnT&& fn) const
    {
        return lift(std::forward<FnT>(fn), *this);
    }

    //! \brief Lift a callable into the rvalue Omittable
    //!
    //! This is an rvalue overload for then().
    template <typename FnT>
    auto then(FnT&& fn)
    {
        return lift(std::forward<FnT>(fn), *this);
    }

    //! \brief Lift a callable into the const lvalue Omittable, with a fallback
    //!
    //! This effectively does the same what then() does, except that it returns
    //! a value of type returned by the callable (unwrapped from the Omittable),
    //! or the provided fallback value if the resulting (or the current - then
    //! the callable is not even touched) Omittable is empty. This is a typesafe
    //! version to apply an operation on an Omittable without having to deal
    //! with another Omittable afterwards.
    template <typename FnT, typename FallbackT>
    auto then_or(FnT&& fn, FallbackT&& fallback) const
    {
        return then(std::forward<FnT>(fn))
            .value_or(std::forward<FallbackT>(fallback));
    }

    //! \brief Lift a callable into the rvalue Omittable, with a fallback
    //!
    //! This is an overload for functions that accept rvalue
    template <typename FnT, typename FallbackT>
    auto then_or(FnT&& fn, FallbackT&& fallback)
    {
        return then(std::forward<FnT>(fn))
            .value_or(std::forward<FallbackT>(fallback));
    }
};

template <typename T>
Omittable(T&&) -> Omittable<T>;

//! \brief Merge the value from an optional
//! This is an adaptation of Omittable::merge() to the case when the value
//! on the left hand side is not an Omittable.
//! \return true if \p rhs is not omitted and the \p lhs value was different,
//!         in other words, if \p lhs has changed;
//!         false otherwise
template <typename T1, typename T2>
inline auto merge(T1& lhs, const std::optional<T2>& rhs)
    -> std::enable_if_t<std::is_assignable_v<T1&, const T2&>, bool>
{
    if (!rhs || lhs == *rhs)
        return false;
    lhs = *rhs;
    return true;
}

} // namespace Quotient
