// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "encryptedfile.h"
#include "logging.h"

#include <openssl/evp.h>
#include <QtCore/QCryptographicHash>

using namespace Quotient;

QByteArray EncryptedFile::decryptFile(const QByteArray &ciphertext) const
{
    QString _key = key.k;
    _key = QByteArray::fromBase64(_key.replace(QLatin1Char('_'), QLatin1Char('/')).replace(QLatin1Char('-'), QLatin1Char('+')).toLatin1());
    const auto sha256 = QByteArray::fromBase64(hashes["sha256"].toLatin1());
    if(sha256 != QCryptographicHash::hash(ciphertext, QCryptographicHash::Sha256)) {
        qCWarning(E2EE) << "Hash verification failed for file";
        return QByteArray();
    }
    QByteArray plaintext(ciphertext.size(), 0);
    EVP_CIPHER_CTX *ctx;
    int length;
    ctx = EVP_CIPHER_CTX_new();
    EVP_DecryptInit_ex(ctx, EVP_aes_256_ctr(), NULL, (const unsigned char *)_key.data(), (const unsigned char *)iv.toLatin1().data());
    EVP_DecryptUpdate(ctx, (unsigned char *)plaintext.data(), &length, (const unsigned char *)ciphertext.data(), ciphertext.size());
    EVP_DecryptFinal_ex(ctx, (unsigned char *)plaintext.data() + length, &length);
    EVP_CIPHER_CTX_free(ctx);
    return plaintext;
}
