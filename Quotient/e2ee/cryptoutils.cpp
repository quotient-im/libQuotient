// SPDX-FileCopyrightText: 2023 Tobias Fella <tobias.fella@kde.org>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "cryptoutils.h"

#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/kdf.h>
#include <openssl/rand.h>
#include <openssl/sha.h>

#include <olm/pk.h>
#include <olm/olm.h>

#include <util.h>

using namespace Quotient;

QByteArray Quotient::pbkdf2HmacSha512(const QByteArray& password, const QByteArray& salt, int iterations, int keyLength)
{
    QByteArray output(keyLength, u'\0');
    PKCS5_PBKDF2_HMAC(password.data(), password.size(), reinterpret_cast<const unsigned char *>(salt.data()), salt.size(), iterations, EVP_sha512(), keyLength, reinterpret_cast<unsigned char *>(output.data()));
    return output;
}

QByteArray Quotient::aesCtr256Encrypt(const QByteArray& plaintext, const QByteArray& key, const QByteArray& iv)
{
    EVP_CIPHER_CTX* ctx = nullptr;
    int length = 0;
    int ciphertextLength = 0;

    auto encrypted = QByteArray(plaintext.size() + AES_BLOCK_SIZE, u'\0');
    RAND_bytes(reinterpret_cast<unsigned char*>(encrypted.data()), encrypted.size());
    auto data = encrypted.data();
    constexpr auto mask = static_cast<std::uint8_t>(~(1U << (63 / 8)));
    data[15 - 63 % 8] &= mask;
    if (ctx = EVP_CIPHER_CTX_new(); !ctx) {
        return {};
    }

    EVP_EncryptInit_ex(ctx, EVP_aes_256_ctr(), nullptr, reinterpret_cast<const unsigned char*>(key.data()), reinterpret_cast<const unsigned char*>(iv.data()));

    EVP_EncryptUpdate(ctx, reinterpret_cast<unsigned char*>(encrypted.data()), &length, reinterpret_cast<const unsigned char *>(&plaintext.data()[0]), (int) plaintext.size());

    ciphertextLength = length;
    EVP_EncryptFinal_ex(ctx, reinterpret_cast<unsigned char*>(encrypted.data()) + length, &length);
    ciphertextLength += length;
    encrypted.resize(ciphertextLength);
    EVP_CIPHER_CTX_free(ctx);
    return encrypted;
}

HkdfKeys Quotient::hkdfSha256(const QByteArray& key, const QByteArray& salt, const QByteArray& info)
{
    QByteArray result(64, u'\0');
    auto context = EVP_PKEY_CTX_new_id(EVP_PKEY_HKDF, nullptr);

    EVP_PKEY_derive_init(context);
    EVP_PKEY_CTX_set_hkdf_md(context, EVP_sha256());
    EVP_PKEY_CTX_set1_hkdf_salt(context, reinterpret_cast<const unsigned char *>(salt.data()), salt.size());
    EVP_PKEY_CTX_set1_hkdf_key(context, reinterpret_cast<const unsigned char *>(key.data()), key.size());
    EVP_PKEY_CTX_add1_hkdf_info(context, reinterpret_cast<const unsigned char *>(info.data()), info.size());
    std::size_t outputLength = result.size();
    EVP_PKEY_derive(context, reinterpret_cast<unsigned char *>(result.data()), &outputLength);
    EVP_PKEY_CTX_free(context);

    if (outputLength != 64) {
        return {};
    }

    QByteArray macKey = result.mid(32);
    result.resize(32);
    return {std::move(result), std::move(macKey)};
}

QByteArray Quotient::hmacSha256(const QByteArray& hmacKey, const QByteArray& data)
{
    uint32_t len = SHA256_DIGEST_LENGTH;
    QByteArray output(SHA256_DIGEST_LENGTH, u'\0');
    HMAC(EVP_sha256(), hmacKey.data(), hmacKey.size(), reinterpret_cast<const unsigned char *>(data.data()), data.size(), reinterpret_cast<unsigned char *>(output.data()), &len);
    return output;
}

QByteArray Quotient::aesCtr256Decrypt(const QByteArray& ciphertext, const QByteArray& aes256Key, const QByteArray& iv)
{
    auto context = EVP_CIPHER_CTX_new();
    Q_ASSERT(context);

    int length = 0;
    int plaintextLength = 0;
    QByteArray decrypted(ciphertext.size(), u'\0');

    EVP_DecryptInit_ex(context, EVP_aes_256_ctr(), nullptr, reinterpret_cast<const unsigned char *>(aes256Key.data()), reinterpret_cast<const unsigned char *>(iv.data()));

    EVP_DecryptUpdate(context, reinterpret_cast<unsigned char*>(decrypted.data()), &length, reinterpret_cast<const unsigned char*>(&ciphertext.data()[0]), (int) ciphertext.size());
    plaintextLength = length;
    EVP_DecryptFinal_ex(context, reinterpret_cast<unsigned char*>(decrypted.data()) + length, &length);
    plaintextLength += length;
    decrypted.resize(plaintextLength);
    EVP_CIPHER_CTX_free(context);
    return decrypted;
}

QByteArray Quotient::curve25519AesSha2Decrypt(QByteArray ciphertext, const QByteArray &privateKey, const QByteArray &ephemeral, const QByteArray &mac)
{
    auto context = makeCStruct(olm_pk_decryption, olm_pk_decryption_size, olm_clear_pk_decryption);

    QByteArray publicKey(olm_pk_key_length(), u'\0');
    olm_pk_key_from_private(context.get(), publicKey.data(), publicKey.size(), privateKey.data(), privateKey.size());

    QByteArray plaintext(olm_pk_max_plaintext_length(context.get(), ciphertext.size()), u'\0');
    auto result = olm_pk_decrypt(context.get(), ephemeral.data(), ephemeral.size(), mac.data(), mac.size(), ciphertext.data(), ciphertext.size(), plaintext.data(), plaintext.size());

    if (result == olm_error()) {
        return {};
    }
    plaintext.resize(result);
    return plaintext;
}

Curve25519Encrypted Quotient::curve25519AesSha2Encrypt(const QByteArray& plaintext, const QByteArray& publicKey)
{
    auto context = makeCStruct(olm_pk_encryption, olm_pk_encryption_size, olm_clear_pk_encryption);

    olm_pk_encryption_set_recipient_key(context.get(), publicKey.data(), publicKey.size());

    QByteArray ephemeral(olm_pk_key_length(), 0);
    QByteArray mac(olm_pk_mac_length(context.get()), 0);
    QByteArray ciphertext(olm_pk_ciphertext_length(context.get(), plaintext.size()), 0);
    QByteArray randomBuffer(olm_pk_encrypt_random_length(context.get()), 0);
    RAND_bytes(reinterpret_cast<unsigned char*>(randomBuffer.data()), randomBuffer.size());

    auto result = olm_pk_encrypt(context.get(), plaintext.data(), plaintext.size(), ciphertext.data(), ciphertext.size(), mac.data(), mac.size(), ephemeral.data(), ephemeral.size(), randomBuffer.data(), randomBuffer.size());

    if (result == olm_error()) {
        return {};
    }
    return Curve25519Encrypted {
        .ciphertext = ciphertext,
        .mac = mac,
        .ephemeral = ephemeral,
    };
}
