// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <Quotient/util.h>

struct OlmUtility;

namespace Quotient {

//! Allows you to make use of crytographic hashing via SHA-2 and
//! verifying ed25519 signatures.
class QUOTIENT_API QOlmUtility
{
public:
    QOlmUtility();

    //! Verify a ed25519 signature.
    //! \param key QByteArray The public part of the ed25519 key that signed the message.
    //! \param message QByteArray The message that was signed.
    //! \param signature QByteArray The signature of the message.
    bool ed25519Verify(const QByteArray& key, const QByteArray& message,
                       QByteArray signature) const;

    OlmErrorCode lastErrorCode() const;
    const char* lastError() const;

private:
    CStructPtr<OlmUtility> olmDataHolder;
};
} // namespace Quotient
