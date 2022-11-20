// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "e2ee/qolmutility.h"

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

QString QOlmUtility::sha256Bytes(const QByteArray& inputBuf) const
{
    const auto outputLength = olm_sha256_length(olmDataHolder.get());
    auto outputBuf = byteArrayForOlm(outputLength);
    olm_sha256(olmDataHolder.get(), inputBuf.data(), unsignedSize(inputBuf),
            outputBuf.data(), outputLength);

    return QString::fromUtf8(outputBuf);
}

QString QOlmUtility::sha256Utf8Msg(const QString& message) const
{
    return sha256Bytes(message.toUtf8());
}

bool QOlmUtility::ed25519Verify(const QByteArray& key, const QByteArray& message,
                                QByteArray signature) const
{
    return olm_ed25519_verify(olmDataHolder.get(), key.data(), unsignedSize(key),
                              message.data(), unsignedSize(message),
                              signature.data(), unsignedSize(signature))
           == 0;
}
