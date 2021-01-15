#pragma once

#include "uri.h"

#include <QtCore/QObject>

#include <functional>

namespace Quotient {
class Connection;
class Room;
class User;

/*! \brief Abstract class to resolve the resource and act on it
 *
 * This class encapsulates the logic of resolving a Matrix identifier or URI
 * into a Quotient object (or objects) and calling an appropriate handler on it.
 * It is a type-safe way of handling a URI with no prior context on its type
 * in cases like, e.g., when a user clicks on a URI in the application.
 *
 * This class provides empty "handlers" for each type of URI to facilitate
 * gradual implementation. Derived classes are encouraged to override as many
 * of them as possible.
 */
class UriResolverBase {
public:
    /*! \brief Resolve the resource and dispatch an action depending on its type
     *
     * This method:
     * 1. Resolves \p uri into an actual object (e.g., Room or User),
     *    with possible additional data such as event id, in the context of
     *    \p account.
     * 2. If the resolving is successful, depending on the type of the object,
     *    calls the appropriate virtual function (defined in a derived
     *    concrete class) to perform an action on the resource (open a room,
     *    mention a user etc.).
     * 3. Returns the result of resolving the resource.
     */
    UriResolveResult visitResource(Connection* account, const Uri& uri);

protected:
    /// Called by visitResource() when the passed URI identifies a Matrix user
    /*!
     * \return IncorrectAction if the action is not correct or not supported;
     *         UriResolved if it is accepted; other values are disallowed
     */
    virtual UriResolveResult visitUser(User* user, const QString& action)
    {
        return IncorrectAction;
    }
    /// Called by visitResource() when the passed URI identifies a room or
    /// an event in a room
    virtual void visitRoom(Room* room, const QString& eventId) {}
    /// Called by visitResource() when the passed URI has `action() == "join"`
    /// and identifies a room that the user defined by the Connection argument
    /// is not a member of
    virtual void joinRoom(Connection* account, const QString& roomAliasOrId,
                          const QStringList& viaServers = {})
    {}
    /// Called by visitResource() when the passed URI has `type() == NonMatrix`
    /*!
     * Should return true if the URI is considered resolved, false otherwise.
     * A basic implementation in a graphical client can look like
     * `return QDesktopServices::openUrl(url);` but it's strongly advised to
     * ask for a user confirmation beforehand.
     */
    virtual bool visitNonMatrix(const QUrl& url) { return false; }
};

/*! \brief Resolve the resource and invoke an action on it, via function objects
 *
 * This function encapsulates the logic of resolving a Matrix identifier or URI
 * into a Quotient object (or objects) and calling an appropriate handler on it.
 * Unlike UriResolverBase it accepts the list of handlers from
 * the caller; internally it's uses a minimal UriResolverBase class
 *
 * \param account The connection used as a context to resolve the identifier
 *
 * \param uri A URI that can represent a Matrix entity
 *
 * \param userHandler Called when the passed URI identifies a Matrix user
 *
 * \param roomEventHandler Called when the passed URI identifies a room or
 *                         an event in a room
 *
 * \param joinHandler Called when the passed URI has `action() == "join"` and
 *                    identifies a room that the user defined by
 *                    the Connection argument is not a member of
 *
 * \param nonMatrixHandler Called when the passed URI has `type() == NonMatrix`;
 *                         should return true if the URI is considered resolved,
 *                         false otherwise
 *
 * \sa UriResolverBase, UriDispatcher
 */
UriResolveResult
visitResource(Connection* account, const Uri& uri,
              std::function<UriResolveResult(User*, QString)> userHandler,
              std::function<void(Room*, QString)> roomEventHandler,
              std::function<void(Connection*, QString, QStringList)> joinHandler,
              std::function<bool(const QUrl&)> nonMatrixHandler);

/*! \brief Check that the resource is resolvable with no action on it */
inline UriResolveResult checkResource(Connection* account, const Uri& uri)
{
    return visitResource(
        account, uri, [](auto, auto) { return UriResolved; }, [](auto, auto) {},
        [](auto, auto, auto) {}, [](auto) { return false; });
}

/*! \brief Resolve the resource and invoke an action on it, via Qt signals
 *
 * This is an implementation of UriResolverBase that is based on
 * QObject and uses Qt signals instead of virtual functions to provide an
 * open-ended interface for visitors.
 *
 * This class is aimed primarily at clients where invoking the resolving/action
 * and handling the action are happening in decoupled parts of the code; it's
 * also useful to operate on Matrix identifiers and URIs from QML/JS code
 * that cannot call resolveResource() due to QML/C++ interface limitations.
 *
 * This class does not restrain the client code to a certain type of
 * connections: both direct and queued (or a mix) will work fine. One limitation
 * caused by that is there's no way to indicate if a non-Matrix URI has been
 * successfully resolved - a signal always returns void.
 *
 * Note that in case of using (non-blocking) queued connections the code that
 * calls resolveResource() should not expect the action to be performed
 * synchronously - the returned value is the result of resolving the URI,
 * not acting on it.
 */
class UriDispatcher : public QObject, public UriResolverBase {
    Q_OBJECT
public:
    explicit UriDispatcher(QObject* parent = nullptr) : QObject(parent) {}

    // It's actually UriResolverBase::visitResource() but with Q_INVOKABLE
    Q_INVOKABLE UriResolveResult resolveResource(Connection* account,
                                                 const Uri& uri)
    {
        return UriResolverBase::visitResource(account, uri);
    }

Q_SIGNALS:
    /// An action on a user has been requested
    void userAction(Quotient::User* user, QString action);

    /// An action on a room has been requested, with optional event id
    void roomAction(Quotient::Room* room, QString eventId);

    /// A join action has been requested, with optional 'via' servers
    void joinAction(Quotient::Connection* account, QString roomAliasOrId,
                    QStringList viaServers);

    /// An action on a non-Matrix URL has been requested
    void nonMatrixAction(QUrl url);

private:
    UriResolveResult visitUser(User* user, const QString& action) override;
    void visitRoom(Room* room, const QString& eventId) override;
    void joinRoom(Connection* account, const QString& roomAliasOrId,
                  const QStringList& viaServers = {}) override;
    bool visitNonMatrix(const QUrl& url) override;
};

} // namespace Quotient


