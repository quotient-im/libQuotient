// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#pragma once

#include <Quotient/converters.h>

namespace Quotient {

struct QUOTIENT_API MsisdnValidationData {
    //! A unique string generated by the client, and used to identify the
    //! validation attempt. It must be a string consisting of the characters
    //! `[0-9a-zA-Z.=_-]`. Its length must not exceed 255 characters and it
    //! must not be empty.
    QString clientSecret;

    //! The two-letter uppercase ISO-3166-1 alpha-2 country code that the
    //! number in `phone_number` should be parsed as if it were dialled from.
    QString country;

    //! The phone number to validate.
    QString phoneNumber;

    //! The server will only send an SMS if the `send_attempt` is a
    //! number greater than the most recent one which it has seen,
    //! scoped to that `country` + `phone_number` + `client_secret`
    //! triple. This is to avoid repeatedly sending the same SMS in
    //! the case of request retries between the POSTing user and the
    //! identity server. The client should increment this value if
    //! they desire a new SMS (e.g. a reminder) to be sent.
    int sendAttempt;

    //! Optional. When the validation is completed, the identity server will
    //! redirect the user to this URL. This option is ignored when submitting
    //! 3PID validation information through a POST request.
    QString nextLink{};

    //! The hostname of the identity server to communicate with. May optionally
    //! include a port. This parameter is ignored when the homeserver handles
    //! 3PID verification.
    //!
    //! This parameter is deprecated with a plan to be removed in a future specification
    //! version for `/account/password` and `/register` requests.
    QString idServer{};

    //! An access token previously registered with the identity server. Servers
    //! can treat this as optional to distinguish between r0.5-compatible clients
    //! and this specification version.
    //!
    //! Required if an `id_server` is supplied.
    QString idAccessToken{};
};

template <>
struct JsonObjectConverter<MsisdnValidationData> {
    static void dumpTo(QJsonObject& jo, const MsisdnValidationData& pod)
    {
        addParam<>(jo, "client_secret"_L1, pod.clientSecret);
        addParam<>(jo, "country"_L1, pod.country);
        addParam<>(jo, "phone_number"_L1, pod.phoneNumber);
        addParam<>(jo, "send_attempt"_L1, pod.sendAttempt);
        addParam<IfNotEmpty>(jo, "next_link"_L1, pod.nextLink);
        addParam<IfNotEmpty>(jo, "id_server"_L1, pod.idServer);
        addParam<IfNotEmpty>(jo, "id_access_token"_L1, pod.idAccessToken);
    }
    static void fillFrom(const QJsonObject& jo, MsisdnValidationData& pod)
    {
        fillFromJson(jo.value("client_secret"_L1), pod.clientSecret);
        fillFromJson(jo.value("country"_L1), pod.country);
        fillFromJson(jo.value("phone_number"_L1), pod.phoneNumber);
        fillFromJson(jo.value("send_attempt"_L1), pod.sendAttempt);
        fillFromJson(jo.value("next_link"_L1), pod.nextLink);
        fillFromJson(jo.value("id_server"_L1), pod.idServer);
        fillFromJson(jo.value("id_access_token"_L1), pod.idAccessToken);
    }
};

} // namespace Quotient
