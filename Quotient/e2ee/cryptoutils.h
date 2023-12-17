// SPDX-FileCopyrightText: 2023 Tobias Fella <tobias.fella@kde.org>
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include "e2ee_common.h"
#include "expected.h"

#include "../quotient_export.h"

#include <QtCore/QByteArray>
#include <QtCore/QString>

#include <openssl/err.h>

namespace Quotient {
struct QUOTIENT_API HkdfKeys {
    //! @brief Key to be used for AES encryption / decryption
    QByteArray aes;
    //! @brief Key to be used for MAC creation / verification
    QByteArray mac;
};

struct QUOTIENT_API Curve25519Encrypted {
    QByteArray ciphertext;
    QByteArray mac;
    QByteArray ephemeral;
};

constexpr auto DefaultPbkdf2KeyLength = 32;

using SslErrorCode = decltype(ERR_get_error()); // unsigned long

constexpr SslErrorCode WrongDerivedKeyLength = ERR_LIB_USER + 1;

//! Same as QOlmExpected but for wrapping OpenSSL instead of Olm calls
template <typename T>
using SslExpected = Expected<T, SslErrorCode>;

//! Generate a key out of the given password
QUOTIENT_API SslExpected<QByteArray> pbkdf2HmacSha512(
    const QByteArray& password, const QByteArray& salt, int iterations,
    int keyLength = DefaultPbkdf2KeyLength);

//! \brief Derive a key from the input data using HKDF-SHA256
//!
//! All input parameters have a length of 32 bytes
QUOTIENT_API SslExpected<HkdfKeys> hkdfSha256(const QByteArray& key,
                                              const QByteArray& salt,
                                              const QByteArray& info);

//! Calculate a MAC from the given key and data
QUOTIENT_API SslExpected<QByteArray> hmacSha256(const QByteArray& hmacKey,
                                                const QByteArray& data);

//! \brief Decrypt the data using Curve25519-AES-Sha256
//! \note ciphertext must be given as base64
QUOTIENT_API QOlmExpected<QByteArray> curve25519AesSha2Decrypt(
    QByteArray ciphertext, const QByteArray& privateKey,
    const QByteArray& ephemeral, const QByteArray& mac);

//! \brief Encrypt the data using Curve25519-AES-Sha256
//! \note publicKey must be given as base64
QUOTIENT_API QOlmExpected<Curve25519Encrypted> curve25519AesSha2Encrypt(
    const QByteArray& plaintext, const QByteArray& publicKey);

//! \brief Encrypt data using AES-CTR-256
//!
//! key and iv have a length of 32 bytes
QUOTIENT_API SslExpected<QByteArray> aesCtr256Encrypt(
    const QByteArray& plaintext, const QByteArray& key, const QByteArray& iv);

//! \brief Decrypt data using AES-CTR-256
//!
//! key and iv have a length of 32 bytes
QUOTIENT_API SslExpected<QByteArray> aesCtr256Decrypt(
    const QByteArray& ciphertext, const QByteArray& key, const QByteArray& iv);

QUOTIENT_API QByteArray base58Decode(const QByteArray& encoded);

} // namespace Quotient
