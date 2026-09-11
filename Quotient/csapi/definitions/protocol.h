// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#pragma once

#include <Quotient/application-service/definitions/protocol_base.h>
#include <Quotient/converters.h>

namespace Quotient {

struct QUOTIENT_API ProtocolInstance
{
    //! A human-readable description for the protocol, such as the name.
    QString desc;

    //! Preset values for `fields` the client may use to search by.
    QJsonObject fields;

    //! A unique identifier across all instances.
    QString networkId;

    //! An optional content URI representing the protocol. Overrides the one provided
    //! at the higher level Protocol object.
    QString icon{};

    //! A unique identifier for this instance on the homeserver. This field is added
    //! to the response of [`GET
    //! /_matrix/app/v1/thirdparty/protocol/{protocol}`](/application-service-api/#get_matrixappv1thirdpartyprotocolprotocol)
    //! by the homeserver.
    //!
    //! This is the identifier to use as the `third_party_instance_id` in a request to
    //! [`POST /_matrix/client/v3/publicRooms`](/client-server-api/#post_matrixclientv3publicrooms).
    QString instanceId{};
};

template <>
struct JsonObjectConverter<ProtocolInstance>
{
    static void dumpTo(QJsonObject &jo, const ProtocolInstance &pod)
    {
        addParam(jo, "desc"_L1, pod.desc);
        addParam(jo, "fields"_L1, pod.fields);
        addParam(jo, "network_id"_L1, pod.networkId);
        addParam<IfNotEmpty>(jo, "icon"_L1, pod.icon);
        addParam<IfNotEmpty>(jo, "instance_id"_L1, pod.instanceId);
    }
    static void fillFrom(const QJsonObject &jo, ProtocolInstance &pod)
    {
        fillFromJson(jo.value("desc"_L1), pod.desc);
        fillFromJson(jo.value("fields"_L1), pod.fields);
        fillFromJson(jo.value("network_id"_L1), pod.networkId);
        fillFromJson(jo.value("icon"_L1), pod.icon);
        fillFromJson(jo.value("instance_id"_L1), pod.instanceId);
    }
};

struct QUOTIENT_API ThirdPartyProtocol
{
    //! Fields which may be used to identify a third-party user. These should be
    //! ordered to suggest the way that entities may be grouped, where higher
    //! groupings are ordered first. For example, the name of a network should be
    //! searched before the nickname of a user.
    QStringList userFields;

    //! Fields which may be used to identify a third-party location. These should be
    //! ordered to suggest the way that entities may be grouped, where higher
    //! groupings are ordered first. For example, the name of a network should be
    //! searched before the name of a channel.
    QStringList locationFields;

    //! A content URI representing an icon for the third-party protocol.
    QString icon;

    //! The type definitions for the fields defined in `user_fields` and
    //! `location_fields`. Each entry in those arrays MUST have an entry here.
    //! The `string` key for this object is the field name itself.
    //!
    //! May be an empty object if no fields are defined.
    QHash<QString, FieldType> fieldTypes;

    //! A list of objects representing independent instances of configuration.
    //! For example, multiple networks on IRC if multiple are provided by the
    //! same application service.
    //!
    //! The instances are modified by the homeserver from the response of
    //! [`GET
    //! /_matrix/app/v1/thirdparty/protocol/{protocol}`](/application-service-api/#get_matrixappv1thirdpartyprotocolprotocol)
    //! to include an `instance_id` to serve as a unique identifier for each
    //! instance on the homeserver.
    QVector<ProtocolInstance> instances{};
};

template <>
struct JsonObjectConverter<ThirdPartyProtocol>
{
    static void dumpTo(QJsonObject &jo, const ThirdPartyProtocol &pod)
    {
        addParam(jo, "user_fields"_L1, pod.userFields);
        addParam(jo, "location_fields"_L1, pod.locationFields);
        addParam(jo, "icon"_L1, pod.icon);
        addParam(jo, "field_types"_L1, pod.fieldTypes);
        addParam<IfNotEmpty>(jo, "instances"_L1, pod.instances);
    }
    static void fillFrom(const QJsonObject &jo, ThirdPartyProtocol &pod)
    {
        fillFromJson(jo.value("user_fields"_L1), pod.userFields);
        fillFromJson(jo.value("location_fields"_L1), pod.locationFields);
        fillFromJson(jo.value("icon"_L1), pod.icon);
        fillFromJson(jo.value("field_types"_L1), pod.fieldTypes);
        fillFromJson(jo.value("instances"_L1), pod.instances);
    }
};

} // namespace Quotient
