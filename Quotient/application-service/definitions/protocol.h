/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include <Quotient/converters.h>

namespace Quotient {
/// Definition of valid values for a field.
struct FieldType {
    /// A regular expression for validation of a field's value. This may be
    /// relatively coarse to verify the value as the application service
    /// providing this protocol may apply additional validation or filtering.
    QString regexp;

    /// An placeholder serving as a valid example of the field value.
    QString placeholder;
};

template <>
struct JsonObjectConverter<FieldType> {
    static void dumpTo(QJsonObject& jo, const FieldType& pod)
    {
        addParam<>(jo, QStringLiteral("regexp"), pod.regexp);
        addParam<>(jo, QStringLiteral("placeholder"), pod.placeholder);
    }
    static void fillFrom(const QJsonObject& jo, FieldType& pod)
    {
        fromJson(jo.value("regexp"_ls), pod.regexp);
        fromJson(jo.value("placeholder"_ls), pod.placeholder);
    }
};

struct ProtocolInstance {
    /// A human-readable description for the protocol, such as the name.
    QString desc;

    /// An optional content URI representing the protocol. Overrides the one
    /// provided at the higher level Protocol object.
    QString icon;

    /// Preset values for `fields` the client may use to search by.
    QJsonObject fields;

    /// A unique identifier across all instances.
    QString networkId;
};

template <>
struct JsonObjectConverter<ProtocolInstance> {
    static void dumpTo(QJsonObject& jo, const ProtocolInstance& pod)
    {
        addParam<>(jo, QStringLiteral("desc"), pod.desc);
        addParam<IfNotEmpty>(jo, QStringLiteral("icon"), pod.icon);
        addParam<>(jo, QStringLiteral("fields"), pod.fields);
        addParam<>(jo, QStringLiteral("network_id"), pod.networkId);
    }
    static void fillFrom(const QJsonObject& jo, ProtocolInstance& pod)
    {
        fromJson(jo.value("desc"_ls), pod.desc);
        fromJson(jo.value("icon"_ls), pod.icon);
        fromJson(jo.value("fields"_ls), pod.fields);
        fromJson(jo.value("network_id"_ls), pod.networkId);
    }
};

struct ThirdPartyProtocol {
    /// Fields which may be used to identify a third-party user. These should be
    /// ordered to suggest the way that entities may be grouped, where higher
    /// groupings are ordered first. For example, the name of a network should
    /// be searched before the nickname of a user.
    QStringList userFields;

    /// Fields which may be used to identify a third-party location. These
    /// should be ordered to suggest the way that entities may be grouped, where
    /// higher groupings are ordered first. For example, the name of a network
    /// should be searched before the name of a channel.
    QStringList locationFields;

    /// A content URI representing an icon for the third-party protocol.
    QString icon;

    /// The type definitions for the fields defined in the `user_fields` and
    /// `location_fields`. Each entry in those arrays MUST have an entry here.
    /// The `string` key for this object is field name itself.
    ///
    /// May be an empty object if no fields are defined.
    QHash<QString, FieldType> fieldTypes;

    /// A list of objects representing independent instances of configuration.
    /// For example, multiple networks on IRC if multiple are provided by the
    /// same application service.
    QVector<ProtocolInstance> instances;
};

template <>
struct JsonObjectConverter<ThirdPartyProtocol> {
    static void dumpTo(QJsonObject& jo, const ThirdPartyProtocol& pod)
    {
        addParam<>(jo, QStringLiteral("user_fields"), pod.userFields);
        addParam<>(jo, QStringLiteral("location_fields"), pod.locationFields);
        addParam<>(jo, QStringLiteral("icon"), pod.icon);
        addParam<>(jo, QStringLiteral("field_types"), pod.fieldTypes);
        addParam<>(jo, QStringLiteral("instances"), pod.instances);
    }
    static void fillFrom(const QJsonObject& jo, ThirdPartyProtocol& pod)
    {
        fromJson(jo.value("user_fields"_ls), pod.userFields);
        fromJson(jo.value("location_fields"_ls), pod.locationFields);
        fromJson(jo.value("icon"_ls), pod.icon);
        fromJson(jo.value("field_types"_ls), pod.fieldTypes);
        fromJson(jo.value("instances"_ls), pod.instances);
    }
};

} // namespace Quotient
