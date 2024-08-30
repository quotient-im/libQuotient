// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "cross_signing.h"

using namespace Quotient;

UploadCrossSigningKeysJob::UploadCrossSigningKeysJob(
    const std::optional<CrossSigningKey>& masterKey,
    const std::optional<CrossSigningKey>& selfSigningKey,
    const std::optional<CrossSigningKey>& userSigningKey,
    const std::optional<AuthenticationData>& auth)
    : BaseJob(HttpVerb::Post, u"UploadCrossSigningKeysJob"_s,
              makePath("/_matrix/client/v3", "/keys/device_signing/upload"))
{
    QJsonObject _dataJson;
    addParam<IfNotEmpty>(_dataJson, "master_key"_L1, masterKey);
    addParam<IfNotEmpty>(_dataJson, "self_signing_key"_L1, selfSigningKey);
    addParam<IfNotEmpty>(_dataJson, "user_signing_key"_L1, userSigningKey);
    addParam<IfNotEmpty>(_dataJson, "auth"_L1, auth);
    setRequestData({ _dataJson });
}

UploadCrossSigningSignaturesJob::UploadCrossSigningSignaturesJob(
    const QHash<UserId, QHash<QString, QJsonObject>>& signatures)
    : BaseJob(HttpVerb::Post, u"UploadCrossSigningSignaturesJob"_s,
              makePath("/_matrix/client/v3", "/keys/signatures/upload"))
{
    setRequestData({ toJson(signatures) });
}
