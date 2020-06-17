#pragma once

#include "quotient_common.h"

#include <QtCore/QUrl>
#include <QtCore/QUrlQuery>
#include <QtCore/QObject>

#include <functional>

namespace Quotient {
class Connection;
class Room;
class User;

/*! \brief A wrapper around a Matrix URI or identifier
 *
 * This class encapsulates a Matrix resource identifier, passed in either of
 * 3 forms: a plain Matrix identifier (sigil, localpart, serverpart or, for
 * modern event ids, sigil and base64 hash); an MSC2312 URI (aka matrix: URI);
 * or a matrix.to URL. The input can be either encoded (serverparts with
 * punycode, the rest with percent-encoding) or unencoded (in this case it is
 * the caller's responsibility to resolve all possible ambiguities).
 *
 * The class provides functions to check the validity of the identifier,
 * its type, and obtain components, also in either unencoded (for displaying)
 * or encoded (for APIs) form.
 */
class MatrixUri : private QUrl {
    Q_GADGET
    Q_PROPERTY(QString primaryId READ primaryId CONSTANT)
    Q_PROPERTY(QString secondaryId READ secondaryId CONSTANT)
//    Q_PROPERTY(QUrlQuery query READ query CONSTANT)
    Q_PROPERTY(QString action READ action CONSTANT)
//    Q_PROPERTY(QStringList viaServers READ viaServers CONSTANT)
public:
    enum Type : char {
        Invalid = char(-1),
        Empty = 0x0,
        UserId = '@',
        RoomId = '!',
        RoomAlias = '#',
        Group = '+'
    };
    Q_ENUM(Type)
    enum SecondaryType : char {
        NoSecondaryId = 0x0,
        EventId = '$'
    };

    enum UriForm : short { CanonicalUri, MatrixToUri };
    Q_ENUM(UriForm)

    /// Construct an empty Matrix URI
    MatrixUri() = default;
    /*! \brief Decode a user input string to a Matrix identifier
     *
     * Accepts plain Matrix ids, MSC2312 URIs (aka matrix: URIs) and
     * matrix.to URLs. In case of URIs/URLs, it uses QUrl's TolerantMode
     * parser to decode common mistakes/irregularities (see QUrl documentation
     * for more details).
     */
    MatrixUri(const QString& uriOrId);

    /// Construct a Matrix URI from components
    explicit MatrixUri(QByteArray primaryId, QByteArray secondaryId = {},
                       QString query = {});
    /// Construct a Matrix URI from matrix.to or MSC2312 (matrix:) URI
    explicit MatrixUri(QUrl url);

    static MatrixUri fromUserInput(const QString& uriOrId);
    static MatrixUri fromUrl(QUrl url);

    /// Get the primary type of the Matrix URI (user id, room id or alias)
    /*! Note that this does not include an event as a separate type, since
     * events can only be addressed inside of rooms, which, in turn, are
     * addressed either by id or alias. If you need to check whether the URI
     * is specifically an event URI, use secondaryType() instead.
     */
    Type type() const;
    SecondaryType secondaryType() const;
    QUrl toUrl(UriForm form = CanonicalUri) const;
    QString toDisplayString(UriForm form = CanonicalUri) const;
    QString primaryId() const;
    QString secondaryId() const;
    QString action() const;
    QStringList viaServers() const;
    bool isValid() const;
    using QUrl::isEmpty, QUrl::path, QUrl::query, QUrl::fragment;

private:

    Type primaryType_ = Empty;
};

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
 * \param uri The Matrix identifier or URI; MSC2312 URIs and classic Matrix IDs
 *            are supported
 *
 * \sa ResourceResolver
 */
UriResolveResult
visitResource(Connection* account, const MatrixUri& uri,
              std::function<void(User*)> userHandler,
              std::function<void(Room*, QString)> roomEventHandler,
              std::function<void(Connection*, QString, QStringList)> joinHandler);

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
    explicit ResourceResolver(QObject* parent = nullptr) : QObject(parent)
    { }

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
    Q_INVOKABLE UriResolveResult openResource(Connection* account,
                                                   const QString& identifier,
                                                   const QString& action = {});
    Q_INVOKABLE UriResolveResult
    openResource(Connection* account, const MatrixUri& uri,
                 const QString& overrideAction = {});

signals:
    /// An action on a user has been requested
    void userAction(Quotient::User* user, QString action);

    /// An action on a room has been requested, with optional event id
    void roomAction(Quotient::Room* room, QString eventId, QString action);

    /// A join action has been requested, with optional 'via' servers
    void joinAction(Quotient::Connection* account, QString roomAliasOrId,
                    QStringList viaServers);
};

} // namespace Quotient


