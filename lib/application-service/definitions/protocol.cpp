/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "protocol.h"

using namespace QMatrixClient;

QJsonObject QMatrixClient::toJson(const FieldType& pod)
{
    QJsonObject jo;
    addParam<IfNotEmpty>(jo, QStringLiteral("regexp"), pod.regexp);
    addParam<IfNotEmpty>(jo, QStringLiteral("placeholder"), pod.placeholder);
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

QJsonObject QMatrixClient::toJson(const FieldTypes& pod)
{
    QJsonObject jo;
    addParam<IfNotEmpty>(jo, QStringLiteral("fieldname"), pod.fieldname);
    return jo;
}

FieldTypes FromJsonObject<FieldTypes>::operator()(const QJsonObject& jo) const
{
    FieldTypes result;
    result.fieldname =
        fromJson<FieldType>(jo.value("fieldname"_ls));

    return result;
}

QJsonObject QMatrixClient::toJson(const ThirdPartyProtocol& pod)
{
    QJsonObject jo;
    addParam<IfNotEmpty>(jo, QStringLiteral("user_fields"), pod.userFields);
    addParam<IfNotEmpty>(jo, QStringLiteral("location_fields"), pod.locationFields);
    addParam<IfNotEmpty>(jo, QStringLiteral("icon"), pod.icon);
    addParam<IfNotEmpty>(jo, QStringLiteral("field_types"), pod.fieldTypes);
    addParam<IfNotEmpty>(jo, QStringLiteral("instances"), pod.instances);
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
        fromJson<FieldTypes>(jo.value("field_types"_ls));
    result.instances =
        fromJson<QVector<QJsonObject>>(jo.value("instances"_ls));

    return result;
}

