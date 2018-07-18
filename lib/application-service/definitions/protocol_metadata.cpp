/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "protocol_metadata.h"

using namespace QMatrixClient;

QJsonObject QMatrixClient::toJson(const ProtocolMetadata& pod)
{
    QJsonObject _json;
    return _json;
}

ProtocolMetadata FromJson<ProtocolMetadata>::operator()(const QJsonValue& jv)
{
    const auto& _json = jv.toObject();
    ProtocolMetadata result;
    
    return result;
}

