// SPDX-FileCopyrightText: 2020 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "uriresolver.h"

#include "connection.h"
#include "user.h"

using namespace Quotient;

UriResolverBase::~UriResolverBase() = default;

UriResolveResult UriResolverBase::visitResource(Connection* account,
                                                const Uri& uri)
{
    switch (uri.type()) {
    case Uri::NonMatrix:
        return visitNonMatrix(uri.toUrl()) ? UriResolved : CouldNotResolve;
    case Uri::Invalid:
    case Uri::Empty:
        return InvalidUri;
    default:;
    }

    if (!account)
        return NoAccount;

    switch (uri.type()) {
    case Uri::UserId: {
        if (uri.action() == "join"_ls)
            return IncorrectAction;
        auto* user = account->user(uri.primaryId());
        Q_ASSERT(user != nullptr);
        return visitUser(user, uri.action());
    }
    case Uri::RoomId:
    case Uri::RoomAlias: {
        auto* room = uri.type() == Uri::RoomId
                         ? account->room(uri.primaryId())
                         : account->roomByAlias(uri.primaryId());
        if (room != nullptr) {
            visitRoom(room, uri.secondaryId());
            return UriResolved;
        }
        if (uri.action() == "join"_ls) {
            joinRoom(account, uri.primaryId(), uri.viaServers());
            return UriResolved;
        }
        [[fallthrough]];
    }
    default:
        return CouldNotResolve;
    }
}

// This template is only instantiated once, for Quotient::visitResource()
template <typename... FnTs>
class StaticUriDispatcher : public UriResolverBase {
public:
    StaticUriDispatcher(const FnTs&... fns) : fns_(fns...) {}

private:
    UriResolveResult visitUser(User* user, const QString& action) override
    {
        return std::get<0>(fns_)(user, action);
    }
    void visitRoom(Room* room, const QString& eventId) override
    {
        std::get<1>(fns_)(room, eventId);
    }
    void joinRoom(Connection* account, const QString& roomAliasOrId,
                  const QStringList& viaServers = {}) override
    {
        std::get<2>(fns_)(account, roomAliasOrId, viaServers);
    }
    bool visitNonMatrix(const QUrl& url) override
    {
        return std::get<3>(fns_)(url);
    }

    std::tuple<FnTs...> fns_;
};
template <typename... FnTs>
StaticUriDispatcher(FnTs&&... fns) -> StaticUriDispatcher<FnTs...>;

UriResolveResult Quotient::visitResource(
    Connection* account, const Uri& uri,
    std::function<UriResolveResult(User*, QString)> userHandler,
    std::function<void(Room*, QString)> roomEventHandler,
    std::function<void(Connection*, QString, QStringList)> joinHandler,
    std::function<bool(const QUrl&)> nonMatrixHandler)
{
    return StaticUriDispatcher(userHandler, roomEventHandler, joinHandler,
                               nonMatrixHandler)
        .visitResource(account, uri);
}

UriResolveResult UriDispatcher::visitUser(User *user, const QString &action)
{
    emit userAction(user, action);
    return UriResolved;
}

void UriDispatcher::visitRoom(Room *room, const QString &eventId)
{
    emit roomAction(room, eventId);
}

void UriDispatcher::joinRoom(Connection* account, const QString& roomAliasOrId,
                             const QStringList& viaServers)
{
    emit joinAction(account, roomAliasOrId, viaServers);
}

bool UriDispatcher::visitNonMatrix(const QUrl &url)
{
    emit nonMatrixAction(url);
    return true;
}
