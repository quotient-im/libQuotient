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

#include "util.h"

#include <QtCore/QRegularExpression>
#include <QtCore/QStandardPaths>
#include <QtCore/QDir>
#include <QtCore/QStringBuilder>

static const auto RegExpOptions =
    QRegularExpression::CaseInsensitiveOption
    | QRegularExpression::OptimizeOnFirstUsageOption
    | QRegularExpression::UseUnicodePropertiesOption;

// Converts all that looks like a URL into HTML links
static void linkifyUrls(QString& htmlEscapedText)
{
    // regexp is originally taken from Konsole (https://github.com/KDE/konsole)
    // full url:
    // protocolname:// or www. followed by anything other than whitespaces,
    // <, >, ' or ", and ends before whitespaces, <, >, ', ", ], !, ), :,
    // comma or dot
    // Note: outer parentheses are a part of C++ raw string delimiters, not of
    // the regex (see http://en.cppreference.com/w/cpp/language/string_literal).
    static const QRegularExpression FullUrlRegExp(QStringLiteral(
            R"(((www\.(?!\.)|[a-z][a-z0-9+.-]*://)(&(?![lg]t;)|[^&\s<>'"])+(&(?![lg]t;)|[^&!,.\s<>'"\]):])))"
        ), RegExpOptions);
    // email address:
    // [word chars, dots or dashes]@[word chars, dots or dashes].[word chars]
    static const QRegularExpression EmailAddressRegExp(QStringLiteral(
            R"((mailto:)?(\b(\w|\.|-)+@(\w|\.|-)+\.\w+\b))"
        ), RegExpOptions);

    // NOTE: htmlEscapedText is already HTML-escaped! No literal <,>,&

    htmlEscapedText.replace(EmailAddressRegExp,
                 QStringLiteral(R"(<a href="mailto:\2">\1\2</a>)"));
    htmlEscapedText.replace(FullUrlRegExp,
                 QStringLiteral(R"(<a href="\1">\1</a>)"));
}

QString QMatrixClient::prettyPrint(const QString& plainText)
{
    auto pt = QStringLiteral("<span style='white-space:pre-wrap'>") +
            plainText.toHtmlEscaped() + QStringLiteral("</span>");
    pt.replace('\n', QStringLiteral("<br/>"));

    linkifyUrls(pt);
    return pt;
}

QString QMatrixClient::cacheLocation(const QString& dirName)
{
    const QString cachePath =
        QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
            % '/' % dirName % '/';
    QDir dir;
    if (!dir.exists(cachePath))
        dir.mkpath(cachePath);
    return cachePath;
}

// Tests for function_traits<>

#ifdef Q_CC_CLANG
#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCSimplifyInspection"
#endif
using namespace QMatrixClient;

int f();
static_assert(std::is_same<fn_return_t<decltype(f)>, int>::value,
              "Test fn_return_t<>");

void f1(int);
static_assert(function_traits<decltype(f1)>::arg_number == 1,
              "Test fn_arg_number");

void f2(int, QString);
static_assert(std::is_same<fn_arg_t<decltype(f2), 1>, QString>::value,
              "Test fn_arg_t<>");

struct S { int mf(); };
static_assert(is_callable_v<decltype(&S::mf)>, "Test member function");
static_assert(returns<int, decltype(&S::mf)>(), "Test returns<> with member function");

struct Fo { int operator()(); };
static_assert(is_callable_v<Fo>, "Test is_callable<> with function object");
static_assert(function_traits<Fo>::arg_number == 0, "Test function object");
static_assert(std::is_same<fn_return_t<Fo>, int>::value,
              "Test return type of function object");

struct Fo1 { void operator()(int); };
static_assert(function_traits<Fo1>::arg_number == 1, "Test function object 1");
static_assert(is_callable_v<Fo1>, "Test is_callable<> with function object 1");
static_assert(std::is_same<fn_arg_t<Fo1>, int>(),
              "Test fn_arg_t defaulting to first argument");

#if (!defined(_MSC_VER) || _MSC_VER >= 1910)
static auto l = [] { return 1; };
static_assert(is_callable_v<decltype(l)>, "Test is_callable_v<> with lambda");
static_assert(std::is_same<fn_return_t<decltype(l)>, int>::value,
              "Test fn_return_t<> with lambda");
#endif

template <typename T>
struct fn_object
{
    static int smf(double) { return 0; }
};
template <>
struct fn_object<QString>
{
    void operator()(QString);
};
static_assert(is_callable_v<fn_object<QString>>, "Test function object");
static_assert(returns<void, fn_object<QString>>(),
              "Test returns<> with function object");
static_assert(!is_callable_v<fn_object<int>>, "Test non-function object");
// FIXME: These two don't work
//static_assert(is_callable_v<decltype(&fn_object<int>::smf)>,
//              "Test static member function");
//static_assert(returns<int, decltype(&fn_object<int>::smf)>(),
//              "Test returns<> with static member function");

template <typename T>
QString ft(T&&);
static_assert(std::is_same<fn_arg_t<decltype(ft<QString>)>, QString&&>(),
              "Test function templates");

#ifdef Q_CC_CLANG
#pragma clang diagnostic pop
#endif
