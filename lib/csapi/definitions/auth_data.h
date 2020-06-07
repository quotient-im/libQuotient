/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "converters.h"

namespace Quotient {
/// Used by clients to submit authentication information to the
/// interactive-authentication API
struct AuthenticationData {
    /// The login type that the client is attempting to complete.
    QString type;

    /// The value of the session key given by the homeserver.
    QString session;

    /// Keys dependent on the login type
    QHash<QString, QJsonObject> authInfo;
};

template <>
struct JsonObjectConverter<AuthenticationData> {
    static void dumpTo(QJsonObject& jo, const AuthenticationData& pod)
    {
        fillJson(jo, pod.authInfo);
        addParam<>(jo, QStringLiteral("type"), pod.type);
        addParam<IfNotEmpty>(jo, QStringLiteral("session"), pod.session);
    }
    static void fillFrom(QJsonObject jo, AuthenticationData& pod)
    {
        fromJson(jo.take("type"_ls), pod.type);
        fromJson(jo.take("session"_ls), pod.session);
        fromJson(jo, pod.authInfo);
    }
};

} // namespace Quotient
