// SPDX-FileCopyrightText: 2023 Tobias Fella <tobias.fella@kde.org>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "cryptoutils.h"

#include "../logging_categories_p.h"
#include "../util.h"

#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/kdf.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <openssl/err.h>

#include <olm/pk.h>
#include <olm/olm.h>

using namespace Quotient;

static_assert(std::is_same_v<SslErrorCode, decltype(ERR_get_error())>);
static_assert(SslErrorUserOffset == ERR_LIB_USER);

SslExpected<QByteArray> Quotient::pbkdf2HmacSha512(const QByteArray& password,
                                                   const QByteArray& salt,
                                                   int iterations, int keyLength)
{
    QByteArray output(keyLength, u'\0');
    bool success = PKCS5_PBKDF2_HMAC(password.data(), password.size(), reinterpret_cast<const unsigned char *>(salt.data()), salt.length(), iterations, EVP_sha512(), keyLength, reinterpret_cast<unsigned char *>(output.data()));
    if (!success) {
        qWarning() << ERR_error_string(ERR_get_error(), nullptr);
        return ERR_get_error();
    }
    return output;
}

SslExpected<QByteArray> Quotient::aesCtr256Encrypt(const QByteArray& plaintext,
                                                   const QByteArray& key,
                                                   const QByteArray& iv)
{
    EVP_CIPHER_CTX* ctx = nullptr;
    int length = 0;
    int ciphertextLength = 0;

    auto encrypted = QByteArray(plaintext.size() + AES_BLOCK_SIZE, u'\0');
    int status = RAND_bytes(reinterpret_cast<unsigned char*>(encrypted.data()), encrypted.size());
    if (status != 1) {
        qWarning() << ERR_error_string(ERR_get_error(), nullptr);
        return ERR_get_error();
    }
    auto data = encrypted.data();
    constexpr auto mask = static_cast<std::uint8_t>(~(1U << (63 / 8)));
    data[15 - 63 % 8] &= mask;
    if (ctx = EVP_CIPHER_CTX_new(); !ctx) {
        qWarning() << ERR_error_string(ERR_get_error(), nullptr);
        return ERR_get_error();
    }

    status = EVP_EncryptInit_ex(ctx, EVP_aes_256_ctr(), nullptr, reinterpret_cast<const unsigned char*>(key.data()), reinterpret_cast<const unsigned char*>(iv.data()));
    if (status != 1) {
        qWarning() << ERR_error_string(ERR_get_error(), nullptr);
        EVP_CIPHER_CTX_free(ctx);
        return ERR_get_error();
    }

    status = EVP_EncryptUpdate(ctx, reinterpret_cast<unsigned char*>(encrypted.data()), &length, reinterpret_cast<const unsigned char *>(&plaintext.data()[0]), (int) plaintext.size());
    if (status != 1) {
        qWarning() << ERR_error_string(ERR_get_error(), nullptr);
        EVP_CIPHER_CTX_free(ctx);
        return ERR_get_error();
    }

    ciphertextLength = length;
    status = EVP_EncryptFinal_ex(ctx, reinterpret_cast<unsigned char*>(encrypted.data()) + length, &length);
    if (status != 1) {
        qWarning() << ERR_error_string(ERR_get_error(), nullptr);
        EVP_CIPHER_CTX_free(ctx);
        return ERR_get_error();
    }

    ciphertextLength += length;
    encrypted.resize(ciphertextLength);
    EVP_CIPHER_CTX_free(ctx);
    return encrypted;
}

#define CALL_OPENSSL(Call_)                                           \
    do {                                                              \
        if (Call_ != 1) {                                             \
            qWarning() << ERR_error_string(ERR_get_error(), nullptr); \
            EVP_PKEY_CTX_free(context);                               \
            return ERR_get_error();                                   \
        }                                                             \
    } while (false) // End of macro

SslExpected<HkdfKeys> Quotient::hkdfSha256(const QByteArray& key,
                                           const QByteArray& salt,
                                           const QByteArray& info)
{
    QByteArray result(64, u'\0');
    auto context = EVP_PKEY_CTX_new_id(EVP_PKEY_HKDF, nullptr);

    CALL_OPENSSL(EVP_PKEY_derive_init(context));
    CALL_OPENSSL(EVP_PKEY_CTX_set_hkdf_md(context, EVP_sha256()));
    CALL_OPENSSL(EVP_PKEY_CTX_set1_hkdf_salt(
        context, reinterpret_cast<const unsigned char*>(salt.data()),
        salt.size()));
    CALL_OPENSSL(EVP_PKEY_CTX_set1_hkdf_key(
        context, reinterpret_cast<const unsigned char*>(key.data()),
        key.size()));
    CALL_OPENSSL(EVP_PKEY_CTX_add1_hkdf_info(
        context, reinterpret_cast<const unsigned char*>(info.data()),
        info.size()));
    auto outputLength = unsignedSize(result);
    CALL_OPENSSL(EVP_PKEY_derive(context, reinterpret_cast<unsigned char *>(result.data()), &outputLength));
    if (outputLength != 64) {
        qCCritical(E2EE) << "hkdfSha256: the derived key is" << outputLength
                         << "bytes instead of 64";
        Q_ASSERT(false);
        return WrongDerivedKeyLength;
    }

    EVP_PKEY_CTX_free(context);

    auto macKey = result.mid(32);
    result.resize(32);
    return HkdfKeys{std::move(result), std::move(macKey)};
}

SslExpected<QByteArray> Quotient::hmacSha256(const QByteArray& hmacKey,
                                             const QByteArray& data)
{
    uint32_t len = SHA256_DIGEST_LENGTH;
    QByteArray output(SHA256_DIGEST_LENGTH, u'\0');
    auto status = HMAC(EVP_sha256(), hmacKey.data(), hmacKey.size(), reinterpret_cast<const unsigned char *>(data.data()), data.size(), reinterpret_cast<unsigned char *>(output.data()), &len);
    if (!status) {
        qWarning() << ERR_error_string(ERR_get_error(), nullptr);
        return ERR_get_error();
    }
    return output;
}

SslExpected<QByteArray> Quotient::aesCtr256Decrypt(const QByteArray& ciphertext,
                                                   const QByteArray& aes256Key,
                                                   const QByteArray& iv)
{
    auto context = EVP_CIPHER_CTX_new();
    Q_ASSERT(context);

    int length = 0;
    int plaintextLength = 0;
    QByteArray decrypted(ciphertext.size(), u'\0');

    int status = EVP_DecryptInit_ex(context, EVP_aes_256_ctr(), nullptr, reinterpret_cast<const unsigned char *>(aes256Key.data()), reinterpret_cast<const unsigned char *>(iv.data()));
    if (status != 1) {
        qWarning() << ERR_error_string(ERR_get_error(), nullptr);
        return ERR_get_error();
    }

    status = EVP_DecryptUpdate(context, reinterpret_cast<unsigned char*>(decrypted.data()), &length, reinterpret_cast<const unsigned char*>(&ciphertext.data()[0]), (int) ciphertext.size());
    if (status != 1) {
        qWarning() << ERR_error_string(ERR_get_error(), nullptr);
        return ERR_get_error();
    }

    plaintextLength = length;
    status = EVP_DecryptFinal_ex(context, reinterpret_cast<unsigned char*>(decrypted.data()) + length, &length);
    if (status != 1) {
        qWarning() << ERR_error_string(ERR_get_error(), nullptr);
        return ERR_get_error();
    }

    plaintextLength += length;
    decrypted.resize(plaintextLength);
    EVP_CIPHER_CTX_free(context);
    return decrypted;
}

QOlmExpected<QByteArray> Quotient::curve25519AesSha2Decrypt(
    QByteArray ciphertext, const QByteArray& privateKey,
    const QByteArray& ephemeral, const QByteArray& mac)
{
    auto context = makeCStruct(olm_pk_decryption, olm_pk_decryption_size, olm_clear_pk_decryption);
    Q_ASSERT(context);

    QByteArray publicKey(olm_pk_key_length(), u'\0');
    auto status = olm_pk_key_from_private(context.get(), publicKey.data(), publicKey.size(), privateKey.data(), privateKey.size());
    if (status == olm_error()) {
        return olm_pk_decryption_last_error_code(context.get());
    }

    QByteArray plaintext(olm_pk_max_plaintext_length(context.get(), ciphertext.size()), u'\0');
    auto result = olm_pk_decrypt(context.get(), ephemeral.data(), ephemeral.size(), mac.data(), mac.size(), ciphertext.data(), ciphertext.size(), plaintext.data(), plaintext.size());

    if (result == olm_error()) {
        return olm_pk_decryption_last_error_code(context.get());
    }
    plaintext.resize(result);
    return plaintext;
}

QOlmExpected<Curve25519Encrypted> Quotient::curve25519AesSha2Encrypt(
    const QByteArray& plaintext, const QByteArray& publicKey)
{
    auto context = makeCStruct(olm_pk_encryption, olm_pk_encryption_size, olm_clear_pk_encryption);

    auto status = olm_pk_encryption_set_recipient_key(context.get(), publicKey.data(), publicKey.size());
    if (status == olm_error()) {
        return olm_pk_encryption_last_error_code(context.get());
    }

    QByteArray ephemeral(olm_pk_key_length(), 0);
    QByteArray mac(olm_pk_mac_length(context.get()), 0);
    QByteArray ciphertext(olm_pk_ciphertext_length(context.get(), plaintext.size()), 0);
    const auto random = getRandom(olm_pk_encrypt_random_length(context.get()));

    auto result = olm_pk_encrypt(context.get(), plaintext.data(),
                                 plaintext.size(), ciphertext.data(),
                                 ciphertext.size(), mac.data(), mac.size(),
                                 ephemeral.data(), ephemeral.size(),
                                 random.data(), random.size());
    if (result == olm_error()) {
        return olm_pk_encryption_last_error_code(context.get());
    }

    return Curve25519Encrypted {
        .ciphertext = ciphertext,
        .mac = mac,
        .ephemeral = ephemeral,
    };
}

QByteArray Quotient::base58Decode(const QByteArray& encoded)
{
    auto alphabet = QByteArrayLiteral("123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz");
    QByteArray reverse_alphabet(256, -1);
    for (auto i = 0; i < 58; i++) {
        reverse_alphabet[static_cast<uint8_t>(alphabet.at(i))] = static_cast<char>(i);
    }

    QByteArray result;
    result.reserve(encoded.size() * 733 / 1000 + 1);

    for (auto b : encoded) {
        uint32_t carry = reverse_alphabet[b];
        for (auto &j : result) {
            carry += static_cast<uint8_t>(j) * 58;
            j = static_cast<char>(static_cast<uint8_t>(carry % 0x100));
            carry /= 0x100;
        }
        while (carry > 0) {
            result.push_back(static_cast<char>(static_cast<uint8_t>(carry % 0x100)));
            carry /= 0x100;
        }
    }

    for (auto i = 0; i < encoded.length() && encoded[i] == '1'; i++) {
        result.push_back(u'\0');
    }

    std::reverse(result.begin(), result.end());
    return result;
}
