#include "resourceresolver.h"

#include "connection.h"
#include "logging.h"

#include <QtCore/QRegularExpression>

using namespace Quotient;

struct ReplacePair { QByteArray uriString; char sigil; };
static const auto replacePairs = { ReplacePair { "user/", '@' },
                                   { "roomid/", '!' },
                                   { "room/", '#' } };

MatrixUri::MatrixUri(QByteArray primaryId, QByteArray secondaryId, QString query)
{
    if (primaryId.isEmpty())
        primaryType_ = Empty;
    else {
        setScheme("matrix");
        QString pathToBe;
        primaryType_ = Invalid;
        for (const auto& p: replacePairs)
            if (primaryId[0] == p.sigil) {
                primaryType_ = Type(p.sigil);
                pathToBe = p.uriString + primaryId.mid(1);
                break;
            }
        if (!secondaryId.isEmpty())
            pathToBe += "/event/" + secondaryId.mid(1);
        setPath(pathToBe);
    }
    setQuery(std::move(query));
}

MatrixUri::MatrixUri(QUrl url) : QUrl(std::move(url))
{
    // NB: url is moved from and empty by now
    if (isEmpty())
        return; // primaryType_ == None

    primaryType_ = Invalid;
    if (!QUrl::isValid()) // MatrixUri::isValid() checks primaryType_
        return;

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
        qCWarning(MAIN) << "Invalid matrix: URI passed to MatrixUri";
    }
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
            *this = MatrixUri { m.captured("main").toUtf8(),
                                m.captured("sec").toUtf8(),
                                m.captured("query") };
    }
}

MatrixUri::MatrixUri(const QString &uriOrId)
    : MatrixUri(fromUserInput(uriOrId))
{ }

MatrixUri MatrixUri::fromUserInput(const QString& uriOrId)
{
    if (uriOrId.isEmpty())
        return {}; // type() == None

    // A quick check if uriOrId is a plain Matrix id
    if (QStringLiteral("!@#+").contains(uriOrId[0]))
        return MatrixUri { uriOrId.toUtf8() };

    // Bare event ids cannot be resolved without a room scope but are treated as
    // valid anyway; in the future we might expose them as, say,
    // matrix:event/eventid
    if (uriOrId[0] == '$')
        return MatrixUri { "", uriOrId.toUtf8() };

    return MatrixUri { QUrl::fromUserInput(uriOrId) };
}

MatrixUri::Type MatrixUri::type() const { return primaryType_; }

MatrixUri::SecondaryType MatrixUri::secondaryType() const
{
    return path().section('/', 2, 2) == "event" ? EventId : NoSecondaryId;
}

QUrl MatrixUri::toUrl(UriForm form) const
{
    if (!isValid())
        return {};

    if (form == CanonicalUri)
        return *this;

    QUrl url;
    url.setScheme("https");
    url.setHost("matrix.to");
    url.setPath("/");
    auto fragment = primaryId();
    if (const auto& secId = secondaryId(); !secId.isEmpty())
        fragment += '/' + secId;
    if (const auto& q = query(); !q.isEmpty())
        fragment += '?' + q;
    url.setFragment(fragment);
    return url;
}

QString MatrixUri::toDisplayString(MatrixUri::UriForm form) const
{
    return toUrl(form).toDisplayString();
}

QString MatrixUri::primaryId() const
{
    if (primaryType_ == Empty || primaryType_ == Invalid)
        return {};

    const auto& idStem = path().section('/', 1, 1);
    return idStem.isEmpty() ? idStem : primaryType_ + idStem;
}

QString MatrixUri::secondaryId() const
{
    const auto& idStem = path().section('/', 3);
    return idStem.isEmpty() ? idStem : secondaryType() + idStem;
}

QString MatrixUri::action() const
{
    return QUrlQuery { query() }.queryItemValue("action");
}

QStringList MatrixUri::viaServers() const
{
    return QUrlQuery { query() }.allQueryItemValues(QStringLiteral("via"),
                                                    QUrl::EncodeReserved);
}

bool MatrixUri::isValid() const
{
    return primaryType_ != Empty && primaryType_ != Invalid;
}

UriResolveResult Quotient::visitResource(
    Connection* account, const MatrixUri& uri,
    std::function<void(User*)> userHandler,
    std::function<void(Room*, QString)> roomEventHandler,
    std::function<void(Connection*, QString, QStringList)> joinHandler)
{
    Q_ASSERT_X(account != nullptr, __FUNCTION__,
               "The Connection argument passed to visit/openResource must not "
               "be nullptr");
    if (uri.action() == "join") {
        if (uri.type() != MatrixUri::RoomAlias
            && uri.type() != MatrixUri::RoomId)
            return MalformedUri;

        joinHandler(account, uri.primaryId(), uri.viaServers());
        return UriResolved;
    }

    Room* room = nullptr;
    switch (uri.type()) {
    case MatrixUri::Invalid:
        return MalformedUri;
    case MatrixUri::Empty:
        return EmptyMatrixId;
    case MatrixUri::UserId:
        if (auto* user = account->user(uri.primaryId())) {
            userHandler(user);
            return UriResolved;
        }
        return MalformedUri;
    case MatrixUri::RoomId:
        if ((room = account->room(uri.primaryId())))
            break;
        return UnknownMatrixId;
    case MatrixUri::RoomAlias:
        if ((room = account->roomByAlias(uri.primaryId())))
            break;
        [[fallthrough]];
    default:
        return UnknownMatrixId;
    }
    roomEventHandler(room, uri.secondaryId());
    return UriResolved;
}

UriResolveResult
ResourceResolver::openResource(Connection* account, const QString& identifier,
                               const QString& action)
{
    return openResource(account, MatrixUri(identifier), action);
}

UriResolveResult ResourceResolver::openResource(Connection* account,
                                                const MatrixUri& uri,
                                                const QString& overrideAction)
{
    return visitResource(
        account, uri,
        [this, &overrideAction](User* u) { emit userAction(u, overrideAction); },
        [this, &overrideAction](Room* room, const QString& eventId) {
            emit roomAction(room, eventId, overrideAction);
        },
        [this](Connection* account, const QString& roomAliasOrId,
               const QStringList& viaServers) {
            emit joinAction(account, roomAliasOrId, viaServers);
        });
}
