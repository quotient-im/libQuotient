// SPDX-FileCopyrightText: The Quotient Project Contributors
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include "util.h"

namespace Quotient {
class Room;
}

Q_DECLARE_OPAQUE_POINTER(Quotient::Room*)

namespace Quotient::HtmlFilter {
Q_NAMESPACE

//! Options supported by HtmlFilter functions
enum Option : unsigned char {
    Default = 0x0,
    //! Treat `<body>` contents as Markdown (toMatrix() only)
    ConvertMarkdown = 0x1,
    //! Treat `<body>` contents as a fragment in a bigger HTML payload (suppresses markup processing
    //! inside HTML elements and `<mx-reply>` conversion - toMatrix() only)
    Fragment = 0x2,
    //! Stop at tags not allowed in Matrix, instead of ignoring them (from*() functions only)
    Validate = 0x4,
    //! Remove <mx-reply> elements previously used for reply fallbacks
    StripMxReply = 0x8
};
Q_ENUM_NS(Option)
Q_DECLARE_FLAGS(Options, Option)

//! \brief Additional context to enrich the filtered HTML
//!
//! For now the contents of this class are only used to add query parameters to mxc URLs so that
//! these URLs could be directly requested via Quotient::NetworkAccessManager, without translating
//! them into Matrix CS API calls. In the future they might be used for other purposes too, such as
//! extra handling of room member mentions.
//!
//! \sa Room::makeMediaUrl()
struct QUOTIENT_API Context {
    Room* room;
    EventId eventId{};

    Q_GADGET
    Q_PROPERTY(Quotient::Room* room MEMBER room CONSTANT)
    Q_PROPERTY(QString eventId MEMBER eventId CONSTANT)
};

//! \brief Result structure for HTML parsing
//!
//! This is the return type of from*() functions, which, unlike toMatrix(), can't assume that HTML
//! it receives is valid since it either comes from the wire or a user input and therefore need a
//! means to report an error when the parser cannot cope (most often because of incorrectly closed
//! tags but also if plain incorrect HTML is passed).
//!
//! \sa fromMatrix(), fromLocal()
struct QUOTIENT_API Result {
    /// HTML that the filter managed to produce (incomplete in case of error)
    QString filteredHtml {};
    /// The position at which the first error was encountered; -1 if no error
    QString::size_type errorPos = -1;
    /// The human-readable error message; empty if no error
    QString errorString {};

    Q_GADGET
    Q_PROPERTY(QString filteredHtml MEMBER filteredHtml CONSTANT)
    Q_PROPERTY(QString::size_type errorPos MEMBER errorPos CONSTANT)
    Q_PROPERTY(QString errorString MEMBER errorString CONSTANT)
};

//! \brief Convert user input to Matrix-flavoured HTML
//!
//! This function takes user input in \p markup and converts it to the Matrix flavour of HTML.
//! The text in \p markup is treated as-if taken from QTextDocument[Fragment]::toHtml(); however,
//! the body of this HTML is itself treated as (HTML-encoded) markup as well, in assumption that
//! rich text (in QTextDocument sense) is exported as the outer level of HTML while the user adds
//! their own HTML inside that rich text. The function decodes and merges the two levels of markup
//! before converting the resulting HTML to its Matrix flavour.
//!
//! When compiling with Qt 5.14 or newer, it is possible to pass ConvertMarkdown in \p options
//! in order to handle the user's markup as a mix of Markdown and HTML. In that case the function
//! will first turn the Markdown parts to HTML and then merge the resulting HTML snippets with
//! the outer markup.
//!
//! The function removes HTML tags disallowed in Matrix; on top of that, it cleans away extra parts
//! (DTD, `head`, top-level `p`, extra `span` inside hyperlinks etc.) added by Qt when exporting
//! QTextDocument to HTML, and converts some formatting that can be represented in Matrix to tags
//! and attributes allowed by the CS API spec.
//!
//! \note This function assumes well-formed XHTML produced by Qt classes; while it corrects
//!       unescaped ampersands (`&`) it does not try to turn HTML to XHTML, as from*() functions do.
//!       In case of an error, debug builds will crash on assertion; release builds will silently
//!       stop processing and return what could be processed so far.
//!
//! \sa https://matrix.org/docs/spec/client_server/latest#m-room-message-msgtypes
QUOTIENT_API Q_INVOKABLE QString
toMatrix(const QString& qtMarkup, const Quotient::HtmlFilter::Context& context,
         Quotient::HtmlFilter::Options options = Quotient::HtmlFilter::Default);

//! \brief Make the received HTML with Matrix attributes compatible with Qt
//!
//! Similar to toMatrix(), this function removes HTML tags disallowed in Matrix and cleans away
//! extraneous HTML parts but it does the reverse conversion of Matrix-specific attributes to
//! the HTML subset that Qt supports. It can deal with a few more irregularities compared to
//! toMatrix(), but still doesn't recover from, e.g., missing closing tags except those usually
//! not closed in HTML (`br` etc.). In case of an irrecoverable error the returned structure will
//! contain the error details (position and brief description), along with whatever HTML
//! the function managed to produce before the failure.
//!
//! \param matrixHtml text in Matrix HTML that should be converted to Qt HTML
//! \param context optional room context
//! \param options whether the algorithm should stop at disallowed HTML tags
//!                rather than ignore them and try to continue
//! \sa HtmlFilter::Result
//! \sa https://matrix.org/docs/spec/client_server/latest#m-room-message-msgtypes
QUOTIENT_API Q_INVOKABLE Quotient::HtmlFilter::Result fromMatrix(
    const QString& matrixHtml, const Quotient::HtmlFilter::Context& context,
    Quotient::HtmlFilter::Options options = Quotient::HtmlFilter::Default);

//! \brief Make the received generic HTML compatible with Qt and convertible to Matrix
//!
//! This function is similar to fromMatrix() in that it produces HTML that can be fed to Qt
//! components - QTextDocument[Fragment]::fromHtml(), in particular; it also uses the same way to
//! tackle irregularities and errors in HTML and removes tags and attributes that cannot be
//! converted to Matrix. Unlike fromMatrix() that accepts Matrix-flavoured HTML, this function
//! accepts generic HTML and allows a few exceptions compared to the Matrix spec recommendations
//! for HTML; specifically, it preserves the `head` element; and `id`, `class`, and `style`
//! attributes throughout HTML are not restricted, allowing generic CSS stuff to do its job inasmuch
//! as Qt supports that.
//!
//! The case for this function is loading a piece of external HTML into a Qt component in order to
//! later translate it to Matrix HTML - e.g. drag-n-drop/clipboard paste into the client's message
//! input control.
//!
//! \sa fromMatrix
QUOTIENT_API Q_INVOKABLE Quotient::HtmlFilter::Result fromLocal(
    const QString& html, const Quotient::HtmlFilter::Context& context,
    Quotient::HtmlFilter::Options options = Quotient::HtmlFilter::Fragment);

} // namespace Quotient::HtmlFilter
