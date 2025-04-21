// SPDX-FileCopyrightText: The Quotient Project Contributors
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "htmlfilter.h"

#include "logging_categories_p.h"
#include "ranges_extras.h"
#include "room.h"

#include <QtCore/QXmlStreamReader>
#include <QtCore/QXmlStreamWriter>
#include <QtGui/QFontDatabase>
#include <QtGui/QTextDocument>

#include <stack>

using Quotient::HTMLFILTER;

namespace {
using namespace std;
using namespace Quotient::Literals;
using namespace Quotient::HtmlFilter;
using Quotient::rangeContains;

enum Mode : unsigned char { QtToMatrix, MatrixToQt, GenericToQt };

class ElementFilter {
public:
    ElementFilter(Mode mode, const Context& context) : _mode(mode), _context(context) {}

    using rewrite_t = vector<pair<QString, QXmlStreamAttributes>>;

    [[nodiscard]] rewrite_t rewrite(QStringView tag, QXmlStreamAttributes attributes);

private:
    const Mode _mode;
    const Context& _context;
    rewrite_t _rewrite;

    void addColorAttr(QStringView newAttrName, QStringView newAttrValue);
    void convertStyleAttr(QStringView css);
    [[nodiscard]] QXmlStreamAttribute enrichMxcUrl(QXmlStreamAttribute&& a) const;
    void filterAttr(QStringView tag, QXmlStreamAttribute&& a, const vector<QStringView>& passList);
};

class Processor : public QXmlStreamEntityResolver {
public:
    explicit Processor(const QString& html, Mode mode, Options options, const Context& context,
                       QXmlStreamWriter& writer)
        : mode(mode), options(options), context(context), writer(writer), reader(html)
    {
        reader.setEntityResolver(this);
    }

    tuple<decltype(Result::errorPos), QString, QString::size_type> run();

private:
    using open_tags_t = stack<QString, vector<QString>>;

    //! \brief The current input elements stack and their correspondence to elements in the output
    //!
    //! The entry in the (outer) stack corresponds to each level in the source
    //! document; the (inner) stack in each entry records open elements in the
    //! target document.
    using tags_stack_t = stack<open_tags_t, vector<open_tags_t>>;

    const Mode mode;
    const Options options;
    const Context& context;
    QXmlStreamWriter& writer;

    QXmlStreamReader reader;
    tags_stack_t tagsStack{};

    //! \brief The buffer on the way to QXmlStreamWriter
    //!
    //! Accumulates characters and resolved entry references until the next
    //! tag (opening or closing); used to linkify (or process Markdown in)
    //! text parts.
    QString textBuffer{};
    decltype(declval<QXmlStreamReader>().characterOffset()) bodyOffset = 0;
    bool firstElement = true;
    bool inAnchor = false;

    qsizetype errorPos = -1;
    QString errorString {};

    //! \brief Process the next XML token available from the reader
    //! \return whether non-white-space was encountered under <body>
    [[nodiscard]] bool processCurrentToken(QXmlStreamReader::TokenType tokenType);
    [[nodiscard]] bool processStartElement();
    void filterText(QString& text);

    QString resolveUndeclaredEntity(const QString& name) override
    {
        return name == u"nbsp" ? u"\xa0"_s : QString();
    }
};

constexpr auto permittedTags = to_array<QStringView>(
    {u"font",       u"del", u"h1",    u"h2",     u"h3",      u"h4",  u"h5",   u"h6",
     u"blockquote", u"p",   u"a",     u"ul",     u"ol",      u"sup", u"sub",  u"li",
     u"b",          u"i",   u"u",     u"strong", u"em",      u"s",   u"code", u"hr",
     u"br",         u"div", u"table", u"thead",  u"tbody",   u"tr",  u"th",   u"td",
     u"caption",    u"pre", u"span",  u"img",    u"mx-reply"});

struct PassList {
    QStringView tag;
    vector<QStringView> allowedAttrs;
};

const auto passLists = to_array<PassList>({
    {u"a", {u"name", u"target", /* u"href" - only from permittedSchemes */}},
    {u"img",
     {u"width", u"height", u"alt", u"title", u"data-mx-emoticon", /* u"src" - only 'mxc:' */}},
    {u"ol", {u"start"}},
    {u"font", {u"color", u"data-mx-color", u"data-mx-bg-color"}},
    {u"span", {u"color", u"data-mx-color", u"data-mx-bg-color"}},
    // { u"code", { u"class" /* must start with 'language-' */ } }
});

constexpr auto permittedSchemes = to_array<QStringView>({
    u"http:", u"https:", u"ftp:", u"mailto:", u"magnet:", u"matrix:", u"mxc:" /* MSC2398 */
});

constexpr auto htmlColorAttr = u"color";
constexpr auto htmlStyleAttr = u"style";
constexpr auto mxColorAttr = u"data-mx-color";
constexpr auto mxBgColorAttr = u"data-mx-bg-color";

//! Find the first element in the rewrite that would accept colour attributes (`font` and, only in
//! Matrix HTML, `span`), and add the passed attribute to it
inline void ElementFilter::addColorAttr(QStringView newAttrName, QStringView newAttrValue)
{
    auto colourableIt = ranges::find_if(_rewrite, [this](const rewrite_t::value_type& element) {
        return element.first == u"font" || (_mode == QtToMatrix && element.first == u"span");
    });
    if (colourableIt == _rewrite.end())
        colourableIt = _rewrite.insert(_rewrite.end(), {u"font"_s, {}});
    colourableIt->second.append(newAttrName.toString(), newAttrValue.toString());
}

template <size_t Len>
inline QStringView cssValue(QStringView css, const char16_t (&propertyNameWithColon)[Len])
{
    return css.startsWith(propertyNameWithColon) ? css.mid(Len - 1).trimmed() : QStringView();
}

void ElementFilter::convertStyleAttr(QStringView css)
{
    // 'style' attribute is not allowed in Matrix; convert
    // everything possible to tags and other attributes
    const auto& cssProperties = css.split(u';');
    for (auto p : cssProperties) {
        p = p.trimmed();
        if (p.isEmpty())
            continue;
        if (auto v = cssValue(p, u"color:"); !v.isEmpty()) {
            addColorAttr(mxColorAttr, v);
        } else if (v = cssValue(p, u"background-color:"); !v.isEmpty())
            addColorAttr(mxBgColorAttr, v);
        else if (v = cssValue(p, u"font-weight:");
                 v == u"bold" || v == u"bolder" || v.toFloat() > 500)
            _rewrite.emplace_back().first = u"b"_s;
        else if (v = cssValue(p, u"font-style:"); v == u"italic" || v.startsWith(u"oblique"))
            _rewrite.emplace_back().first = u"i"_s;
        else if (cssValue(p, u"text-decoration:").contains(u"line-through"))
            _rewrite.emplace_back().first = u"del"_s;
        else {
            const auto& fontFamilies = cssValue(p, u"font-family:").split(u',');
            for (auto ff : views::transform(fontFamilies, &QStringView::trimmed)
                               | views::filter(std::not_fn(&QStringView::empty))) {
                if (ff.front() == u'\'' || ff.front() == u'"')
                    ff = ff.mid(1, ff.size() - 2);
                if (QFontDatabase::isFixedPitch(ff.toString())) {
                    _rewrite.emplace_back().first = u"code"_s;
                    break;
                }
            }
        }
    }
}

//! Enrich mxc source URL for images with the context so that Quotient::NAM could resolve them
QXmlStreamAttribute ElementFilter::enrichMxcUrl(QXmlStreamAttribute&& a) const
{
    const auto aName = a.qualifiedName().toString();
    auto url = QUrl::fromUserInput(a.value().toString());
    if (_mode == QtToMatrix) {
        // Make sure the mxc URL is just that, with no internal extras
        QUrlQuery q{url.query()};
        for (const auto& k : {u"user_id"_s, u"room_id"_s, u"event_id"_s})
            q.removeAllQueryItems(k);
        url.setQuery(q);
        return QXmlStreamAttribute(aName, url.toString(QUrl::FullyEncoded));
    } else if (_context.room)
        return QXmlStreamAttribute(
            aName, _context.room->makeMediaUrl(_context.eventId, url).toString(QUrl::FullyEncoded));

    return std::move(a);
}

void ElementFilter::filterAttr(QStringView tag, QXmlStreamAttribute&& a,
                               const vector<QStringView>& passList)
{
    const auto aName = a.qualifiedName();
    const auto aValue = a.value();
    auto& targetAttrs = _rewrite.front().second;

    // Attribute conversions between Matrix and Qt subsets; generic HTML
    // is treated as possibly-Matrix
    if (_mode != QtToMatrix) {
        if (aName == mxColorAttr) {
            addColorAttr(htmlColorAttr, aValue.toString());
            return;
        } else if (aName == mxBgColorAttr) {
            targetAttrs.append(QString::fromUtf16(htmlStyleAttr),
                               u"background-color:" % aValue.toString());
            return;
        }
    } else {
        if (aName == htmlStyleAttr) {
            convertStyleAttr(aValue);
            return;
        } else if (aName == htmlColorAttr)
            addColorAttr(mxColorAttr, aValue); // Add to 'color', so return false
    }

    if (tag == u"img" && aName == u"src" && aValue.startsWith(u"mxc:")) {
        targetAttrs.push_back(enrichMxcUrl(std::move(a)));
        return;
    }

    // Generic filtering for attributes
    if ((_mode == GenericToQt && (aName == htmlStyleAttr || aName == u"class" || aName == u"id"))
        || (tag == u"a" && aName == u"href"
            && ranges::any_of(permittedSchemes, [&aValue](QStringView s) {
        return aValue.startsWith(s);
    })) || rangeContains(passList, aName))
        targetAttrs.push_back(std::move(a));
}

ElementFilter::rewrite_t ElementFilter::rewrite(QStringView tag, QXmlStreamAttributes attributes)
{
    if (_mode == MatrixToQt) {
        if (tag == u"del" || tag == u"strike") { // Qt doesn't support these...
            QXmlStreamAttributes attrs;
            attrs.append(u"style"_s, u"text-decoration:line-through"_s);
            return {{u"font"_s, std::move(attrs)}};
        }
        if (tag == u"mx-reply")
            return {{u"div"_s, {}}}; // The spec says that mx-reply is HTML div
        // If `mx-reply` is encountered on the way to the wire, just pass it
    }

    if (tag == u"code" && _mode != GenericToQt) { // Special case
        erase_if(attributes, [](const auto& a) {
            return a.qualifiedName() != u"class" || !a.value().startsWith(u"language-");
        });
        return {{tag.toString(), std::move(attributes)}};
    }

    if (!rangeContains(permittedTags, tag))
        return {}; // The tag is not allowed

    _rewrite.push_back({tag.toString(), {}});
    const auto it = ranges::find(passLists, tag, &PassList::tag);
    if (it == end(passLists))
        return _rewrite; // Drop all attributes, pass the tag

    for (auto&& a : attributes)
        filterAttr(tag, std::move(a), it->allowedAttrs);

    // Remove <font> and <span> that ended up without attributes as these are no-op
    erase_if(_rewrite, [](const rewrite_t::value_type& e) {
        return e.second.empty() && (e.first == u"font" || e.first == u"span");
    });

    return _rewrite;
}

// The following function intends to merge user-entered Markdown+HTML markup (HTML-escaped at this
// point) into HTML exported by QTextDocument. Unfortunately, Markdown engine of QTextDocument is
// not dealing well with ampersands and &-escaped HTML entities inside HTML tags: see
// https://bugreports.qt.io/browse/QTBUG-91222 for details. Instead, Processor::run() splits
// segments between HTML tags and filterText() treats each of them as Markdown individually.
#ifdef Quotient_ENABLE_MERGE_MARKDOWN
[[nodiscard]] QString mergeMarkdown(const QString& html)
{
    QXmlStreamReader reader(html);
    QString mdWithHtml;
    QXmlStreamWriter writer(&mdWithHtml);
    while (reader.readNext() != QXmlStreamReader::StartElement || reader.qualifiedName() != u"p")
        if (reader.atEnd()) {
            Q_ASSERT_X(false, __FUNCTION__, "Malformed Qt markup");
            qCCritical(HTMLFILTER) << "The passed text doesn't seem to come from QTextDocument";
            return {};
        }

    int depth = 1; // Count <p> just entered
    while (!reader.atEnd()) {
        // Minimal validation, just pipe things through
        // decoding what needs decoding
        const auto tokenType = reader.readNext();
        switch (tokenType) {
        case QXmlStreamReader::Characters:
        case QXmlStreamReader::EntityReference: {
            auto text = reader.text().toString();
            if (depth > 1)
                break;

            // Flush the writer's buffer before side-writing
            writer.writeCharacters({});
            mdWithHtml += text; // Append text as is
            continue;
        }

        case QXmlStreamReader::StartElement:
            ++depth;
            if (reader.qualifiedName() != u"p")
                break;
            // Convert <p> elements except the first one
            // to Markdown paragraph breaks
            writer.writeCharacters(u"\n\n"_s);
            continue;
        case QXmlStreamReader::EndElement:
            --depth;
            if (reader.qualifiedName() == u"p")
                continue; // See above in StartElement
            break;
        case QXmlStreamReader::Comment:
            continue; // Just drop comments
        default:
            qCWarning(HTMLFILTER) << "Unexpected token, type" << tokenType;
        }
        if (depth < 0) {
            Q_ASSERT(tokenType == QXmlStreamReader::EndElement && reader.qualifiedName() == u"body");
            break;
        }
        writer.writeCurrentToken(reader);
    }
    writer.writeEndElement();
    QTextDocument doc;
    doc.setMarkdown(mdWithHtml);
    return doc.toHtml();
}
#endif

struct IndexAndLength {
    QString::size_type index;
    QString::size_type length = 0;
};

//! Call QString::replace() and return the difference between the old and the new substring length
[[nodiscard]] inline QString::size_type replace(QString& s, IndexAndLength at, const QString& with)
{
    s.replace(at.index, at.length, with);
    return with.size() - at.length;
}

//! Turn minimized attributes between \p pos and \p gtPos to full-fledged ones by appending `=''`
//! \return the new position of the tag's closing bracket (\p gtPos)
//! \sa https://www.w3.org/TR/xhtml1/diffs.html#h-4.5
[[nodiscard]] inline QString::size_type processMinimizedAttrs(QString& html, QString::size_type pos,
                                                              QString::size_type gtPos)
{
    // There's no simple way to replace all occurrences within a string segment; so just go through
    // the segment and insert `=''` after minimized attributes.
    // This is not the place to _filter_ allowed/disallowed attributes - all filtering should
    // happen in ElementFilter
    static const auto MinAttrRE =
        R"(([^[:space:]>/"'=]+)\s*(=\s*([^[:space:]>/"']|"[^"]*"|'[^']*')+)?)"_qre;
    QRegularExpressionMatch m;
    while ((m = MinAttrRE.match(html, pos)).hasMatch() && m.capturedEnd(1) < gtPos) {
        pos = m.capturedEnd();
        if (m.captured(2).isEmpty()) {
            const auto d = replace(html, {m.capturedEnd(1)}, u"=''"_s);
            gtPos += d;
            pos += d;
        }
    }
    return gtPos;
}

//! Close elements known to be empty in HTML (such as img or meta) if they are not self-closing
[[nodiscard]] inline QString::size_type closeEmptyElements(const QString& tag, QString& html,
                                                           QString::size_type gtPos)
{
    static const QRegularExpression EmptyElementRE{"^img|[hb]r|meta$"_L1,
                                                   QRegularExpression::CaseInsensitiveOption};
    if (html[gtPos - 1] != u'/' && EmptyElementRE.match(tag).hasMatch())
        gtPos += replace(html, {gtPos}, u"/"_s);

    return gtPos;
}

inline bool isBetween(auto v, const auto lo, const auto hi) { return v >= lo && v <= hi; }

[[nodiscard]] inline bool isTagNameTerminator(QChar c)
{
    return c.isSpace() || c == u'/' || c == u'>';
}

//! \brief Massage user HTML to look more like XHTML
//!
//! Since Qt doesn't have an HTML parser (outside of QTextDocument) Processor::run() uses
//! QXmlStreamReader instead, and it's quite picky about properly closed tags and escaped &'s.
//! &'s are dealt with in process() as they have to be escaped regardless of the conversion type.
//! This helper function further tries to convert the passed HTML to something more XHTML-like,
//! so that the XML reader doesn't choke on, e.g., unclosed `br` or `img` tags and minimised HTML
//! attributes. It also filters away tags that are not compliant with Matrix specification, where
//! appropriate.
[[nodiscard]] Result preprocess(QString html, Mode mode, Options options)
{
    Q_ASSERT(mode != QtToMatrix);
    bool isFragment = options.testFlag(Fragment) || mode == MatrixToQt;
    bool inHead = false;
    for (auto pos = html.indexOf(u'<'); pos != -1; pos = html.indexOf(u'<', pos)) {
        const auto tagNamePos = pos + 1 + static_cast<int>(html[pos + 1] == u'/');
        const auto uncheckedHtml = QStringView(html).mid(tagNamePos);
        static constexpr auto commentOpen = "!--"_L1;
        static constexpr auto commentClose = "-->"_L1;
        if (uncheckedHtml.startsWith(commentOpen)) { // Skip comments
            pos = html.indexOf(commentClose, tagNamePos + commentOpen.size()) + commentClose.size();
            continue;
        }
        // Look ahead to detect stray < and escape it
        auto gtPos = html.indexOf(u'>', tagNamePos);
        if (gtPos == tagNamePos /* <> or </> */ || gtPos == -1 /* no more > */
            || isBetween(html.indexOf(u'<', tagNamePos), 0, gtPos - 1) /* another < before > */) {
            pos += replace(html, {pos, 1}, u"&lt;"_s); // Put pos after the escaped sequence
            continue;
        }
        if (uncheckedHtml.startsWith(u"head>", Qt::CaseInsensitive)) {
            if (mode == MatrixToQt) {
                // Matrix spec doesn't allow <head>; report if it occurs in
                // user input (Validate is on) or remove the whole header if
                // it comes from the wire (Validate is off).
                if (options.testFlag(Validate))
                    return { {}, pos, u"<head> elements are not allowed in Matrix"_s };
                static constexpr auto HeadEnd = "</head>"_L1;
                const auto headEndPos = html.indexOf(HeadEnd, tagNamePos, Qt::CaseInsensitive);
                html.remove(pos, headEndPos - pos + HeadEnd.size());
                continue;
            }
            Q_ASSERT(mode == GenericToQt);
            inHead = html[pos + 1] != u'/'; // Track header entry and exit
            if (!inHead) { // Just exited, </head>
                pos = gtPos + 1;
                continue;
            }
        }

        const auto tagEndIt = ranges::find_if(uncheckedHtml, isTagNameTerminator);
        const auto tag = uncheckedHtml.left(tagEndIt - uncheckedHtml.cbegin()).toString().toLower();
        // <head> contents are necessary to apply styles but obviously
        // neither `head` nor tags inside of it are in permittedTags;
        // however, minimised attributes still have to be handled everywhere
        // and <meta> tags should be closed
        if (mode == GenericToQt && (tag == u"html" || tag == u"body")) {
            // Only in generic mode, allow <html> and <body>
            pos += tagNamePos + tag.size() + 1;
            isFragment = false;
            continue;
        }
        // Check if it's a valid (opening or closing) tag allowed in Matrix
        if (!inHead && !rangeContains(permittedTags, tag)) {
            // Invalid tag or non-tag - either remove the abusing piece or stop and report
            if (options.testFlag(Validate))
                return {{},
                        pos,
                        u"Non-tag or disallowed tag: "_s % uncheckedHtml.left(gtPos - tagNamePos)};

            html.remove(pos, gtPos - pos + 1);
            continue;
        }

        gtPos = processMinimizedAttrs(html, tagNamePos + tag.size(), gtPos);
        pos = closeEmptyElements(tag, html, gtPos) + 1;
        Q_ASSERT(pos > 0);
    }
    // Wrap in a no-op tag to make the text look like valid XML if it's
    // a fragment (always the case when HTML comes from a homeserver, and
    // possibly with generic HTML).
    if (isFragment)
        html = u"<span>" % html % u"</span>";
    // Discard characters behind the last tag (LibreOffice attaches \n\0, e.g.)
    html.truncate(html.lastIndexOf(u'>') + 1);
    return { html };
}

Result process(QString html, Mode mode, const Context& context, Options options)
{
    // Since Qt doesn't have an HTML parser (outside of QTextDocument; and the one in QTextDocument
    // is opinionated and not configurable) Processor::run() uses QXmlStreamReader instead. Being
    // an XML parser, this class is quite picky about properly closed tags, escaped ampersands etc.
    // Before passing to run(), the following code tries to bring the passed HTML to something more
    // XHTML-like, so that the XML parser doesn't choke on things HTML-but-not-XML. In QtToMatrix
    // mode the only such thing is unescaped ampersands in attributes (especially `href`), since
    // QTextDocument::toHtml() produces (otherwise) valid XHTML. In other modes no such assumption
    // can be made so an attempt is taken to close elements that are normally empty (`br`, `hr` and
    // `img`), turn minimised attributes to their full interpretations (`disabled -> disabled=''`)
    // and remove things that are obvious non-tags around unescaped `<` characters -
    // see preprocess() for all the gory details.

    // Escape ampersands outside of character entities
    static const auto freestandingAmps =
      "&(?!(#[0-9]+|#x[0-9a-fA-F]+|[[:alpha:]_][-[:alnum:]_:.]*);)"_qre;
    html.replace(freestandingAmps, QStringLiteral("&amp;"));

    if (mode != GenericToQt) {
        // Handling control codes (excluding, for this discussion, \n, \r, and \t) in HTML is
        // somewhat messy. HTML 4, XML 1.0, XHTML 1.0 all disallow C0/C1 control codes in any form.
        // XML 1.1 allows them as numeric character references (aka NCRs) but QXmlStreamReader only
        // implements XML 1.0 and doesn't accept them even as NCRs. Meanwhile, QTextDocument emits
        // control codes to HTML without any conversion, formally violating HTML 4 spec
        // (https://bugreports.qt.io/browse/QTBUG-122466) and, more importantly for this code,
        // upsetting QXmlStreamReader (#900). HTML 5 (which Matrix HTML is - assumed to be -
        // based on) formally disallows control codes too, adding \f to the allowed exclusions
        // (see https://dev.w3.org/html5/spec-LC/syntax.html#text-0) which gives us the right to
        // eliminate control characters from Matrix payloads, even though the Web generally seems
        // to admit them as NCRs.
        // NB: [:cntrl:] doesn't work because it includes the allowed \n, \r, \t
        static const auto controlCharRE = R"([\x01-\x08\x0b\x0c\x0e-\x1f\x7f-\x9f])"_qre;
        html.remove(controlCharRE);
    }

    if (mode == QtToMatrix) {
        if (options.testFlag(ConvertMarkdown)) {
            // The processor handles Markdown in chunks between HTML tags; <br /> breaks character
            // sequences that are otherwise valid Markdown, leading to issues with, e.g., lists.
            html.replace(QStringLiteral("<br />"), QStringLiteral("\n"));
#if 0
            html = mergeMarkdown(html);
            if (html.isEmpty())
                return { "", 0, "This markup doesn't seem to be sourced from Qt" };
            options &= ~ConvertMarkdown;
#endif
        }
    } else {
        auto r = preprocess(html, mode, options);
        if (r.errorPos != -1)
            return r;
        html = r.filteredHtml;
    }

    QString resultHtml;
    QXmlStreamWriter writer(&resultHtml);
    writer.setAutoFormatting(false);
    const auto [errorPos, errorString, rawOffset] =
        Processor(html, mode, options, context, writer).run();
    if (errorPos != -1) {
        qCCritical(HTMLFILTER) << "Invalid XHTML:" << html;
        qCCritical(HTMLFILTER).nospace() << "Error at char " << errorPos << ": " << errorString;
        const auto remainder = QStringView(html).mid(rawOffset);
        qCCritical(HTMLFILTER).nospace() << "Buffer at error: " << remainder << ", "
                                         << remainder.size() << " character(s) remaining";
    }

    return { resultHtml.trimmed(), errorPos, errorString };
}

tuple<decltype(Result::errorPos), QString, QString::size_type> Processor::run()
{
    while (!reader.atEnd()) {
        const auto tokenType = reader.readNext();
        if (bodyOffset == -1) // See processStartElement()
            bodyOffset = reader.characterOffset(); // As of the token just read

        if (!textBuffer.isEmpty() && !reader.isCharacters() && !reader.isEntityReference())
            filterText(textBuffer);

        if (processCurrentToken(tokenType))
            firstElement &= (bodyOffset <= 0 || reader.isWhitespace());
    }

    return { errorPos, errorString, reader.characterOffset() };
}

bool Processor::processCurrentToken(QXmlStreamReader::TokenType tokenType)
{
    switch (tokenType) {
    case QXmlStreamReader::StartElement:
        return processStartElement();
    case QXmlStreamReader::Characters:
    case QXmlStreamReader::EntityReference: {
        // Remove the line break Qt inserts after <body> because it adds an unnecessary whitespace
        // in the HTML context and an unnecessary line break in the Markdown context.
        if (firstElement && mode == QtToMatrix && reader.text().startsWith(u'\n')) {
            textBuffer += reader.text().mid(1);
            return false; // Maintain firstElement
        }
        // Outside of links, defer writing until the next non-character,
        // non-entity reference token in order to pass the whole text
        // piece to filterText() with all entity references resolved.
        if (!inAnchor && !options.testFlag(Fragment))
            textBuffer += reader.text();
        else
            writer.writeCurrentToken(reader);
        break;
    }
    case QXmlStreamReader::EndElement:
        if (tagsStack.empty()) {
            if (const auto& tag = reader.qualifiedName(); tag != u"body" && tag != u"html")
                qCWarning(HTMLFILTER) << "Empty tags stack, skipping" << (u'/' % tag.toString());
            break;
        }
        // Close as many elements as were opened in case StartElement
        for (auto& t = tagsStack.top(); !t.empty(); t.pop()) {
            writer.writeEndElement();
            if (t.top() == u"a")
                inAnchor = false;
        }
        tagsStack.pop();
        break;
    case QXmlStreamReader::EndDocument:
        if (!tagsStack.empty())
            qCWarning(HTMLFILTER) << "Not all HTML tags closed at the document end";
        if (mode == GenericToQt)
            writer.writeEndDocument(); // </body></html>
        break;
    case QXmlStreamReader::NoToken:
        QUO_ALARM("Unexpected NoToken received from QXmlStreamReader");
        break;
    case QXmlStreamReader::Invalid: {
        errorPos = reader.characterOffset() - bodyOffset;
        errorString = reader.errorString();
        break;
    }
    case QXmlStreamReader::Comment:
    case QXmlStreamReader::StartDocument:
    case QXmlStreamReader::DTD:
    case QXmlStreamReader::ProcessingInstruction:
        return false; // All these should not affect firstElement state
    }
    return true;
}

//! \brief Copy the current element along with its content from \p reader to \p writer
//!
//! This is in place of a non-existent QXmlStreamWriter::writeCurrentElement() - there are only
//! skipCurrentElement() and writeCurrentToken().
void writeCurrentElement(QXmlStreamReader& reader, QXmlStreamWriter& writer)
{
    const auto elementName = reader.qualifiedName();
    // Copy through the whole element - having
    // QXmlStreamWriter::writeCurrentElement() would help but there's none such
    do {
        writer.writeCurrentToken(reader);
        const auto nextTokenType = reader.readNext();
        if (nextTokenType == QXmlStreamReader::EndElement && reader.qualifiedName() == elementName) {
            writer.writeCurrentToken(reader);
            break;
        }
    } while (!reader.atEnd());
}

bool Processor::processStartElement()
{
    const auto& tagName = reader.qualifiedName();
    if (tagsStack.empty()) {
        // These tags are invalid anywhere deeper, and we don't even care to put them to tagsStack
        if (tagName == u"html") {
            if (mode == GenericToQt)
                writer.writeCurrentToken(reader);
            return false; // Otherwise, just ignore, get to the content inside
        } else if (tagName == u"head") {
            // <head> is only needed for Qt to import HTML more accurately, and entirely
            // uninteresting in other modes
            if (mode == GenericToQt)
                writeCurrentElement(reader, writer);
            else
                reader.skipCurrentElement();
            return false; // Not in <body> yet
        } else if (tagName == u"body") {
            if (mode == GenericToQt)
                writer.writeCurrentToken(reader);
            // Except importing HTML into QTextDocument, skip just like <html> but record
            // the position for error reporting
            // (FIXME: this position is still not exactly related to the original text...)
            bodyOffset = -1; // See run()
            return true;
        }
    }
    if (options.testFlag(StripMxReply) && tagName == u"mx-reply") {
        reader.skipCurrentElement();
        return false;
    }

    auto attrs = reader.attributes();
    if (ranges::any_of(attrs, [](const auto& a) {
            return a.qualifiedName() == u"style" && a.value().contains(u"-qt-paragraph-type:empty");
        })) { // Hidden text block, just skip it
        reader.skipCurrentElement();
        return false;
    }

    tagsStack.emplace(); // NB: No skipCurrentElement() after this point
    if (tagsStack.size() > 100)
        qCCritical(HTMLFILTER) << "CS API spec limits HTML tags depth at 100";

    // Qt hardcodes the link style in a `<span>` under `<a>`. This breaks the looks on the receiving
    // side if the sender uses a different style of links from that of the receiver. Since Qt
    // decorates links when importing HTML anyway, we don't lose anything if we just strip away this
    // span tag.
    if (mode != MatrixToQt && inAnchor && textBuffer.isEmpty() && tagName == u"span"
        && attrs.size() == 1 && attrs.front().qualifiedName() == u"style")
        return false; // inAnchor == true ==> firstElement == false, no need to unset it

    // Skip the first top-level <p> and replace further top-level `<p>...</p>` with `<br/>...` -
    // kinda controversial but there's no cleaner way to get rid of the single top-level <p>
    // generated by Qt without assuming that it's the only <p> spanning the whole body (copy-pasting
    // rich text from other editors can bring several legitimate paragraphs of text, e.g.). This is
    // also a very special case where a converted tag is immediately closed, unlike the one in
    // the source text; which is why it's checked here rather than in ElementFilter
    if (mode == QtToMatrix && tagName == u"p" && tagsStack.size() == 1) {
        if (firstElement)
            return false; // Skip unsetting firstElement just yet
        writer.writeEmptyElement(u"br"_s);
        return true;
    }
    // The spec only allows `<mx-reply>` at the very beginning and it's not supposed to be
    // in the user input (user input is always analysed inside a Fragment)
    if (tagName == u"mx-reply" && (!firstElement || options.testFlag(Fragment)))
        return false;

    for (const auto& [rewrittenTag, rewrittenAttrs] :
         ElementFilter(mode, context).rewrite(tagName, std::move(attrs))) {
        tagsStack.top().push(rewrittenTag);
        writer.writeStartElement(rewrittenTag);
        writer.writeAttributes(rewrittenAttrs);
        inAnchor |= (rewrittenTag == u"a");
    }

    return true;
}

void Processor::filterText(QString& text)
{
    if (text.isEmpty())
        return;

    if (options.testFlag(ConvertMarkdown)) {
        // Protect leading/trailing whitespaces (Markdown disregards them);
        // specific string doesn't matter as long as it isn't whitespace itself,
        // doesn't have special meaning in Markdown and doesn't occur in
        // the HTML boilerplate that QTextDocument generates.
        static constexpr auto Marker = "$$"_L1;
        const bool hasLeadingWhitespace = text.cbegin()->isSpace();
        if (hasLeadingWhitespace)
            text.prepend(Marker);
        const bool hasTrailingWhitespace = (text.cend() - 1)->isSpace();
        if (hasTrailingWhitespace)
            text.append(Marker);
        const auto markerCount = text.count(Marker); // For self-check

#ifndef QTBUG_92445_FIXED
        // Protect list items from https://bugreports.qt.io/browse/QTBUG-92445
        // (see also https://spec.commonmark.org/0.29/#list-items)
        static const auto ReOptions = QRegularExpression::MultilineOption;
        static const QRegularExpression //
          UlRE(u"^( *[-+*] {1,4})(?=[^ ])"_s, ReOptions),
          OlRE(u"^( *[0-9]{1,9}+[.)] {1,4})(?=[^ ])"_s, ReOptions);
        static constexpr auto UlMarker = "@@ul@@"_L1, OlMarker = "@@ol@@"_L1;
        text.replace(UlRE, u"\\1" % UlMarker);
        text.replace(OlRE, u"\\1" % OlMarker);
        const auto markerCountOl = text.count(OlMarker);
        const auto markerCountUl = text.count(UlMarker);
#endif

        // Convert Markdown to HTML
        QTextDocument doc;
        doc.setMarkdown(text, QTextDocument::MarkdownNoHTML);
        text = doc.toHtml();

        // Delete protection characters, now buried inside HTML
#ifndef QTBUG_92445_FIXED
        QUO_CHECK(text.count(OlMarker) == markerCountOl);
        QUO_CHECK(text.count(UlMarker) == markerCountUl);
        // After HTML conversion, list markers end up being after HTML tags
        text.replace(QRegularExpression(u'>' % OlMarker), u">"_s);
        text.replace(QRegularExpression(u'>' % UlMarker), u">"_s);
#endif

        QUO_CHECK(text.count(Marker) == markerCount);
        if (hasLeadingWhitespace)
            text.remove(text.indexOf(Marker), Marker.size());
        if (hasTrailingWhitespace)
            text.remove(text.lastIndexOf(Marker), Marker.size());
    } else {
        text = text.toHtmlEscaped(); // The reader unescaped it
        Quotient::linkifyUrls(text);
        text = u"<span>" % text % u"</span>";
    }
    // Re-process this piece of text as HTML but dump text snippets as they are,
    // without recursing into filterText() again
    Processor(text, mode, Fragment, context, writer).run();

    text.clear();
}
} // anonymous namespace

namespace Quotient::HtmlFilter {

QString toMatrix(const QString& qtMarkup, const Context& context, Options options)
{
    if (QUO_ALARM_X(options.testFlag(Validate),
                    "Ignoring HtmlFilter::Validate for HTML emitted by Qt"))
        options.setFlag(Validate, false);
    const auto& result = process(qtMarkup, QtToMatrix, context, options);
    QUO_CHECK(result.errorPos == -1);
    return result.filteredHtml;
}

Result fromMatrix(const QString& matrixHtml, const Context& context, Options options)
{
    // Matrix HTML body should never be treated as Markdown
    if (QUO_ALARM_X(options.testFlag(ConvertMarkdown),
                    "Ignoring HtmlFilter::ConvertMarkdown for Matrix HTML body"))
        options.setFlag(ConvertMarkdown, false);
    auto result = process(matrixHtml, MatrixToQt, context, options);
    if (result.errorPos == -1) {
        // Make sure to preserve whitespace sequences
        result.filteredHtml =
            u"<span style=\"white-space: pre-wrap\">" % result.filteredHtml % u"</span>";
    }
    return result;
}

Result fromLocal(const QString& html, const Context& context, Options options)
{
    return process(html, GenericToQt, context, options);
}

} // namespace HtmlFilter
