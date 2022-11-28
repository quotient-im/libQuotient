// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "e2ee/qolmutility.h"

#include "csapi/definitions/device_keys.h"

#include "e2ee_common.h"

#include <olm/olm.h>

using namespace Quotient;

OlmErrorCode QOlmUtility::lastErrorCode() const {
    return olm_utility_last_error_code(olmDataHolder.get());
}

const char* QOlmUtility::lastError() const
{
    return olm_utility_last_error(olmDataHolder.get());
}

QOlmUtility::QOlmUtility()
    : olmDataHolder(
        makeCStruct(olm_utility, olm_utility_size, olm_clear_utility))
{}

QByteArray QOlmUtility::sha256Bytes(const QByteArray& inputBuf) const
{
    const auto outputLength = olm_sha256_length(olmDataHolder.get());
    auto outputBuf = byteArrayForOlm(outputLength);
    if (olm_sha256(olmDataHolder.get(), inputBuf.data(), unsignedSize(inputBuf),
                   outputBuf.data(), outputLength)
        != OLM_SUCCESS)
        QOLM_INTERNAL_ERROR_X("Failed to calculate SHA256 sum", lastError());

    return outputBuf;
}

bool QOlmUtility::ed25519Verify(const QByteArray& key,
                                const QByteArray& canonicalJson,
                                QByteArray signature) const
{
    return olm_ed25519_verify(olmDataHolder.get(), key.data(), unsignedSize(key),
                              canonicalJson.data(), unsignedSize(canonicalJson),
                              signature.data(), unsignedSize(signature))
           == 0;
}

bool QOlmUtility::verifyIdentitySignature(const DeviceKeys& deviceKeys,
                                          const QString& deviceId,
                                          const QString& userId) const
{
    const auto signKeyId = "ed25519:" + deviceId;
    const auto signature = deviceKeys.signatures[userId][signKeyId];
    if (signature.isEmpty())
        return false;

    return ed25519Verify(deviceKeys.keys[signKeyId].toLatin1(),
                         toCanonicalJson(deviceKeys), signature.toLatin1());
}
