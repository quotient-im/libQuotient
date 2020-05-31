#pragma once

#include "connection.h"

#include <functional>

namespace Quotient {

/*! \brief Matrix resource resolver
 * TODO: rewrite
 * Similar to visitResource(), this class encapsulates the logic of resolving
 * a Matrix identifier or a URI into Quotient object(s) and applying an action
 * to the resolved object(s). Instead of using a C++ visitor pattern, it
 * announces the request through Qt's signals passing the resolved object(s)
 * through those (still in a typesafe way).
 *
 * This class is aimed primarily at clients where invoking the resolving/action
 * and handling the action are happening in decoupled parts of the code; it's
 * also useful to operate on Matrix identifiers and URIs from QML/JS code
 * that cannot call visitResource due to QML/C++ interface limitations.
 */
class ResourceResolver : public QObject {
    Q_OBJECT
public:
    enum Result : short {
        StillResolving = -1,
        Success = 0,
        UnknownMatrixId,
        MalformedMatrixId,
        NoAccount,
        EmptyMatrixId
    };
    Q_ENUM(Result)

    explicit ResourceResolver(QObject* parent = nullptr) : QObject(parent)
    { }

    /*! \brief Decode a URI to a Matrix identifier (or a room/event pair)
     *
     * This accepts plain Matrix ids, MSC2312 URIs (aka matrix: URIs) and
     * matrix.to URIs.
     *
     * \return a Matrix identifier as defined by the common identifier grammars
     *         or a slash separated pair of Matrix identifiers if the original
     *         uri/id pointed to an event in a room
     */
    static QString toMatrixId(const QString& uriOrId,
                                 QStringList uriServers = {});

    /*! \brief Resolve the resource and invoke an action on it, visitor style
     *
     * This template function encapsulates the logic of resolving a Matrix
     * identifier or URI into a Quotient object (or objects) and applying an
     * appropriate action handler from the set provided by the caller to it.
     * A typical use case for that is opening a room or mentioning a user in
     * response to clicking on a Matrix URI or identifier.
     *
     * \param account The connection used as a context to resolve the identifier
     *
     * \param identifier The Matrix identifier or URI. MSC2312 URIs and classic
     *                   Matrix ID scheme are supported.
     *
     * \sa ResourceResolver
     */
    static Result
    visitResource(Connection* account, const QString& identifier,
                  std::function<void(User*)> userHandler,
                  std::function<void(Room*, QString)> roomEventHandler);

    /*! \brief Resolve the resource and request an action on it, signal style
     *
     * This method:
     * 1. Resolves \p identifier into an actual object (Room or User), with
     *    possible additional data such as event id, in the context of
     *    \p account.
     * 2. If the resolving is successful, depending on the type of the object,
     *    emits the respective signal to which the client must connect in order
     *    to apply the action to the resource (open a room, mention a user etc.).
     * 3. Returns the result of resolving the resource.
     *
     * Note that the action can be applied either synchronously or entirely
     * asynchronously; ResourceResolver does not restrain the client code
     * to use either method. The resource resolving part is entirely synchronous
     * though. If the synchronous operation is chosen, only
     * direct connections to ResourceResolver signals must be used, and
     * the caller should check the future's state immediately after calling
     * openResource() to process any feedback from the resolver and/or action
     * handler. If asynchronous operation is needed then either direct or queued
     * connections to ResourceResolver's signals can be used and the caller
     * must both check the ResourceFuture state right after calling openResource
     * and also connect to ResourceFuture::ready() signal in order to process
     * the result of resolving and action.
     */
    Q_INVOKABLE Result openResource(Connection* account,
                                    const QString& identifier,
                                    const QString& action = {});

signals:
    /// An action on a user has been requested
    void userAction(Quotient::User* user, QString action);

    /// An action on a room has been requested, with optional event id
    void roomAction(Quotient::Room* room, QString eventId, QString action);

private:
    struct IdentifierParts {
        char sigil;
        QString mainId {};
        QString secondaryId = {};
    };

    static IdentifierParts parseIdentifier(const QString& identifier);
};

} // namespace Quotient


