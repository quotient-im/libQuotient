/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "protocol.h"

using namespace QMatrixClient;

QJsonObject QMatrixClient::toJson(const FieldType& pod)
{
    QJsonObject jo;
    addParam<>(jo, QStringLiteral("regexp"), pod.regexp);
    addParam<>(jo, QStringLiteral("placeholder"), pod.placeholder);
    return jo;
}

FieldType FromJsonObject<FieldType>::operator()(const QJsonObject& jo) const
{
    FieldType result;
    result.regexp =
        fromJson<QString>(jo.value("regexp"_ls));
    result.placeholder =
        fromJson<QString>(jo.value("placeholder"_ls));

    return result;
}

QJsonObject QMatrixClient::toJson(const ProtocolInstance& pod)
{
    QJsonObject jo;
    addParam<>(jo, QStringLiteral("desc"), pod.desc);
    addParam<IfNotEmpty>(jo, QStringLiteral("icon"), pod.icon);
    addParam<>(jo, QStringLiteral("fields"), pod.fields);
    addParam<>(jo, QStringLiteral("network_id"), pod.networkId);
    return jo;
}

ProtocolInstance FromJsonObject<ProtocolInstance>::operator()(const QJsonObject& jo) const
{
    ProtocolInstance result;
    result.desc =
        fromJson<QString>(jo.value("desc"_ls));
    result.icon =
        fromJson<QString>(jo.value("icon"_ls));
    result.fields =
        fromJson<QJsonObject>(jo.value("fields"_ls));
    result.networkId =
        fromJson<QString>(jo.value("network_id"_ls));

    return result;
}

QJsonObject QMatrixClient::toJson(const ThirdPartyProtocol& pod)
{
    QJsonObject jo;
    addParam<>(jo, QStringLiteral("user_fields"), pod.userFields);
    addParam<>(jo, QStringLiteral("location_fields"), pod.locationFields);
    addParam<>(jo, QStringLiteral("icon"), pod.icon);
    addParam<>(jo, QStringLiteral("field_types"), pod.fieldTypes);
    addParam<>(jo, QStringLiteral("instances"), pod.instances);
    return jo;
}

ThirdPartyProtocol FromJsonObject<ThirdPartyProtocol>::operator()(const QJsonObject& jo) const
{
    ThirdPartyProtocol result;
    result.userFields =
        fromJson<QStringList>(jo.value("user_fields"_ls));
    result.locationFields =
        fromJson<QStringList>(jo.value("location_fields"_ls));
    result.icon =
        fromJson<QString>(jo.value("icon"_ls));
    result.fieldTypes =
        fromJson<QHash<QString, FieldType>>(jo.value("field_types"_ls));
    result.instances =
        fromJson<QVector<ProtocolInstance>>(jo.value("instances"_ls));

    return result;
}

