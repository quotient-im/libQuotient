/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "converters.h"

namespace Quotient {

struct RequestTokenResponse {
    /// The session ID. Session IDs are opaque strings that must consist
    /// entirely of the characters `[0-9a-zA-Z.=_-]`. Their length must not
    /// exceed 255 characters and they must not be empty.
    QString sid;

    /// An optional field containing a URL where the client must submit the
    /// validation token to, with identical parameters to the Identity Service
    /// API's `POST /validate/email/submitToken` endpoint (without the
    /// requirement for an access token). The homeserver must send this token to
    /// the user (if applicable), who should then be prompted to provide it to
    /// the client.
    ///
    /// If this field is not present, the client can assume that verification
    /// will happen without the client's involvement provided the homeserver
    /// advertises this specification version in the `/versions` response
    /// (ie: r0.5.0).
    QUrl submitUrl;
};

template <>
struct JsonObjectConverter<RequestTokenResponse> {
    static void dumpTo(QJsonObject& jo, const RequestTokenResponse& pod)
    {
        addParam<>(jo, QStringLiteral("sid"), pod.sid);
        addParam<IfNotEmpty>(jo, QStringLiteral("submit_url"), pod.submitUrl);
    }
    static void fillFrom(const QJsonObject& jo, RequestTokenResponse& pod)
    {
        fromJson(jo.value("sid"_ls), pod.sid);
        fromJson(jo.value("submit_url"_ls), pod.submitUrl);
    }
};

} // namespace Quotient
