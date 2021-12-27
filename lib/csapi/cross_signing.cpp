/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "cross_signing.h"

using namespace Quotient;

UploadCrossSigningKeysJob::UploadCrossSigningKeysJob(
    const Omittable<CrossSigningKey>& masterKey,
    const Omittable<CrossSigningKey>& selfSigningKey,
    const Omittable<CrossSigningKey>& userSigningKey)
    : BaseJob(HttpVerb::Post, QStringLiteral("UploadCrossSigningKeysJob"),
              makePath("/_matrix/client/r0", "/keys/device_signing/upload"))
{
    QJsonObject _data;
    addParam<IfNotEmpty>(_data, QStringLiteral("master_key"), masterKey);
    addParam<IfNotEmpty>(_data, QStringLiteral("self_signing_key"),
                         selfSigningKey);
    addParam<IfNotEmpty>(_data, QStringLiteral("user_signing_key"),
                         userSigningKey);
    setRequestData(std::move(_data));
}

UploadCrossSigningSignaturesJob::UploadCrossSigningSignaturesJob(
    const QHash<QString, QHash<QString, QJsonObject>>& signatures)
    : BaseJob(HttpVerb::Post, QStringLiteral("UploadCrossSigningSignaturesJob"),
              makePath("/_matrix/client/r0", "/keys/signatures/upload"))
{
    setRequestData(RequestData(toJson(signatures)));
}
