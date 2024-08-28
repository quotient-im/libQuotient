// SPDX-FileCopyrightText: 2023 Tobias Fella <tobias.fella@kde.org>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "cryptoutils.h"
#include "e2ee_common.h"

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

#include <source_location>

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
    if (uncheckedSize <= maxSize) [[likely]]
        return { static_cast<int>(uncheckedSize), false };

    qCCritical(E2EE) << "Cryptoutils:" << uncheckedSize
                     << "bytes is too many for OpenSSL, first" << maxSize
                     << "bytes will be taken";
    return { maxSize, true };
}

// NOLINTBEGIN(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
// TODO: remove NOLINT brackets once we're on clang-tidy 18
#define CLAMP_SIZE(SizeVar_, ByteArray_, ...)                                               \
    const auto [SizeVar_, ByteArray_##Clamped] =                                            \
        checkedSize((ByteArray_).size() __VA_OPT__(, ) __VA_ARGS__);                        \
    if (QUO_ALARM_X(ByteArray_##Clamped,                                                    \
                    u"" #ByteArray_                                                         \
                    " is %1 bytes long, too much for OpenSSL and overall suspicious"_s.arg( \
                        (ByteArray_).size())))                                              \
        return SslPayloadTooLong;                                                           \
    do {} while (false)                                                                     \
// End of macro

#define CALL_OPENSSL(Call_)                                                    \
    do {                                                                       \
        if ((Call_) <= 0) {                                                    \
            qCWarning(E2EE) << std::source_location::current().function_name() \
                            << "failed to call OpenSSL API:"                   \
                            << ERR_error_string(ERR_get_error(), nullptr);     \
            return ERR_get_error();                                            \
        }                                                                      \
    } while (false)                                                            \
    // End of macro
// NOLINTEND(cppcoreguidelines-pro-bounds-array-to-pointer-decay)

SslErrorCode Quotient::_impl::pbkdf2HmacSha512(const QByteArray& passphrase, const QByteArray& salt,
                                               int iterations, byte_span_t<> output)
{
    CLAMP_SIZE(passphraseSize, passphrase);
    CLAMP_SIZE(saltSize, salt);
    CLAMP_SIZE(outputSize, output);
    CALL_OPENSSL(PKCS5_PBKDF2_HMAC(passphrase.data(), passphraseSize, asCBytes(salt).data(),
                                   saltSize, iterations, EVP_sha512(), outputSize, output.data()));
    return 0; // OpenSSL doesn't have a special constant for success code :/
}

SslExpected<QByteArray> Quotient::aesCtr256Encrypt(const QByteArray& plaintext,
                                                   byte_view_t<Aes256KeySize> key,
                                                   byte_view_t<AesBlockSize> iv)
{
    CLAMP_SIZE(plaintextSize, plaintext);

    const ContextHolder ctx(EVP_CIPHER_CTX_new(), &EVP_CIPHER_CTX_free);
    if (QUO_ALARM_X(!ctx, QByteArrayLiteral("failed to create SSL context: ")
                          + ERR_error_string(ERR_get_error(), nullptr)))
        return ERR_get_error();

    QByteArray encrypted(plaintextSize + static_cast<int>(iv.size()),
                         Qt::Uninitialized);
    int encryptedLength = 0;
    {
        // Working with `encrypted` the span adaptor in this scope, avoiding reinterpret_casts
        auto encryptedSpan = asWritableCBytes(encrypted);
        fillFromSecureRng(encryptedSpan); // Now `encrypted` is initialised
        constexpr auto mask = static_cast<uint8_t>(~(1U << (63 / 8)));
        encryptedSpan[15 - 63 % 8] &= mask;

        CALL_OPENSSL(EVP_EncryptInit_ex(ctx.get(), EVP_aes_256_ctr(), nullptr,
                                        key.data(), iv.data()));

        CALL_OPENSSL(
            EVP_EncryptUpdate(ctx.get(), encryptedSpan.data(), &encryptedLength,
                              asCBytes(plaintext).data(), plaintextSize));
        Q_ASSERT(encryptedLength >= 0);

        int tailLength = -1;
        CALL_OPENSSL(EVP_EncryptFinal_ex(
            ctx.get(),
            encryptedSpan.subspan(static_cast<size_t>(encryptedLength)).data(),
            &tailLength));
        Q_ASSERT_X(tailLength == 0, std::source_location::current().function_name(),
                   "Encryption finalizer returned non-zero-size tail - this should not happen with "
                   "AES CTR algorithm.");
        encryptedLength += tailLength; // Recovery for Release builds
    }
    encrypted.resize(encryptedLength);
    return encrypted;
}

SslExpected<HkdfKeys> Quotient::hkdfSha256(byte_view_t<DefaultPbkdf2KeyLength> key,
                                           byte_view_t<32> salt, byte_view_t<> info)
{
    CLAMP_SIZE(infoSize, info);

    HkdfKeys result;
    const ContextHolder context(EVP_PKEY_CTX_new_id(EVP_PKEY_HKDF, nullptr),
                                &EVP_PKEY_CTX_free);

    CALL_OPENSSL(EVP_PKEY_derive_init(context.get()));
    CALL_OPENSSL(EVP_PKEY_CTX_set_hkdf_md(context.get(), EVP_sha256()));
    CALL_OPENSSL(EVP_PKEY_CTX_set1_hkdf_salt(context.get(), salt.data(), salt.size()));
    CALL_OPENSSL(EVP_PKEY_CTX_set1_hkdf_key(context.get(), key.data(), key.size()));
    CALL_OPENSSL(EVP_PKEY_CTX_add1_hkdf_info(context.get(), info.data(), infoSize));
    size_t outputLength = result.size();
    CALL_OPENSSL(EVP_PKEY_derive(context.get(), result.data(), &outputLength));
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
    if (HMAC(EVP_sha256(), hmacKey.data(), hmacKey.size(), asCBytes(data).data(),
             unsignedSize(data), asWritableCBytes(output).data(), &len)
        == nullptr) {
        qWarning() << ERR_error_string(ERR_get_error(), nullptr);
        return ERR_get_error();
    }
    return output;
}

SslExpected<QByteArray> Quotient::aesCtr256Decrypt(const QByteArray& ciphertext,
                                                   byte_view_t<Aes256KeySize> key,
                                                   byte_view_t<AesBlockSize> iv)
{
    CLAMP_SIZE(ciphertextSize, ciphertext);

    const ContextHolder context(EVP_CIPHER_CTX_new(), &EVP_CIPHER_CTX_free);
    if (!context) {
        qCCritical(E2EE)
            << "aesCtr256Decrypt() failed to create cipher context:"
            << ERR_error_string(ERR_get_error(), nullptr);
        Q_ASSERT(context);
        return ERR_get_error();
    }

    auto decrypted = zeroedByteArray(ciphertextSize);
    int decryptedLength = 0;
    {
        const auto decryptedSpan = asWritableCBytes(decrypted);
        CALL_OPENSSL(EVP_DecryptInit_ex(context.get(), EVP_aes_256_ctr(),
                                        nullptr, key.data(), iv.data()));
        CALL_OPENSSL(EVP_DecryptUpdate(context.get(), decryptedSpan.data(),
                                       &decryptedLength,
                                       asCBytes(ciphertext).data(),
                                       ciphertextSize));
        int tailLength = -1;
        CALL_OPENSSL(
            EVP_DecryptFinal_ex(context.get(),
                                decryptedSpan.subspan(static_cast<size_t>(decryptedLength)).data(),
                                &tailLength));
        Q_ASSERT_X(tailLength == 0, std::source_location::current().function_name(),
                   "Decrypt operation finalizer returned non-zero-size tail - this should not "
                   "happen with AES CTR algorithm.");
        decryptedLength += tailLength;
    }
    decrypted.resize(decryptedLength);
    return decrypted;
}

QOlmExpected<QByteArray> Quotient::curve25519AesSha2Decrypt(
    QByteArray ciphertext, const QByteArray& privateKey,
    const QByteArray& ephemeral, const QByteArray& mac)
{
    auto context = makeCStruct(olm_pk_decryption, olm_pk_decryption_size, olm_clear_pk_decryption);
    Q_ASSERT(context);

    // NB: The produced public key is not actually used, the call is only
    //     to fill the context with the private key for olm_pk_decrypt()
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

std::vector<byte_t> Quotient::base58Decode(const QByteArray& encoded)
{
    // See https://spec.matrix.org/latest/client-server-api/#recovery-key
    constexpr auto reverse_alphabet = []() consteval {
        std::array<byte_t, 256> init; // NOLINT(cppcoreguidelines-pro-type-member-init)
        init.fill(0xFF);
        const std::array<byte_t, 59> alphabet{
            "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz"
        };
        // NOLINTNEXTLINE(bugprone-too-small-loop-variable)
        for (uint8_t i = 0; i < std::size(alphabet) - 1; ++i) {
            init[alphabet[i]] = i;
        }
        return init;
    }();

    std::vector<byte_t> result;
    result.reserve(unsignedSize(encoded) * 733 / 1000 + 1);

    for (const auto b : encoded) {
        uint32_t carry = reverse_alphabet[static_cast<byte_t>(b)];
        for (auto& j : result) {
            carry += j * 58;
            j = carry & 0xFF;
            carry /= 0x100;
        }
        while (carry > 0) {
            result.push_back(carry & 0xFF);
            carry /= 0x100;
        }
    }

    for (auto i = 0; i < encoded.size() && encoded[i] == '1'; ++i) {
        result.push_back(0x0);
    }

    std::reverse(result.begin(), result.end());
    return result;
}

QByteArray Quotient::sign(const QByteArray& key, const QByteArray& data)
{
    auto context = makeCStruct(olm_pk_signing, olm_pk_signing_size, olm_clear_pk_signing);
    QByteArray pubKey(olm_pk_signing_public_key_length(), 0);
    olm_pk_signing_key_from_seed(context.get(), pubKey.data(), unsignedSize(pubKey), key.data(),
                                 unsignedSize(key));
    Q_ASSERT(context);

    const auto signatureLength = olm_pk_signature_length();
    auto signatureBuffer = byteArrayForOlm(signatureLength);

    if (olm_pk_sign(context.get(), asCBytes(data).data(), unsignedSize(data), asWritableCBytes(signatureBuffer).data(), signatureLength)
        == olm_error())
        QOLM_INTERNAL_ERROR_X("Failed to sign a message", olm_pk_signing_last_error(context.get()));

    return signatureBuffer;
}
