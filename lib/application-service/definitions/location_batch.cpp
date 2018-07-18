/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "location_batch.h"

using namespace QMatrixClient;

QJsonObject QMatrixClient::toJson(const LocationBatch& pod)
{
    QJsonObject _json;
    return _json;
}

LocationBatch FromJson<LocationBatch>::operator()(const QJsonValue& jv)
{
    const auto& _json = jv.toObject();
    LocationBatch result;
    
    return result;
}

