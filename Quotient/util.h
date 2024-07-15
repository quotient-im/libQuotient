// SPDX-FileCopyrightText: 2016 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-FileCopyrightText: 2019 Alexey Andreyev <aa13q@ya.ru>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "quotient_export.h"

#include <QtCore/QDebug>
#include <QtCore/QElapsedTimer>
#include <QtCore/QHashFunctions>
#include <QtCore/QLatin1String>
#include <QtCore/QUrl>

#include <memory>
#include <optional>
#include <source_location>
#include <unordered_map>

#define QUO_IMPLICIT explicit(false)

#define DECL_DEPRECATED_ENUMERATOR(Deprecated, Recommended) \
    Deprecated Q_DECL_ENUMERATOR_DEPRECATED_X("Use " #Recommended) = Recommended

/// \brief Copy an object with slicing
///
/// Unintended slicing is bad, which is why there's a C++ Core Guideline that
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

inline const char* printable(std::convertible_to<QString> auto s)
{
    return QString(s).toUtf8().constData();
}
inline const char* printable(const char* s) { return s; }
inline const char* printable(QUtf8StringView s) { return s.data(); }

inline bool alarmX(bool alarmCondition, const auto& msg,
                   [[maybe_unused]] std::source_location loc = std::source_location::current())
{
    if (alarmCondition) [[unlikely]] {
        Q_ASSERT_X(false, loc.function_name(), printable(msg));
        qCritical() << msg;
    }
    return alarmCondition;
}

//! \brief A negative assertion facility that can be put in an if statement
//!
//! Unlike most other assertion functions or macros, this doesn't collapse to no-op in Release
//! builds; rather, it sends a critical level message to the log and returns true so that you could
//! immediately return or do some other damage control for Release builds. Also unlike most other
//! assertion functions, \p AlarmCondition is the condition for failure, not for health; in other
//! words, \p Message is sent to logs (and, in Debug configuration, the assertion fails)
//! if \p AlarmCondition holds, not the other way around.
//!
//! This macro is a trivial wrapper around alarmX(), provided for API uniformity with ALARM()
#define ALARM_X(AlarmCondition, Message) alarmX(AlarmCondition, Message)

#define ALARM(AlarmCondition) alarmX(AlarmCondition, "Alarm: " #AlarmCondition)

#if Quotient_VERSION_MAJOR == 0 && Quotient_VERSION_MINOR < 10
/// This is only to make UnorderedMap alias work until we get rid of it
template <typename T>
struct HashQ {
    size_t operator()(const T& s) const Q_DECL_NOEXCEPT
    {
        return qHash(s, uint(QHashSeed::globalSeed()));
    }
};
/// A wrapper around std::unordered_map compatible with types that have qHash
template <typename KeyT, typename ValT>
using UnorderedMap
    [[deprecated("Use std::unordered_map directly")]] = std::unordered_map<KeyT, ValT, HashQ<KeyT>>;

constexpr auto operator"" _ls(const char* s, std::size_t size)
{
    return QLatin1String(s, int(size));
}

template <typename ArrayT>
class [[deprecated("Use std::ranges::subrange instead")]] Range {
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

namespace _impl {
    template <typename T>
    concept Holds_NonConst_LValue_Ref = requires {
        std::is_lvalue_reference_v<T>;
        !std::is_const_v<std::remove_reference<T>>;
    };
}

//! \brief An adaptor for Qt (hash-)maps to make them iterable in STL style
//!
//! QMap/QHash container iterators returned by begin() and end() dereference
//! to values, unlike STL where similar iterators dereference to key-value
//! pairs. It is a problem in range-for if you want to also access map keys.
//! This adaptor allows to use range-for syntax with access to both keys and
//! values in QMap/QHash containers. Just use
//! `for (auto&& [key, value] : asKeyValueRange(myMap)` instead of
//! `for (auto&& value : myMap)`.
//! \note When an rvalue is passed as the constructor argument, asKeyValueRange
//!       shallow-copies the map object to ensure its lifetime is maintained
//!       throughout the loop; with lvalues, no copying occurs, assuming that
//!       the map object outlives the range-for loop
template <typename T>
class [[deprecated(
    "Use the member function with the same name of the respective container")]] asKeyValueRange {
public:
    explicit asKeyValueRange(T data)
        : m_data { data }
    {}

    auto begin() requires _impl::Holds_NonConst_LValue_Ref<T>
    {
        return m_data.keyValueBegin();
    }
    auto end() requires _impl::Holds_NonConst_LValue_Ref<T>
    {
        return m_data.keyValueEnd();
    }
    auto begin() const { return m_data.keyValueBegin(); }
    auto end() const { return m_data.keyValueEnd(); }

private:
    T m_data;
};

// GCC complains about a deprecation even on deduction hints, so here's to suppress noise
QT_IGNORE_DEPRECATIONS(
template <typename U>
asKeyValueRange(U&) -> asKeyValueRange<U&>;

template <typename U>
asKeyValueRange(U&&) -> asKeyValueRange<U>;
)
#endif

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
                return { first, it };

    return { last, sLast };
}

//! \brief An owning implementation pointer
//!
//! This is basically std::unique_ptr<> to hold your pimpl's but without having
//! to define default constructors/operator=() out of line.
//! Thanks to https://oliora.github.io/2015/12/29/pimpl-and-rule-of-zero.html
//! for inspiration
template <typename ImplType, typename TypeToDelete = ImplType>
using ImplPtr = std::unique_ptr<ImplType, void (*)(TypeToDelete*)>;

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
template <typename ImplType, typename TypeToDelete = ImplType, typename... ArgTs>
inline ImplPtr<ImplType, TypeToDelete> makeImpl(ArgTs&&... args)
{
    return ImplPtr<ImplType, TypeToDelete> {
        new ImplType{std::forward<ArgTs>(args)...},
        [](TypeToDelete* impl) { delete impl; }
    };
}

template <typename ImplType, typename TypeToDelete = ImplType>
inline ImplPtr<ImplType, TypeToDelete> acquireImpl(ImplType* from)
{
    return ImplPtr<ImplType, TypeToDelete> { from, [](TypeToDelete* impl) {
                                                delete impl;
                                            } };
}

template <typename ImplType, typename TypeToDelete = ImplType>
constexpr ImplPtr<ImplType, TypeToDelete> ZeroImpl()
{
    return { nullptr, [](TypeToDelete*) { /* nullptr doesn't need deletion */ } };
}

template <typename T>
struct CStructDeleter {
    size_t (*destructor)(T*);

    void operator()(T* toDelete)
    {
        destructor(toDelete);
        delete[] reinterpret_cast<std::byte*>(toDelete);
    }
};

//! \brief An owning pointer to a C structure
//!
//! This is intented to ease lifecycle management of Olm structures.
//! \sa makeCStruct
template <typename T>
using CStructPtr = std::unique_ptr<T, CStructDeleter<T>>;

//! \brief Create a C structure with pre-programmed deletion logic
//!
//! This facility function creates a CStructPtr that owns the pointer returned
//! by \p constructor. The memory passed to \p constructor is allocated
//! as an array of bytes; the size of that array is determined by calling
//! \p sizeFn. Finally, since the returned pointer is owning, it also stores
//! the corresponding CStructDeleter instance; when called at destruction of
//! the owning pointer, this deleter first calls \p destructor passing the
//! original C pointer returned by \p constructor; and then deletes the
//! allocated array of bytes.
template <typename T>
inline auto makeCStruct(T* (*constructor)(void*), size_t (*sizeFn)(),
                        auto destructor)
{
    return CStructPtr<T>{ constructor(new std::byte[sizeFn()]), { destructor } };
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
 * \param dirName path to cache directory relative to the standard cache path
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

// QDebug manipulators

//! \brief QDebug manipulator to setup the stream for JSON output
//!
//! Originally made to encapsulate the change in QDebug behavior in Qt 5.4
//! and the respective addition of QDebug::noquote().
//! Together with the operator<<() helper, the proposed usage is
//! (similar to std:: I/O manipulators):
//! `qCDebug(MAIN) << formatJson << json_object; // (QJsonObject etc.)`
inline QDebug formatJson(QDebug dbg) { return dbg.noquote(); }

//! Suppress full qualification of enums/QFlags when logging
inline QDebug terse(QDebug dbg)
{
    return dbg.verbosity(QDebug::MinimumVerbosity);
}

constexpr qint64 ProfilerMinNsecs =
#ifdef PROFILER_LOG_USECS
    PROFILER_LOG_USECS
#else
    200
#endif
    * 1000;
} // namespace Quotient

//! \brief A helper operator for QDebug manipulators, e.g. formatJson
//!
//! \param dbg to output the json to
//! \param manipFn a QDebug manipulator
//! \return a copy of dbg that has its mode altered by manipFn
inline QDebug operator<<(QDebug dbg, std::invocable<QDebug> auto manipFn)
{
    return std::invoke(manipFn, dbg);
}

inline QDebug operator<<(QDebug dbg, QElapsedTimer et)
{
    // NOLINTNEXTLINE(bugprone-integer-division)
    dbg << static_cast<double>(et.nsecsElapsed() / 1000) / 1000
                 << "ms"; // Show in ms with 3 decimal digits precision
    return dbg;
}

//! \brief Lift an operation into dereferenceable types (std::optional or pointers)
//!
//! This is a more generic version of std::optional::and_then() that accepts an arbitrary number of
//! arguments of any type that is dereferenceable (i.e. unary operator*() can be applied to it) and
//! (explicitly or implicitly) convertible to bool. This allows to streamline checking for
//! nullptr/nullopt before applying the operation on the underlying types. \p fn is only invoked if
//! all \p args are "truthy" (i.e. <tt>(... && bool(args)) == true</tt>).
//! \param fn A callable that should accept the types stored inside optionals/pointers passed in
//!           \p args (NOT optionals/pointers themselves; they are unwrapped)
//! \return Always an optional: if \p fn returns another type, lift() wraps it in std::optional;
//!         if \p fn returns std::optional, that return value (or std::nullopt) is returned as is.
template <typename FnT>
inline auto lift(FnT&& fn, auto&&... args)
{
    if constexpr (std::is_void_v<std::invoke_result_t<FnT, decltype(*args)...>>) {
        if ((... && bool(args)))
            std::invoke(std::forward<FnT>(fn), *args...);
    } else
        return (... && bool(args))
                   ? std::optional(std::invoke(std::forward<FnT>(fn), *args...))
                   : std::nullopt;
}

//! \brief Merge the value from an optional
//!
//! Assigns the value stored at \p rhs to \p lhs if, and only if, \p rhs is not omitted and
//! `lhs != *rhs`. \p lhs can be either an optional or an ordinary variable.
//! \return `true` if \p rhs is not omitted and the \p lhs value was different, in other words,
//!         if \p lhs has changed; `false` otherwise
template <typename T1, typename T2>
    requires std::is_assignable_v<T1&, const T2&>
constexpr inline bool merge(T1& lhs, const std::optional<T2>& rhs)
{
    if (!rhs || lhs == *rhs)
        return false;
    lhs = *rhs;
    return true;
}

//! \brief Merge structure-like types
//!
//! Merges fields in \p lhs from counterparts in \p rhs. The list of fields to merge is passed
//! in additional parameters (\p fields). E.g.:
//! \codeline mergeStruct(struct1, struct2, &Struct::field1, &Struct::field2, &Struct::field3)
//! \return the number of fields in \p lhs that were changed
template <typename StructT>
constexpr inline size_t mergeStruct(StructT& lhs, const StructT& rhs, const auto... fields)
{
    return ((... + static_cast<size_t>(merge(lhs.*fields, rhs.*fields))));
}

// These are meant to eventually become separate classes derived from QString (or perhaps
// QByteArray?), with their own construction and validation logic; for now they are just aliases
// for QString to make numerous IDs at least semantically different in the code.

using UserId = QString;
using RoomId = QString;
using EventId = QString;

struct HomeserverData {
    QUrl baseUrl;
    QStringList supportedSpecVersions;
};

