/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "converters.h"

#include <QtCore/QJsonObject>
#include <QtCore/QHash>

namespace QMatrixClient
{
    // Data structures

    /// Used by clients to submit authentication information to the interactive-authentication API
    struct AuthenticationData
    {
        /// The login type that the client is attempting to complete.
        QString type;
        /// The value of the session key given by the homeserver.
        QString session;
        /// Keys dependent on the login type
        QHash<QString, QJsonObject> authInfo;
    };

    QJsonObject toJson(const AuthenticationData& pod);

    template <> struct FromJsonObject<AuthenticationData>
    {
        AuthenticationData operator()(QJsonObject jo) const;
    };

} // namespace QMatrixClient
