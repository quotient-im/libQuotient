// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "cross_signing.h"

using namespace Quotient;

UploadCrossSigningKeysJob::UploadCrossSigningKeysJob(
    const std::optional<CrossSigningKey>& masterKey,
    const std::optional<CrossSigningKey>& selfSigningKey,
    const std::optional<CrossSigningKey>& userSigningKey,
    const std::optional<AuthenticationData>& auth)
    : BaseJob(HttpVerb::Post, QStringLiteral("UploadCrossSigningKeysJob"),
              makePath("/_matrix/client/v3", "/keys/device_signing/upload"))
{
    QJsonObject _dataJson;
    addParam<IfNotEmpty>(_dataJson, QStringLiteral("master_key"), masterKey);
    addParam<IfNotEmpty>(_dataJson, QStringLiteral("self_signing_key"), selfSigningKey);
    addParam<IfNotEmpty>(_dataJson, QStringLiteral("user_signing_key"), userSigningKey);
    addParam<IfNotEmpty>(_dataJson, QStringLiteral("auth"), auth);
    setRequestData({ _dataJson });
}

UploadCrossSigningSignaturesJob::UploadCrossSigningSignaturesJob(
    const QHash<UserId, QHash<QString, QJsonObject>>& signatures)
    : BaseJob(HttpVerb::Post, QStringLiteral("UploadCrossSigningSignaturesJob"),
              makePath("/_matrix/client/v3", "/keys/signatures/upload"))
{
    setRequestData({ toJson(signatures) });
}
