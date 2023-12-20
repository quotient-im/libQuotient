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

// The checks below make sure the definitions in cryptoutils.h match those in
// OpenSSL headers

static_assert(AesBlockSize == AES_BLOCK_SIZE);
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

template <typename SizeT>
    requires (sizeof(SizeT) >= sizeof(int))
inline std::pair<int, bool> checkedSize(
    SizeT uncheckedSize,
    std::type_identity_t<SizeT> maxSize = std::numeric_limits<int>::max())
// ^ NB: usage of type_identity_t disables type deduction
{
    Q_ASSERT(uncheckedSize >= 0 && maxSize >= 0);
    if (uncheckedSize <= maxSize)
        return { static_cast<int>(uncheckedSize), false };

    qCCritical(E2EE) << "Cryptoutils:" << uncheckedSize
                     << "bytes is too many for OpenSSL, first" << maxSize
                     << "bytes will be taken";
    return { maxSize, true };
}

// NOLINTBEGIN(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
// TODO: remove NOLINT brackets once we're on clang-tidy 18
#define CLAMP_SIZE(SizeVar_, ByteArray_, ...)                              \
    const auto [SizeVar_, ByteArray_##Clamped] =                           \
        checkedSize((ByteArray_).size() __VA_OPT__(, ) __VA_ARGS__);       \
    if (ByteArray_##Clamped) {                                             \
        qCCritical(E2EE).nospace()                                         \
            << __func__ << ": " #ByteArray_ " is " << (ByteArray_).size()  \
            << " bytes long, too much for OpenSSL and overall suspicious"; \
        Q_ASSERT(!ByteArray_##Clamped); /* Always false */                 \
        return SslPayloadTooLong;                                          \
    }                                                                      \
    // End of macro

#define CALL_OPENSSL(Call_)                                                \
    do {                                                                   \
        if ((Call_) <= 0) {                                                \
            qCWarning(E2EE) << __func__ << "failed to call OpenSSL API:"   \
                            << ERR_error_string(ERR_get_error(), nullptr); \
            return ERR_get_error();                                        \
        }                                                                  \
    } while (false) // End of macro
// NOLINTEND(cppcoreguidelines-pro-bounds-array-to-pointer-decay)

SslExpected<QByteArray> Quotient::pbkdf2HmacSha512(const QByteArray& password,
                                                   const QByteArray& salt,
                                                   int iterations, int keyLength)
{
    CLAMP_SIZE(passwordSize, password)
    CLAMP_SIZE(saltSize, salt)
    auto output = zeroedByteArray(keyLength);
    CALL_OPENSSL(
        PKCS5_PBKDF2_HMAC(password.data(), passwordSize, asCBytes(salt).data(),
                          saltSize, iterations, EVP_sha512(), keyLength,
                          reinterpret_cast<unsigned char*>(output.data())));
    return output;
}

SslExpected<QByteArray> Quotient::aesCtr256Encrypt(const QByteArray& plaintext,
                                                   byte_view_t<AesKeySize> key,
                                                   byte_view_t<AesBlockSize> iv)
{
    CLAMP_SIZE(plaintextSize, plaintext,
               std::numeric_limits<int>::max() - iv.size())

    auto encrypted = getRandom(static_cast<size_t>(plaintextSize) + iv.size());
    auto encryptedSpan = asWritableCBytes(encrypted);
    constexpr auto mask = static_cast<uint8_t>(~(1U << (63 / 8)));
    encryptedSpan[15 - 63 % 8] &= mask;

    const ContextHolder ctx(EVP_CIPHER_CTX_new(), &EVP_CIPHER_CTX_free);
    if (!ctx) {
        qCCritical(E2EE) << __func__ << "failed to create SSL context:"
                         << ERR_error_string(ERR_get_error(), nullptr);
        return ERR_get_error();
    }

    CALL_OPENSSL(EVP_EncryptInit_ex(ctx.get(), EVP_aes_256_ctr(), nullptr,
                                    key.data(), iv.data()));

    int encryptedLength = 0;
    CALL_OPENSSL(EVP_EncryptUpdate(ctx.get(), encrypted.data(), &encryptedLength,
                                   reinterpret_cast<const unsigned char*>(
                                       plaintext.constData()),
                                   plaintextSize));
    Q_ASSERT(encryptedLength >= 0);

    int tailLength = -1; // Normally becomes 0 after the next call
    CALL_OPENSSL(EVP_EncryptFinal_ex(
        ctx.get(),
        encryptedSpan.subspan(static_cast<size_t>(encryptedLength)).data(),
        &tailLength));

    return encrypted.copyToByteArray(encryptedLength + tailLength);
}

SslExpected<HkdfKeys> Quotient::hkdfSha256(const QByteArray& key,
                                           const QByteArray& salt,
                                           const QByteArray& info)
{
    const auto saltSize = checkedSize(salt.size()).first,
               keySize = checkedSize(key.size()).first,
               infoSize = checkedSize(info.size()).first;
    HkdfKeys result;
    const ContextHolder context(EVP_PKEY_CTX_new_id(EVP_PKEY_HKDF, nullptr),
                                &EVP_PKEY_CTX_free);

    CALL_OPENSSL(EVP_PKEY_derive_init(context.get()));
    CALL_OPENSSL(EVP_PKEY_CTX_set_hkdf_md(context.get(), EVP_sha256()));
    CALL_OPENSSL(EVP_PKEY_CTX_set1_hkdf_salt(
        context.get(), reinterpret_cast<const unsigned char*>(salt.constData()),
        saltSize));
    CALL_OPENSSL(EVP_PKEY_CTX_set1_hkdf_key(
        context.get(), reinterpret_cast<const unsigned char*>(key.constData()),
        keySize));
    CALL_OPENSSL(EVP_PKEY_CTX_add1_hkdf_info(
        context.get(), reinterpret_cast<const unsigned char*>(info.constData()),
        infoSize));
    size_t outputLength = result.size();
    CALL_OPENSSL(EVP_PKEY_derive(context.get(),
                                 reinterpret_cast<unsigned char*>(result.data()),
                                 &outputLength));
    if (outputLength != result.size()) {
        qCCritical(E2EE) << "hkdfSha256: the shared secret is" << outputLength
                         << "bytes instead of" << result.size();
        Q_ASSERT(false);
        return WrongDerivedKeyLength;
    }

    return result;
}

SslExpected<QByteArray> Quotient::hmacSha256(byte_view_t<HmacKeySize> hmacKey,
                                             const QByteArray& data)
{
    unsigned int len = SHA256_DIGEST_LENGTH;
    auto output = zeroedByteArray(SHA256_DIGEST_LENGTH);
    if (!HMAC(EVP_sha256(), hmacKey.data(), hmacKey.size(), reinterpret_cast<const unsigned char *>(data.constData()), unsignedSize(data), reinterpret_cast<unsigned char *>(output.data()), &len)) {
        qWarning() << ERR_error_string(ERR_get_error(), nullptr);
        return ERR_get_error();
    }
    return output;
}

SslExpected<QByteArray> Quotient::aesCtr256Decrypt(const QByteArray& ciphertext,
                                                   byte_view_t<AesKeySize> key,
                                                   byte_view_t<AesBlockSize> iv)
{
    const ContextHolder context(EVP_CIPHER_CTX_new(), &EVP_CIPHER_CTX_free);
    if (!context) {
        qCCritical(E2EE)
            << "aesCtr256Decrypt() failed to create cipher context:"
            << ERR_error_string(ERR_get_error(), nullptr);
        Q_ASSERT(context);
        return ERR_get_error();
    }

    const auto [ciphertextSize, clamped] = checkedSize(ciphertext.size());
    if (clamped) {
        qCCritical(E2EE) << "The ciphertext is too long to be decrypted";
        Q_ASSERT(!clamped); // We just don't deal with such sizes in Matrix?
        return SslPayloadTooLong;
    }

    FixedBuffer<> decrypted(static_cast<size_t>(ciphertextSize),
                            FixedBufferBase::FillWithZeros);
    CALL_OPENSSL(EVP_DecryptInit_ex(context.get(), EVP_aes_256_ctr(), nullptr,
                                    key.data(), iv.data()));

    int decryptedLength = 0;
    CALL_OPENSSL(EVP_DecryptUpdate(
        context.get(), decrypted.data(), &decryptedLength,
        reinterpret_cast<const unsigned char*>(ciphertext.constData()),
        ciphertextSize));

    int tailLength = -1; // Normally becomes 0 after the next call
    CALL_OPENSSL(EVP_DecryptFinal_ex(
        context.get(),
        asWritableCBytes(decrypted).subspan(static_cast<size_t>(decryptedLength)).data(),
        &tailLength));

    return decrypted.copyToByteArray(decryptedLength + tailLength);
}

QOlmExpected<QByteArray> Quotient::curve25519AesSha2Decrypt(
    QByteArray ciphertext, const QByteArray& privateKey,
    const QByteArray& ephemeral, const QByteArray& mac)
{
    auto context = makeCStruct(olm_pk_decryption, olm_pk_decryption_size, olm_clear_pk_decryption);
    Q_ASSERT(context);

    // NB: The produced public key is not actually used, it's only a check
    if (std::vector<uint8_t> publicKey(olm_pk_key_length());
        olm_pk_key_from_private(context.get(), publicKey.data(),
                                publicKey.size(), privateKey.data(),
                                unsignedSize(privateKey))
        == olm_error())
        return olm_pk_decryption_last_error_code(context.get());

    auto plaintext = byteArrayForOlm(olm_pk_max_plaintext_length(context.get(), unsignedSize(ciphertext)));
    const auto resultSize = olm_pk_decrypt(context.get(), ephemeral.data(), unsignedSize(ephemeral), mac.data(), unsignedSize(mac), ciphertext.data(), unsignedSize(ciphertext), plaintext.data(), unsignedSize(plaintext));
    if (resultSize == olm_error())
        return olm_pk_decryption_last_error_code(context.get());

    const auto checkedResultSize = checkedSize(resultSize).first;
    plaintext.resize(checkedResultSize);
    return plaintext;
}

QOlmExpected<Curve25519Encrypted> Quotient::curve25519AesSha2Encrypt(
    const QByteArray& plaintext, const QByteArray& publicKey)
{
    auto context = makeCStruct(olm_pk_encryption, olm_pk_encryption_size,
                               olm_clear_pk_encryption);

    if (olm_pk_encryption_set_recipient_key(context.get(), publicKey.data(),
                                            unsignedSize(publicKey))
        == olm_error())
        return olm_pk_encryption_last_error_code(context.get());

    auto ephemeral = byteArrayForOlm(olm_pk_key_length());
    auto mac = byteArrayForOlm(olm_pk_mac_length(context.get()));
    auto ciphertext = byteArrayForOlm(
        olm_pk_ciphertext_length(context.get(), unsignedSize(plaintext)));

    const auto randomLength = olm_pk_encrypt_random_length(context.get());
    if (olm_pk_encrypt(context.get(), plaintext.data(), unsignedSize(plaintext),
                       ciphertext.data(), unsignedSize(ciphertext), mac.data(),
                       unsignedSize(mac), ephemeral.data(),
                       unsignedSize(ephemeral), getRandom(randomLength).data(),
                       randomLength)
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
    for (auto i = 0; i < 58; ++i) {
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

    for (auto i = 0; i < encoded.length() && encoded[i] == '1'; ++i) {
        result.push_back(u'\0');
    }

    std::reverse(result.begin(), result.end());
    return result;
}
