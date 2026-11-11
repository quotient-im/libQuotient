// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#pragma once

#include <Quotient/converters.h>

namespace Quotient {

struct QUOTIENT_API Invite3pid
{
    //! The hostname+port of the identity server which should be used for third-party identifier
    //! lookups.
    QString idServer;

    //! An access token previously registered with the identity server. Servers
    //! can treat this as optional to distinguish between r0.5-compatible clients
    //! and this specification version.
    QString idAccessToken;

    //! The kind of address being passed in the address field, for example `email`
    //! (see [the list of recognised values](/appendices/#3pid-types)).
    QString medium;

    //! The invitee's third-party identifier.
    QString address;
};

template <>
struct JsonObjectConverter<Invite3pid>
{
    static void dumpTo(QJsonObject &jo, const Invite3pid &pod)
    {
        addParam(jo, "id_server"_L1, pod.idServer);
        addParam(jo, "id_access_token"_L1, pod.idAccessToken);
        addParam(jo, "medium"_L1, pod.medium);
        addParam(jo, "address"_L1, pod.address);
    }
    static void fillFrom(const QJsonObject &jo, Invite3pid &pod)
    {
        fillFromJson(jo.value("id_server"_L1), pod.idServer);
        fillFromJson(jo.value("id_access_token"_L1), pod.idAccessToken);
        fillFromJson(jo.value("medium"_L1), pod.medium);
        fillFromJson(jo.value("address"_L1), pod.address);
    }
};

} // namespace Quotient
