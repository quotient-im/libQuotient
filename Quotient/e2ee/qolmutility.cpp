// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "qolmutility.h"

#include "e2ee_common.h"

#include <vodozemac/vodozemac.h>

using namespace Quotient;

QOlmUtility::QOlmUtility()
    : olmDataHolder()
{}

bool QOlmUtility::ed25519Verify(const QByteArray& key, const QByteArray& message,
                                QByteArray signature) const
{
    auto sigResult = types::ed25519_signature_from_base64(rust::String(signature.data(), signature.size()));
    //TODO sigResult error handling
    auto sig = types::ed25519_signature_from_base64_result_value(std::move(sigResult));

    auto keyResult = types::ed25519_public_key_from_base64(rust::String(key.data(), key.size()));
    //TODO keyResult error handling
    auto edKey = types::ed25519_public_key_from_base64_result_value(std::move(keyResult));
    return edKey->verify(rust::Slice<const std::uint8_t>((const unsigned char *) message.data(), message.size()), *sig);
}
