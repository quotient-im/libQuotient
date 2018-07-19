/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "protocol.h"

using namespace QMatrixClient;

QJsonObject QMatrixClient::toJson(const FieldType& pod)
{
    QJsonObject _json;
    addParam<IfNotEmpty>(_json, QStringLiteral("regexp"), pod.regexp);
    addParam<IfNotEmpty>(_json, QStringLiteral("placeholder"), pod.placeholder);
    return _json;
}

FieldType FromJson<FieldType>::operator()(const QJsonValue& jv)
{
    const auto& _json = jv.toObject();
    FieldType result;
    result.regexp =
        fromJson<QString>(_json.value("regexp"_ls));
    result.placeholder =
        fromJson<QString>(_json.value("placeholder"_ls));
    
    return result;
}

QJsonObject QMatrixClient::toJson(const FieldTypes& pod)
{
    QJsonObject _json;
    addParam<IfNotEmpty>(_json, QStringLiteral("fieldname"), pod.fieldname);
    return _json;
}

FieldTypes FromJson<FieldTypes>::operator()(const QJsonValue& jv)
{
    const auto& _json = jv.toObject();
    FieldTypes result;
    result.fieldname =
        fromJson<FieldType>(_json.value("fieldname"_ls));
    
    return result;
}

QJsonObject QMatrixClient::toJson(const ThirdPartyProtocol& pod)
{
    QJsonObject _json;
    addParam<IfNotEmpty>(_json, QStringLiteral("user_fields"), pod.userFields);
    addParam<IfNotEmpty>(_json, QStringLiteral("location_fields"), pod.locationFields);
    addParam<IfNotEmpty>(_json, QStringLiteral("icon"), pod.icon);
    addParam<IfNotEmpty>(_json, QStringLiteral("field_types"), pod.fieldTypes);
    addParam<IfNotEmpty>(_json, QStringLiteral("instances"), pod.instances);
    return _json;
}

ThirdPartyProtocol FromJson<ThirdPartyProtocol>::operator()(const QJsonValue& jv)
{
    const auto& _json = jv.toObject();
    ThirdPartyProtocol result;
    result.userFields =
        fromJson<QStringList>(_json.value("user_fields"_ls));
    result.locationFields =
        fromJson<QStringList>(_json.value("location_fields"_ls));
    result.icon =
        fromJson<QString>(_json.value("icon"_ls));
    result.fieldTypes =
        fromJson<FieldTypes>(_json.value("field_types"_ls));
    result.instances =
        fromJson<QVector<QJsonObject>>(_json.value("instances"_ls));
    
    return result;
}

