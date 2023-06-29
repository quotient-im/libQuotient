// SPDX-FileCopyrightText: 2018 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-FileCopyrightText: 2019 Alexey Andreyev <aa13q@ya.ru>
// SPDX-License-Identifier: LGPL-2.1-or-later

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
            R"(\b((www\.(?!\.)(?!(\w|\.|-)+@)|(https?|ftp):(//)?\w|(magnet|matrix):)(&(?![lg]t;)|[^&\s<>'"])+(&(?![lg]t;)|[^&!,.\s<>'"\]):])))"),
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
            R"((^|[][[:space:](){}`'";])([!#@][-a-z0-9_=#/.]{1,252}:\w(?:\w|\.|-)*\.\w+(?::\d{1,5})?))"),
        RegExpOptions);
    Q_ASSERT(FullUrlRegExp.isValid() && EmailAddressRegExp.isValid()
             && MxIdRegExp.isValid());

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
    pt.replace(u'\n', QStringLiteral("<br/>"));
    return QStringLiteral("<span style='white-space:pre-wrap'>") + pt
           + QStringLiteral("</span>");
}

QString Quotient::cacheLocation(const QString& dirName)
{
    const QString cachePath =
        QStandardPaths::writableLocation(QStandardPaths::CacheLocation) % u'/'
        % dirName % u'/';
    if (const QDir dir(cachePath); !dir.exists())
        dir.mkpath("."_ls);
    return cachePath;
}

qreal Quotient::stringToHueF(const QString& s)
{
    Q_ASSERT(!s.isEmpty());
    const auto hash =
        QCryptographicHash::hash(s.toUtf8(), QCryptographicHash::Sha1);
    QDataStream dataStream(hash.left(2));
    dataStream.setByteOrder(QDataStream::LittleEndian);
    quint16 hashValue = 0;
    dataStream >> hashValue;
    const auto hueF = qreal(hashValue) / std::numeric_limits<quint16>::max();
    Q_ASSERT((0 <= hueF) && (hueF <= 1));
    return hueF;
}

static const auto ServerPartRegEx = QStringLiteral(
    "(\\[[^][:space:]]+]|[-[:alnum:].]+)" // IPv6 address or hostname/IPv4 address
    "(?::(\\d{1,5}))?" // Optional port
);

QString Quotient::serverPart(const QString& mxId)
{
    static const QString re("^[@!#$+].*?:("_ls // Localpart and colon
                            % ServerPartRegEx % ")$"_ls);
    static const QRegularExpression parser(
        re,
        QRegularExpression::UseUnicodePropertiesOption); // Because Asian digits
    Q_ASSERT(parser.isValid());
    return parser.match(mxId).captured(1);
}

QString Quotient::versionString()
{
    return QStringLiteral(Quotient_VERSION_STRING);
}

int Quotient::majorVersion()
{
    return Quotient_VERSION_MAJOR;
}

int Quotient::minorVersion()
{
    return Quotient_VERSION_MINOR;
}

int Quotient::patchVersion()
{
    return Quotient_VERSION_PATCH;
}

bool Quotient::encryptionSupported()
{
    return E2EE_Enabled;
}
