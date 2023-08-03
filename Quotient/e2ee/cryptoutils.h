// SPDX-FileCopyrightText: 2023 Tobias Fella <tobias.fella@kde.org>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include <QtCore/QString>
#include <QtCore/QByteArray>

#include "../quotient_export.h"

namespace Quotient {
struct QUOTIENT_API HkdfKeys {
    QByteArray aes;
    QByteArray mac;
};

QUOTIENT_API QByteArray pbkdf2HmacSha512(const QString & password, const QByteArray& salt, int iterations, int keyLength);

QUOTIENT_API HkdfKeys hkdfSha256(const QByteArray& key, const QByteArray& salt, const QByteArray& info);

QUOTIENT_API QByteArray hmacSha256(const QByteArray& hmacKey, const QByteArray& data);

QUOTIENT_API QByteArray curve25519AesSha2Decrypt(QByteArray base64_ciphertext, const QByteArray &privateKey, const QByteArray &ephemeral, const QByteArray &mac);

/**
 * @brief Encrypt a file using AES-CTR-256
 */
QUOTIENT_API QByteArray aesCtr256Encrypt(const QByteArray& plaintext, const QByteArray& key, const QByteArray& iv);
QUOTIENT_API QByteArray aesCtr256Decrypt(const QByteArray& ciphertext, const QByteArray& key, const QByteArray& iv);
}
