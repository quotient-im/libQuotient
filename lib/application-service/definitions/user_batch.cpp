/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "user_batch.h"

using namespace QMatrixClient;

QJsonObject QMatrixClient::toJson(const UserBatch& pod)
{
    QJsonObject _json;
    return _json;
}

UserBatch FromJson<UserBatch>::operator()(const QJsonValue& jv)
{
    const auto& _json = jv.toObject();
    UserBatch result;
    
    return result;
}

