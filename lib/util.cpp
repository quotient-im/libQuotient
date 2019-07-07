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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include "util.h"

#include <QtCore/QCryptographicHash>
#include <QtCore/QDataStream>
#include <QtCore/QDir>
#include <QtCore/QRegularExpression>
#include <QtCore/QStandardPaths>
#include <QtCore/QStringBuilder>
#include <QtCore/QtEndian>

static const auto RegExpOptions =
    QRegularExpression::CaseInsensitiveOption
    | QRegularExpression::OptimizeOnFirstUsageOption
    | QRegularExpression::UseUnicodePropertiesOption;

// Converts all that looks like a URL into HTML links
void Quotient::linkifyUrls(QString& htmlEscapedText)
{
    // Note: outer parentheses are a part of C++ raw string delimiters, not of
    // the regex (see http://en.cppreference.com/w/cpp/language/string_literal).
    // Note2: the next-outer parentheses are \N in the replacement.

    // generic url:
    // regexp is originally taken from Konsole (https://github.com/KDE/konsole)
    // protocolname:// or www. followed by anything other than whitespaces,
    // <, >, ' or ", and ends before whitespaces, <, >, ', ", ], !, ), :,
    // comma or dot
    static const QRegularExpression FullUrlRegExp(
        QStringLiteral(
            R"(\b((www\.(?!\.)(?!(\w|\.|-)+@)|(https?|ftp|magnet|matrix):(//)?)(&(?![lg]t;)|[^&\s<>'"])+(&(?![lg]t;)|[^&!,.\s<>'"\]):])))"),
        RegExpOptions);
    // email address:
    // [word chars, dots or dashes]@[word chars, dots or dashes].[word chars]
    static const QRegularExpression EmailAddressRegExp(
        QStringLiteral(R"(\b(mailto:)?((\w|\.|-)+@(\w|\.|-)+\.\w+\b))"),
        RegExpOptions);
    // An interim liberal implementation of
    // https://matrix.org/docs/spec/appendices.html#identifier-grammar
    static const QRegularExpression MxIdRegExp(
        QStringLiteral(
            R"((^|[^<>/])([!#@][-a-z0-9_=/.]{1,252}:(?:\w|\.|-)+\.\w+(?::\d{1,5})?))"),
        RegExpOptions);

    // NOTE: htmlEscapedText is already HTML-escaped! No literal <,>,&,"

    htmlEscapedText.replace(EmailAddressRegExp,
                            QStringLiteral(R"(<a href="mailto:\2">\1\2</a>)"));
    htmlEscapedText.replace(FullUrlRegExp,
                            QStringLiteral(R"(<a href="\1">\1</a>)"));
    htmlEscapedText.replace(
        MxIdRegExp,
        QStringLiteral(R"(\1<a href="https://matrix.to/#/\2">\2</a>)"));
}

QString Quotient::sanitized(const QString& plainText)
{
    auto text = plainText;
    text.remove(QChar(0x202e)); // RLO
    text.remove(QChar(0x202d)); // LRO
    text.remove(QChar(0xfffc)); // Object replacement character
    return text;
}

QString Quotient::prettyPrint(const QString& plainText)
{
    auto pt = plainText.toHtmlEscaped();
    linkifyUrls(pt);
    pt.replace('\n', QStringLiteral("<br/>"));
    return QStringLiteral("<span style='white-space:pre-wrap'>") + pt
           + QStringLiteral("</span>");
}

QString Quotient::cacheLocation(const QString& dirName)
{
    const QString cachePath =
        QStandardPaths::writableLocation(QStandardPaths::CacheLocation) % '/'
        % dirName % '/';
    QDir dir;
    if (!dir.exists(cachePath))
        dir.mkpath(cachePath);
    return cachePath;
}

qreal Quotient::stringToHueF(const QString& string)
{
    Q_ASSERT(!string.isEmpty());
    QByteArray hash = QCryptographicHash::hash(string.toUtf8(),
                                               QCryptographicHash::Sha1);
    QDataStream dataStream(qToLittleEndian(hash).left(2));
    dataStream.setByteOrder(QDataStream::LittleEndian);
    quint16 hashValue;
    dataStream >> hashValue;
    const auto hueF = qreal(hashValue) / std::numeric_limits<quint16>::max();
    Q_ASSERT((0 <= hueF) && (hueF <= 1));
    return hueF;
}

static const auto ServerPartRegEx = QStringLiteral(
    "(\\[[^]]+\\]|[^:@]+)" // Either IPv6 address or hostname/IPv4 address
    "(?::(\\d{1,5}))?" // Optional port
);

QString Quotient::serverPart(const QString& mxId)
{
    static QString re = "^[@!#$+].+?:(" // Localpart and colon
                        % ServerPartRegEx % ")$";
    static QRegularExpression parser(
        re,
        QRegularExpression::UseUnicodePropertiesOption); // Because Asian
                                                         // digits
    return parser.match(mxId).captured(1);
}

// Tests for function_traits<>

#ifdef Q_CC_CLANG
#    pragma clang diagnostic push
#    pragma ide diagnostic ignored "OCSimplifyInspection"
#endif
using namespace Quotient;

int f();
static_assert(std::is_same<fn_return_t<decltype(f)>, int>::value,
              "Test fn_return_t<>");

void f1(int);
static_assert(function_traits<decltype(f1)>::arg_number == 1,
              "Test fn_arg_number");

void f2(int, QString);
static_assert(std::is_same<fn_arg_t<decltype(f2), 1>, QString>::value,
              "Test fn_arg_t<>");

struct S {
    int mf();
};
static_assert(is_callable_v<decltype(&S::mf)>, "Test member function");
static_assert(returns<int, decltype(&S::mf)>(),
              "Test returns<> with member function");

struct Fo {
    int operator()();
};
static_assert(is_callable_v<Fo>, "Test is_callable<> with function object");
static_assert(function_traits<Fo>::arg_number == 0, "Test function object");
static_assert(std::is_same<fn_return_t<Fo>, int>::value,
              "Test return type of function object");

struct Fo1 {
    void operator()(int);
};
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
struct fn_object {
    static int smf(double) { return 0; }
};
template <>
struct fn_object<QString> {
    void operator()(QString);
};
static_assert(is_callable_v<fn_object<QString>>, "Test function object");
static_assert(returns<void, fn_object<QString>>(),
              "Test returns<> with function object");
static_assert(!is_callable_v<fn_object<int>>, "Test non-function object");
// FIXME: These two don't work
// static_assert(is_callable_v<decltype(&fn_object<int>::smf)>,
//              "Test static member function");
// static_assert(returns<int, decltype(&fn_object<int>::smf)>(),
//              "Test returns<> with static member function");

template <typename T>
QString ft(T&&)
{
    return {};
}
static_assert(std::is_same<fn_arg_t<decltype(ft<QString>)>, QString&&>(),
              "Test function templates");

#ifdef Q_CC_CLANG
#    pragma clang diagnostic pop
#endif
