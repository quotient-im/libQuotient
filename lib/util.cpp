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

static const auto RegExpOptions =
    QRegularExpression::CaseInsensitiveOption
    | QRegularExpression::OptimizeOnFirstUsageOption
    | QRegularExpression::UseUnicodePropertiesOption;

/** Converts all that looks like a URL into HTML links */
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
    pt.replace('\n', "<br/>");

    linkifyUrls(pt);
    return pt;
}
