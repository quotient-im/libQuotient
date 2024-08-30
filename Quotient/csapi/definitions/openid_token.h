// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#pragma once

#include <Quotient/converters.h>

namespace Quotient {

struct QUOTIENT_API OpenIdCredentials {
    //! An access token the consumer may use to verify the identity of
    //! the person who generated the token. This is given to the federation
    //! API `GET /openid/userinfo` to verify the user's identity.
    QString accessToken;

    //! The string `Bearer`.
    QString tokenType;

    //! The homeserver domain the consumer should use when attempting to
    //! verify the user's identity.
    QString matrixServerName;

    //! The number of seconds before this token expires and a new one must
    //! be generated.
    int expiresIn;
};

template <>
struct JsonObjectConverter<OpenIdCredentials> {
    static void dumpTo(QJsonObject& jo, const OpenIdCredentials& pod)
    {
        addParam<>(jo, "access_token"_L1, pod.accessToken);
        addParam<>(jo, "token_type"_L1, pod.tokenType);
        addParam<>(jo, "matrix_server_name"_L1, pod.matrixServerName);
        addParam<>(jo, "expires_in"_L1, pod.expiresIn);
    }
    static void fillFrom(const QJsonObject& jo, OpenIdCredentials& pod)
    {
        fillFromJson(jo.value("access_token"_L1), pod.accessToken);
        fillFromJson(jo.value("token_type"_L1), pod.tokenType);
        fillFromJson(jo.value("matrix_server_name"_L1), pod.matrixServerName);
        fillFromJson(jo.value("expires_in"_L1), pod.expiresIn);
    }
};

} // namespace Quotient
