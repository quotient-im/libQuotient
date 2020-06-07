/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "converters.h"

namespace Quotient {

struct OpenidToken {
    /// An access token the consumer may use to verify the identity of
    /// the person who generated the token. This is given to the federation
    /// API ``GET /openid/userinfo`` to verify the user's identity.
    QString accessToken;

    /// The string ``Bearer``.
    QString tokenType;

    /// The homeserver domain the consumer should use when attempting to
    /// verify the user's identity.
    QString matrixServerName;

    /// The number of seconds before this token expires and a new one must
    /// be generated.
    int expiresIn;
};

template <>
struct JsonObjectConverter<OpenidToken> {
    static void dumpTo(QJsonObject& jo, const OpenidToken& pod)
    {
        addParam<>(jo, QStringLiteral("access_token"), pod.accessToken);
        addParam<>(jo, QStringLiteral("token_type"), pod.tokenType);
        addParam<>(jo, QStringLiteral("matrix_server_name"),
                   pod.matrixServerName);
        addParam<>(jo, QStringLiteral("expires_in"), pod.expiresIn);
    }
    static void fillFrom(const QJsonObject& jo, OpenidToken& pod)
    {
        fromJson(jo.value("access_token"_ls), pod.accessToken);
        fromJson(jo.value("token_type"_ls), pod.tokenType);
        fromJson(jo.value("matrix_server_name"_ls), pod.matrixServerName);
        fromJson(jo.value("expires_in"_ls), pod.expiresIn);
    }
};

} // namespace Quotient
