// SPDX-FileCopyrightText: 2016 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-FileCopyrightText: 2019 Alexey Andreyev <aa13q@ya.ru>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "quotient_export.h"

#include <QtCore/QLatin1String>
#include <QtCore/QHashFunctions>

#include <memory>
#include <unordered_map>

#ifndef Q_DISABLE_MOVE
// Q_DISABLE_MOVE was introduced in Q_VERSION_CHECK(5,13,0)
#    define Q_DISABLE_MOVE(_ClassName)             \
        _ClassName(_ClassName&&) Q_DECL_EQ_DELETE; \
        _ClassName& operator=(_ClassName&&) Q_DECL_EQ_DELETE;
#endif

#ifndef Q_DISABLE_COPY_MOVE
#define Q_DISABLE_COPY_MOVE(Class) \
    Q_DISABLE_COPY(Class) \
    Q_DISABLE_MOVE(Class)
#endif

#define DISABLE_MOVE(_ClassName) \
static_assert(false, "Use Q_DISABLE_MOVE instead; Quotient enables it across all used versions of Qt");

#ifndef QT_IGNORE_DEPRECATIONS
// QT_IGNORE_DEPRECATIONS was introduced in Q_VERSION_CHECK(5,15,0)
#    define QT_IGNORE_DEPRECATIONS(statement) \
        QT_WARNING_PUSH                       \
        QT_WARNING_DISABLE_DEPRECATED         \
        statement                             \
        QT_WARNING_POP
#endif

#if __cpp_conditional_explicit >= 201806L
#define QUO_IMPLICIT explicit(false)
#else
#define QUO_IMPLICIT
#endif

#define DECL_DEPRECATED_ENUMERATOR(Deprecated, Recommended) \
    Deprecated Q_DECL_ENUMERATOR_DEPRECATED_X("Use " #Recommended) = Recommended

/// \brief Copy an object with slicing
///
/// Unintended slicing is bad, which why there's a C++ Core Guideline that
/// basically says "don't slice, or if you do, make it explicit". Sonar and
/// clang-tidy have warnings matching this guideline; unfortunately, those
/// warnings trigger even when you have a dedicated method (as the guideline
/// recommends) that makes a slicing copy.
///
/// This macro is meant for cases when slicing is intended: the static cast
/// silences the static analysis warning, and the macro appearance itself makes
/// it very clear that slicing is wanted here. It is made as a macro
/// (not as a function template) to support the case of private inheritance
/// in which a function template would not be able to cast to the private base
/// (see Uri::toUrl() for an example of just that situation).
#define SLICE(Object, ToType) ToType{static_cast<const ToType&>(Object)}

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

constexpr auto operator"" _ls(const char* s, std::size_t size)
{
    return QLatin1String(s, int(size));
}

/** An abstraction over a pair of iterators
 * This is a very basic range type over a container with iterators that
 * are at least ForwardIterators. Inspired by Ranges TS.
 */
template <typename ArrayT>
class Range {
    // Looking forward to C++20 ranges
    using iterator = typename ArrayT::iterator;
    using const_iterator = typename ArrayT::const_iterator;
    using size_type = typename ArrayT::size_type;

public:
    constexpr Range(ArrayT& arr) : from(std::begin(arr)), to(std::end(arr)) {}
    constexpr Range(iterator from, iterator to) : from(from), to(to) {}

    constexpr size_type size() const
    {
        Q_ASSERT(std::distance(from, to) >= 0);
        return size_type(std::distance(from, to));
    }
    constexpr bool empty() const { return from == to; }
    constexpr const_iterator begin() const { return from; }
    constexpr const_iterator end() const { return to; }
    constexpr iterator begin() { return from; }
    constexpr iterator end() { return to; }

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

//! \brief An owning implementation pointer
//!
//! This is basically std::unique_ptr<> to hold your pimpl's but without having
//! to define default constructors/operator=() out of line.
//! Thanks to https://oliora.github.io/2015/12/29/pimpl-and-rule-of-zero.html
//! for inspiration
template <typename ImplType>
using ImplPtr = std::unique_ptr<ImplType, void (*)(ImplType*)>;

// Why this works (see also the link above): because this defers the moment
// of requiring sizeof of ImplType to the place where makeImpl is invoked
// (which is located, necessarily, in the .cpp file after ImplType definition).
// The stock unique_ptr deleter (std::default_delete) normally needs sizeof
// at the same spot - as long as you defer definition of the owning type
// constructors and operator='s to the .cpp file as well. Which means you
// have to explicitly declare and define them (even if with = default),
// formally breaking the rule of zero; informally, just adding boilerplate code.
// The custom deleter itself is instantiated at makeImpl invocation - there's
// no way earlier to even know how ImplType will be deleted and whether that
// will need sizeof(ImplType) earlier. In theory it's a tad slower because
// the deleter is called by the pointer; however, the difference will not
// be noticeable (if exist at all) for any class with non-trivial contents.

//! \brief make_unique for ImplPtr
//!
//! Since std::make_unique is not compatible with ImplPtr, this should be used
//! in constructors of frontend classes to create implementation instances.
template <typename ImplType, typename DeleterType = void (*)(ImplType*),
          typename... ArgTs>
inline ImplPtr<ImplType> makeImpl(ArgTs&&... args)
{
    return ImplPtr<ImplType> { new ImplType(std::forward<ArgTs>(args)...),
                               [](ImplType* impl) { delete impl; } };
}

template <typename ImplType>
constexpr ImplPtr<ImplType> ZeroImpl()
{
    return { nullptr, [](ImplType*) { /* nullptr doesn't need deletion */ } };
}

//! \brief Multiplex several functors in one
//!
//! This is a well-known trick to wrap several lambdas into a single functor
//! class that can be passed to std::visit.
//! \sa  https://en.cppreference.com/w/cpp/utility/variant/visit
template <typename... FunctorTs>
struct Overloads : FunctorTs... {
    using FunctorTs::operator()...;
};

template <typename... FunctorTs>
Overloads(FunctorTs&&...) -> Overloads<FunctorTs...>;

/** Convert what looks like a URL or a Matrix ID to an HTML hyperlink */
QUOTIENT_API void linkifyUrls(QString& htmlEscapedText);

/** Sanitize the text before showing in HTML
 *
 * This does toHtmlEscaped() and removes Unicode BiDi marks.
 */
QUOTIENT_API QString sanitized(const QString& plainText);

/** Pretty-print plain text into HTML
 *
 * This includes HTML escaping of <,>,",& and calling linkifyUrls()
 */
QUOTIENT_API QString prettyPrint(const QString& plainText);

/** Return a path to cache directory after making sure that it exists
 *
 * The returned path has a trailing slash, clients don't need to append it.
 * \param dir path to cache directory relative to the standard cache path
 */
QUOTIENT_API QString cacheLocation(const QString& dirName);

/** Hue color component of based of the hash of the string.
 *
 * The implementation is based on XEP-0392:
 * https://xmpp.org/extensions/xep-0392.html
 * Naming and range are the same as QColor's hueF method:
 * https://doc.qt.io/qt-5/qcolor.html#integer-vs-floating-point-precision
 */
QUOTIENT_API qreal stringToHueF(const QString& s);

/** Extract the serverpart from MXID */
QUOTIENT_API QString serverPart(const QString& mxId);

QUOTIENT_API QString versionString();
QUOTIENT_API int majorVersion();
QUOTIENT_API int minorVersion();
QUOTIENT_API int patchVersion();
QUOTIENT_API bool encryptionSupported();
} // namespace Quotient
