// SPDX-FileCopyrightText: 2020 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "quotient_common.h"

#include <QtCore/QUrl>
#include <QtCore/QUrlQuery>

namespace Quotient {

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
class QUOTIENT_API Uri : private QUrl {
    Q_GADGET
public:
    enum Type : char {
        Invalid = char(-1),
        Empty = 0x0,
        UserId = '@',
        RoomId = '!',
        RoomAlias = '#',
        Group = '+',
        BareEventId = '$', // https://github.com/matrix-org/matrix-doc/pull/2644
        NonMatrix = ':'
    };
    Q_ENUM(Type)
    enum SecondaryType : char { NoSecondaryId = 0x0, EventId = '$' };
    Q_ENUM(SecondaryType)

    enum UriForm : short { CanonicalUri, MatrixToUri };
    Q_ENUM(UriForm)

    /// Construct an empty Matrix URI
    Uri() = default;
    /*! \brief Decode a user input string to a Matrix identifier
     *
     * Accepts plain Matrix ids, MSC2312 URIs (aka matrix: URIs) and
     * matrix.to URLs. In case of URIs/URLs, it uses QUrl's TolerantMode
     * parser to decode common mistakes/irregularities (see QUrl documentation
     * for more details).
     */
    Uri(const QString& uriOrId);

    /// Construct a Matrix URI from components
    explicit Uri(QByteArray primaryId, QByteArray secondaryId = {},
                 QString query = {});
    /// Construct a Matrix URI from matrix.to or MSC2312 (matrix:) URI
    explicit Uri(QUrl url);

    static Uri fromUserInput(const QString& uriOrId);
    static Uri fromUrl(QUrl url);

    /// Get the primary type of the Matrix URI (user id, room id or alias)
    /*! Note that this does not include an event as a separate type, since
     * events can only be addressed inside of rooms, which, in turn, are
     * addressed either by id or alias. If you need to check whether the URI
     * is specifically an event URI, use secondaryType() instead.
     */
    Q_INVOKABLE Type type() const;
    Q_INVOKABLE SecondaryType secondaryType() const;
    Q_INVOKABLE QUrl toUrl(UriForm form = CanonicalUri) const;
    Q_INVOKABLE QString primaryId() const;
    Q_INVOKABLE QString secondaryId() const;
    Q_INVOKABLE QString action() const;
    Q_INVOKABLE void setAction(const QString& newAction);
    Q_INVOKABLE QStringList viaServers() const;
    Q_INVOKABLE bool isValid() const;
    using QUrl::path, QUrl::query, QUrl::fragment;
    using QUrl::isEmpty, QUrl::toDisplayString;

private:
    Type primaryType_ = Empty;
};

}
