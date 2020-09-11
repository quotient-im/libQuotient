#include "uri.h"

#include "logging.h"

#include <QtCore/QRegularExpression>

using namespace Quotient;

struct ReplacePair { QByteArray uriString; char sigil; };
/// Defines bi-directional mapping of path prefixes and sigils
static const auto replacePairs = {
    ReplacePair { "user/", '@' },
    { "roomid/", '!' },
    { "room/", '#' },
    // The notation for bare event ids is not proposed in MSC2312 but there's
    // https://github.com/matrix-org/matrix-doc/pull/2644
    { "event/", '$' }
};

Uri::Uri(QByteArray primaryId, QByteArray secondaryId, QString query)
{
    if (primaryId.isEmpty())
        primaryType_ = Empty;
    else {
        setScheme("matrix");
        QString pathToBe;
        primaryType_ = Invalid;
        if (primaryId.size() < 2) // There should be something after sigil
            return;
        for (const auto& p: replacePairs)
            if (primaryId[0] == p.sigil) {
                primaryType_ = Type(p.sigil);
                pathToBe = p.uriString + primaryId.mid(1);
                break;
            }
        if (!secondaryId.isEmpty()) {
            if (secondaryId.size() < 2) {
                primaryType_ = Invalid;
                return;
            }
            pathToBe += "/event/" + secondaryId.mid(1);
        }
        setPath(pathToBe);
    }
    setQuery(std::move(query));
}

Uri::Uri(QUrl url) : QUrl(std::move(url))
{
    // NB: don't try to use `url` from here on, it's moved-from and empty
    if (isEmpty())
        return; // primaryType_ == Empty

    if (!QUrl::isValid()) { // MatrixUri::isValid() checks primaryType_
        primaryType_ = Invalid;
        return;
    }

    if (scheme() == "matrix") {
        // Check sanity as per https://github.com/matrix-org/matrix-doc/pull/2312
        const auto& urlPath = path();
        const auto& splitPath = urlPath.splitRef('/');
        switch (splitPath.size()) {
        case 2:
            break;
        case 4:
            if (splitPath[2] == "event")
                break;
            [[fallthrough]];
        default:
            return; // Invalid
        }

        for (const auto& p: replacePairs)
            if (urlPath.startsWith(p.uriString)) {
                primaryType_ = Type(p.sigil);
                return; // The only valid return path for matrix: URIs
            }
        qCDebug(MAIN) << "The matrix: URI is not recognised:"
                      << toDisplayString();
        return;
    }

    primaryType_ = NonMatrix; // Default, unless overridden by the code below
    if (scheme() == "https" && authority() == "matrix.to") {
        // See https://matrix.org/docs/spec/appendices#matrix-to-navigation
        static const QRegularExpression MatrixToUrlRE {
            R"(^/(?<main>[^/?]+)(/(?<sec>[^?]+))?(\?(?<query>.+))?$)"
        };
        // matrix.to accepts both literal sigils (as well as & and ? used in
        // its "query" substitute) and their %-encoded forms;
        // so force QUrl to decode everything.
        auto f = fragment(QUrl::FullyDecoded);
        if (auto&& m = MatrixToUrlRE.match(f); m.hasMatch())
            *this = Uri { m.captured("main").toUtf8(),
                          m.captured("sec").toUtf8(), m.captured("query") };
    }
}

Uri::Uri(const QString& uriOrId) : Uri(fromUserInput(uriOrId)) {}

Uri Uri::fromUserInput(const QString& uriOrId)
{
    if (uriOrId.isEmpty())
        return {}; // type() == None

    // A quick check if uriOrId is a plain Matrix id
    // Bare event ids cannot be resolved without a room scope as per the current
    // spec but there's a movement towards making them navigable (see, e.g.,
    // https://github.com/matrix-org/matrix-doc/pull/2644) - so treat them
    // as valid
    if (QStringLiteral("!@#+$").contains(uriOrId[0]))
        return Uri { uriOrId.toUtf8() };

    return Uri { QUrl::fromUserInput(uriOrId) };
}

Uri::Type Uri::type() const { return primaryType_; }

Uri::SecondaryType Uri::secondaryType() const
{
    return path().section('/', 2, 2) == "event" ? EventId : NoSecondaryId;
}

QUrl Uri::toUrl(UriForm form) const
{
    if (!isValid())
        return {};

    if (form == CanonicalUri || type() == NonMatrix)
        return *this;

    QUrl url;
    url.setScheme("https");
    url.setHost("matrix.to");
    url.setPath("/");
    auto fragment = '/' + primaryId();
    if (const auto& secId = secondaryId(); !secId.isEmpty())
        fragment += '/' + secId;
    if (const auto& q = query(); !q.isEmpty())
        fragment += '?' + q;
    url.setFragment(fragment);
    return url;
}

QString Uri::primaryId() const
{
    if (primaryType_ == Empty || primaryType_ == Invalid)
        return {};

    const auto& idStem = path().section('/', 1, 1);
    return idStem.isEmpty() ? idStem : primaryType_ + idStem;
}

QString Uri::secondaryId() const
{
    const auto& idStem = path().section('/', 3);
    return idStem.isEmpty() ? idStem : secondaryType() + idStem;
}

static const auto ActionKey = QStringLiteral("action");

QString Uri::action() const
{
    return type() == NonMatrix || !isValid()
               ? QString()
               : QUrlQuery { query() }.queryItemValue(ActionKey);
}

void Uri::setAction(const QString& newAction)
{
    if (!isValid()) {
        qCWarning(MAIN) << "Cannot set an action on an invalid Quotient::Uri";
        return;
    }
    QUrlQuery q { query() };
    q.removeQueryItem(ActionKey);
    q.addQueryItem(ActionKey, newAction);
    setQuery(q);
}

QStringList Uri::viaServers() const
{
    return QUrlQuery { query() }.allQueryItemValues(QStringLiteral("via"),
                                                    QUrl::EncodeReserved);
}

bool Uri::isValid() const
{
    return primaryType_ != Empty && primaryType_ != Invalid;
}
