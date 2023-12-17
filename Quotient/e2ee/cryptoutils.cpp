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

//! \brief A wrapper for `std::unique_ptr` for use with OpenSSL context functions
//!
//! This class and the deduction guide for it are merely to remove
//! the boilerplate necessary to pass custom deleter to `std::unique_ptr`.
//! Usage: `const ContextHolder ctx(CTX_new(), &CTX_free);`, where `CTX_new` and
//! `CTX_free` are the matching allocation and deallocation functions from
//! OpenSSL API. You can pass additional parameters to the allocation function
//! as needed; the deallocation function is assumed to take exactly one
//! parameter of the same type that is returned by the allocation function.
template <class Context>
class ContextHolder : public std::unique_ptr<Context, void (*)(Context*)> {
public:
    using std::unique_ptr<Context, void (*)(Context*)>::unique_ptr;
};
template <class CryptoContext, typename Deleter>
ContextHolder(CryptoContext*, Deleter) -> ContextHolder<CryptoContext>;

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
    auto encrypted = getRandom(unsignedSize(plaintext) + AES_BLOCK_SIZE);
    constexpr auto mask = static_cast<std::uint8_t>(~(1U << (63 / 8)));
    encrypted[15 - 63 % 8] &= mask;

    const ContextHolder ctx(EVP_CIPHER_CTX_new(), &EVP_CIPHER_CTX_free);
    if (!ctx) {
        qWarning() << ERR_error_string(ERR_get_error(), nullptr);
        return ERR_get_error();
    }

    if (EVP_EncryptInit_ex(ctx.get(), EVP_aes_256_ctr(), nullptr, reinterpret_cast<const unsigned char*>(key.data()), reinterpret_cast<const unsigned char*>(iv.data())) != 1) {
        qWarning() << ERR_error_string(ERR_get_error(), nullptr);
        return ERR_get_error();
    }

    int length = 0;
    if (EVP_EncryptUpdate(ctx.get(), reinterpret_cast<unsigned char*>(encrypted.data()), &length, reinterpret_cast<const unsigned char *>(&plaintext.data()[0]), (int) plaintext.size()) != 1) {
        qWarning() << ERR_error_string(ERR_get_error(), nullptr);
        return ERR_get_error();
    }

    int ciphertextLength = length;
    if (EVP_EncryptFinal_ex(ctx.get(), reinterpret_cast<unsigned char*>(encrypted.data()) + length, &length) != 1) {
        qWarning() << ERR_error_string(ERR_get_error(), nullptr);
        return ERR_get_error();
    }

    ciphertextLength += length;
    return encrypted.viewAsByteArray().left(ciphertextLength);
}

#define CALL_OPENSSL(Call_)                                           \
    do {                                                              \
        if (Call_ != 1) {                                             \
            qWarning() << ERR_error_string(ERR_get_error(), nullptr); \
            return ERR_get_error();                                   \
        }                                                             \
    } while (false) // End of macro

SslExpected<HkdfKeys> Quotient::hkdfSha256(const QByteArray& key,
                                           const QByteArray& salt,
                                           const QByteArray& info)
{
    QByteArray result(64, u'\0');
    const ContextHolder context(EVP_PKEY_CTX_new_id(EVP_PKEY_HKDF, nullptr),
                                &EVP_PKEY_CTX_free);

    CALL_OPENSSL(EVP_PKEY_derive_init(context.get()));
    CALL_OPENSSL(EVP_PKEY_CTX_set_hkdf_md(context.get(), EVP_sha256()));
    CALL_OPENSSL(EVP_PKEY_CTX_set1_hkdf_salt(
        context.get(), reinterpret_cast<const unsigned char*>(salt.data()),
        salt.size()));
    CALL_OPENSSL(EVP_PKEY_CTX_set1_hkdf_key(
        context.get(), reinterpret_cast<const unsigned char*>(key.data()),
        key.size()));
    CALL_OPENSSL(EVP_PKEY_CTX_add1_hkdf_info(
        context.get(), reinterpret_cast<const unsigned char*>(info.data()),
        info.size()));
    auto outputLength = unsignedSize(result);
    CALL_OPENSSL(EVP_PKEY_derive(context.get(),
                                 reinterpret_cast<unsigned char*>(result.data()),
                                 &outputLength));
    if (outputLength != 64) {
        qCCritical(E2EE) << "hkdfSha256: the derived key is" << outputLength
                         << "bytes instead of 64";
        Q_ASSERT(false);
        return WrongDerivedKeyLength;
    }

    auto macKey = result.mid(32);
    result.resize(32);
    return HkdfKeys{std::move(result), std::move(macKey)};
}

SslExpected<QByteArray> Quotient::hmacSha256(const QByteArray& hmacKey,
                                             const QByteArray& data)
{
    uint32_t len = SHA256_DIGEST_LENGTH;
    QByteArray output(SHA256_DIGEST_LENGTH, u'\0');
    if (!HMAC(EVP_sha256(), hmacKey.data(), hmacKey.size(), reinterpret_cast<const unsigned char *>(data.data()), data.size(), reinterpret_cast<unsigned char *>(output.data()), &len)) {
        qWarning() << ERR_error_string(ERR_get_error(), nullptr);
        return ERR_get_error();
    }
    return output;
}

SslExpected<QByteArray> Quotient::aesCtr256Decrypt(const QByteArray& ciphertext,
                                                   const QByteArray& aes256Key,
                                                   const QByteArray& iv)
{
    const ContextHolder context(EVP_CIPHER_CTX_new(), EVP_CIPHER_CTX_free);
    Q_ASSERT(context);

    int length = 0;
    int plaintextLength = 0;
    QByteArray decrypted(ciphertext.size(), u'\0');

    if (EVP_DecryptInit_ex(context.get(), EVP_aes_256_ctr(), nullptr, reinterpret_cast<const unsigned char *>(aes256Key.data()), reinterpret_cast<const unsigned char *>(iv.data())) != 1) {
        qWarning() << ERR_error_string(ERR_get_error(), nullptr);
        return ERR_get_error();
    }

    if (EVP_DecryptUpdate(context.get(), reinterpret_cast<unsigned char*>(decrypted.data()), &length, reinterpret_cast<const unsigned char*>(&ciphertext.data()[0]), (int) ciphertext.size()) != 1) {
        qWarning() << ERR_error_string(ERR_get_error(), nullptr);
        return ERR_get_error();
    }

    plaintextLength = length;
    if (EVP_DecryptFinal_ex(context.get(), reinterpret_cast<unsigned char*>(decrypted.data()) + length, &length) != 1) {
        qWarning() << ERR_error_string(ERR_get_error(), nullptr);
        return ERR_get_error();
    }

    plaintextLength += length;
    decrypted.resize(plaintextLength);
    return decrypted;
}

QOlmExpected<QByteArray> Quotient::curve25519AesSha2Decrypt(
    QByteArray ciphertext, const QByteArray& privateKey,
    const QByteArray& ephemeral, const QByteArray& mac)
{
    auto context = makeCStruct(olm_pk_decryption, olm_pk_decryption_size, olm_clear_pk_decryption);
    Q_ASSERT(context);

    QByteArray publicKey(olm_pk_key_length(), u'\0');
    if (olm_pk_key_from_private(context.get(), publicKey.data(), publicKey.size(), privateKey.data(), privateKey.size()) == olm_error())
        return olm_pk_decryption_last_error_code(context.get());

    QByteArray plaintext(olm_pk_max_plaintext_length(context.get(), ciphertext.size()), u'\0');
    auto result = olm_pk_decrypt(context.get(), ephemeral.data(), ephemeral.size(), mac.data(), mac.size(), ciphertext.data(), ciphertext.size(), plaintext.data(), plaintext.size());
    if (result == olm_error())
        return olm_pk_decryption_last_error_code(context.get());

    plaintext.resize(result);
    return plaintext;
}

QOlmExpected<Curve25519Encrypted> Quotient::curve25519AesSha2Encrypt(
    const QByteArray& plaintext, const QByteArray& publicKey)
{
    auto context = makeCStruct(olm_pk_encryption, olm_pk_encryption_size, olm_clear_pk_encryption);

    if (olm_pk_encryption_set_recipient_key(context.get(), publicKey.data(), publicKey.size()) == olm_error())
        return olm_pk_encryption_last_error_code(context.get());

    QByteArray ephemeral(olm_pk_key_length(), 0);
    QByteArray mac(olm_pk_mac_length(context.get()), 0);
    QByteArray ciphertext(olm_pk_ciphertext_length(context.get(), plaintext.size()), 0);
    const auto random = getRandom(olm_pk_encrypt_random_length(context.get()));

    if (olm_pk_encrypt(context.get(), plaintext.data(), plaintext.size(),
                       ciphertext.data(), ciphertext.size(), mac.data(),
                       mac.size(), ephemeral.data(), ephemeral.size(),
                       random.data(), random.size())
        == olm_error())
        return olm_pk_encryption_last_error_code(context.get());

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
