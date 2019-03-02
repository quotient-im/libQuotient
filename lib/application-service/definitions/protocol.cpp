/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "protocol.h"

using namespace QMatrixClient;

void JsonObjectConverter<FieldType>::dumpTo(QJsonObject& jo,
                                            const FieldType& pod)
{
    addParam<>(jo, QStringLiteral("regexp"), pod.regexp);
    addParam<>(jo, QStringLiteral("placeholder"), pod.placeholder);
}

void JsonObjectConverter<FieldType>::fillFrom(const QJsonObject& jo,
                                              FieldType& result)
{
    fromJson(jo.value("regexp"_ls), result.regexp);
    fromJson(jo.value("placeholder"_ls), result.placeholder);
}

void JsonObjectConverter<ProtocolInstance>::dumpTo(QJsonObject& jo,
                                                   const ProtocolInstance& pod)
{
    addParam<>(jo, QStringLiteral("desc"), pod.desc);
    addParam<IfNotEmpty>(jo, QStringLiteral("icon"), pod.icon);
    addParam<>(jo, QStringLiteral("fields"), pod.fields);
    addParam<>(jo, QStringLiteral("network_id"), pod.networkId);
}

void JsonObjectConverter<ProtocolInstance>::fillFrom(const QJsonObject& jo,
                                                     ProtocolInstance& result)
{
    fromJson(jo.value("desc"_ls), result.desc);
    fromJson(jo.value("icon"_ls), result.icon);
    fromJson(jo.value("fields"_ls), result.fields);
    fromJson(jo.value("network_id"_ls), result.networkId);
}

void JsonObjectConverter<ThirdPartyProtocol>::dumpTo(
        QJsonObject& jo, const ThirdPartyProtocol& pod)
{
    addParam<>(jo, QStringLiteral("user_fields"), pod.userFields);
    addParam<>(jo, QStringLiteral("location_fields"), pod.locationFields);
    addParam<>(jo, QStringLiteral("icon"), pod.icon);
    addParam<>(jo, QStringLiteral("field_types"), pod.fieldTypes);
    addParam<>(jo, QStringLiteral("instances"), pod.instances);
}

void JsonObjectConverter<ThirdPartyProtocol>::fillFrom(
        const QJsonObject& jo, ThirdPartyProtocol& result)
{
    fromJson(jo.value("user_fields"_ls), result.userFields);
    fromJson(jo.value("location_fields"_ls), result.locationFields);
    fromJson(jo.value("icon"_ls), result.icon);
    fromJson(jo.value("field_types"_ls), result.fieldTypes);
    fromJson(jo.value("instances"_ls), result.instances);
}
