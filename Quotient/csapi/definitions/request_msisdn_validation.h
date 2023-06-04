/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include <Quotient/converters.h>
#include <Quotient/identity/definitions/request_msisdn_validation.h>

namespace Quotient {

struct MsisdnValidationData : RequestMsisdnValidation {
    /// The hostname of the identity server to communicate with. May optionally
    /// include a port. This parameter is ignored when the homeserver handles
    /// 3PID verification.
    ///
    /// This parameter is deprecated with a plan to be removed in a future
    /// specification version for `/account/password` and `/register` requests.
    QString idServer;

    /// An access token previously registered with the identity server. Servers
    /// can treat this as optional to distinguish between r0.5-compatible
    /// clients and this specification version.
    ///
    /// Required if an `id_server` is supplied.
    QString idAccessToken;
};

template <>
struct JsonObjectConverter<MsisdnValidationData> {
    static void dumpTo(QJsonObject& jo, const MsisdnValidationData& pod)
    {
        fillJson<RequestMsisdnValidation>(jo, pod);
        addParam<IfNotEmpty>(jo, QStringLiteral("id_server"), pod.idServer);
        addParam<IfNotEmpty>(jo, QStringLiteral("id_access_token"),
                             pod.idAccessToken);
    }
    static void fillFrom(const QJsonObject& jo, MsisdnValidationData& pod)
    {
        fillFromJson<RequestMsisdnValidation>(jo, pod);
        fromJson(jo.value("id_server"_ls), pod.idServer);
        fromJson(jo.value("id_access_token"_ls), pod.idAccessToken);
    }
};

} // namespace Quotient
