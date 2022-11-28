// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "util.h"

#include <olm/error.h>

struct OlmUtility;

namespace Quotient {

struct DeviceKeys;

//! Allows you to make use of crytographic hashing via SHA-2 and
//! verifying ed25519 signatures.
class QUOTIENT_API QOlmUtility
{
public:
    QOlmUtility();


    //! Get a base64-encoded sha256 sum of the supplied byte slice
    QByteArray sha256Bytes(const QByteArray& inputBuf) const;

    //! \brief Verify a ed25519 signature
    //!
    //! \param key The public part of the ed25519 key that signed the message
    //! \param canonicalJson The JSON that was signed
    //! \param signature The signature of the message
    bool ed25519Verify(const QByteArray& key, const QByteArray& canonicalJson,
                       QByteArray signature) const;

    //! \brief Verify the device using the given device and user id
    //!
    //! This is a convenience function extracting the necessary key and
    //! signature from \p deviceKeys and using them to verify (the canonical
    //! part of) \p deviceKeys. \sa ed25519Verify \return same as in
    //! ed25519Verify()
    bool verifyIdentitySignature(const DeviceKeys& deviceKeys,
                                 const QString& deviceId, const QString& userId) const;

    OlmErrorCode lastErrorCode() const;
    const char* lastError() const;

private:
    CStructPtr<OlmUtility> olmDataHolder;
};
} // namespace Quotient
