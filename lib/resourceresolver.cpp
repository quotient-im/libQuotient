#include "resourceresolver.h"

#include "settings.h"

#include <QtCore/QRegularExpression>

using namespace Quotient;

QString ResourceResolver::toMatrixId(const QString& uriOrId,
                                     QStringList uriServers)
{
    auto id = QUrl::fromPercentEncoding(uriOrId.toUtf8());
    const auto MatrixScheme = "matrix:"_ls;
    if (id.startsWith(MatrixScheme)) {
        id.remove(0, MatrixScheme.size());
        for (const auto& p: { std::pair { "user/"_ls, '@' },
                              { "roomid/"_ls, '!' },
                              { "room/"_ls, '#' } })
            if (id.startsWith(p.first)) {
                id.replace(0, p.first.size(), p.second);
                break;
            }
        // The below assumes that /event/ cannot show up in normal Matrix ids.
        id.replace("/event/"_ls, "/$"_ls);
    } else {
        const auto MatrixTo_ServerName = QStringLiteral("matrix.to");
        if (!uriServers.contains(MatrixTo_ServerName))
            uriServers.push_back(MatrixTo_ServerName);
        id.remove(
            QRegularExpression("^https://(" + uriServers.join('|') + ")/?#/"));
    }
    return id;
}

ResourceResolver::Result ResourceResolver::visitResource(
    Connection* account, const QString& identifier,
    std::function<void(User*)> userHandler,
    std::function<void(Room*, QString)> roomEventHandler)
{
    const auto& normalizedId = toMatrixId(identifier);
    auto&& [sigil, mainId, secondaryId] = parseIdentifier(normalizedId);
            Room* room = nullptr;
            switch (sigil) {
        case char(-1):
            return MalformedMatrixId;
        case char(0):
            return EmptyMatrixId;
        case '@':
            if (auto* user = account->user(mainId)) {
                userHandler(user);
                return Success;
            }
            return MalformedMatrixId;
        case '!':
            if ((room = account->room(mainId)))
                break;
            return UnknownMatrixId;
        case '#':
            if ((room = account->roomByAlias(mainId)))
                break;
            [[fallthrough]];
        default:
            return UnknownMatrixId;
    }
    roomEventHandler(room, secondaryId);
    return Success;
}

ResourceResolver::IdentifierParts
ResourceResolver::parseIdentifier(const QString& identifier)
{
    if (identifier.isEmpty())
        return {};
    
    // The regex is quick and dirty, only intending to triage the id.
    static const QRegularExpression IdRE {
        "^(?<main>(?<sigil>.)([^/]+))(/(?<sec>[^?]+))?"
    };
    auto dissectedId = IdRE.match(identifier);
    if (!dissectedId.hasMatch())
        return { char(-1) };

    const auto sigil = dissectedId.captured("sigil");
    return { sigil.size() != 1 ? char(-1) : sigil.front().toLatin1(),
                dissectedId.captured("main"), dissectedId.captured("sec") };
}

ResourceResolver::Result
ResourceResolver::openResource(Connection* account, const QString& identifier,
                               const QString& action)
{
    return visitResource(account, identifier,
        [this, &action](User* u) { emit userAction(u, action); },
        [this, &action](Room* room, const QString& eventId) {
            emit roomAction(room, eventId, action);
        });
}
