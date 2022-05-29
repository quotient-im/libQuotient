// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "filesourceinfo.h"

#include "logging.h"

#ifdef Quotient_E2EE_ENABLED
#    include "e2ee/qolmutils.h"

#    include <QtCore/QCryptographicHash>

#    include <openssl/evp.h>
#endif

using namespace Quotient;

QByteArray Quotient::decryptFile(const QByteArray& ciphertext,
                                 const EncryptedFileMetadata& metadata)
{
#ifdef Quotient_E2EE_ENABLED
    if (QByteArray::fromBase64(metadata.hashes["sha256"_ls].toLatin1())
        != QCryptographicHash::hash(ciphertext, QCryptographicHash::Sha256)) {
        qCWarning(E2EE) << "Hash verification failed for file";
        return {};
    }

    auto _key = metadata.key.k;
    const auto keyBytes = QByteArray::fromBase64(
        _key.replace(u'_', u'/').replace(u'-', u'+').toLatin1());
    int length;
    auto* ctx = EVP_CIPHER_CTX_new();
    QByteArray plaintext(ciphertext.size() + EVP_MAX_BLOCK_LENGTH - 1, '\0');
    EVP_DecryptInit_ex(
        ctx, EVP_aes_256_ctr(), nullptr,
        reinterpret_cast<const unsigned char*>(keyBytes.data()),
        reinterpret_cast<const unsigned char*>(
            QByteArray::fromBase64(metadata.iv.toLatin1()).data()));
    EVP_DecryptUpdate(ctx, reinterpret_cast<unsigned char*>(plaintext.data()),
                      &length,
                      reinterpret_cast<const unsigned char*>(ciphertext.data()),
                      ciphertext.size());
    EVP_DecryptFinal_ex(ctx,
                        reinterpret_cast<unsigned char*>(plaintext.data())
                            + length,
                        &length);
    EVP_CIPHER_CTX_free(ctx);
    return plaintext.left(ciphertext.size());
#else
    qWarning(MAIN) << "This build of libQuotient doesn't support E2EE, "
                      "cannot decrypt the file";
    return ciphertext;
#endif
}

std::pair<EncryptedFileMetadata, QByteArray> Quotient::encryptFile(
    const QByteArray& plainText)
{
#ifdef Quotient_E2EE_ENABLED
    QByteArray k = getRandom(32);
    auto kBase64 = k.toBase64();
    QByteArray iv = getRandom(16);
    JWK key = { "oct"_ls,
                { "encrypt"_ls, "decrypt"_ls },
                "A256CTR"_ls,
                QString(k.toBase64())
                    .replace(u'/', u'_')
                    .replace(u'+', u'-')
                    .left(kBase64.indexOf('=')),
                true };

    int length;
    auto* ctx = EVP_CIPHER_CTX_new();
    EVP_EncryptInit_ex(ctx, EVP_aes_256_ctr(), nullptr,
                       reinterpret_cast<const unsigned char*>(k.data()),
                       reinterpret_cast<const unsigned char*>(iv.data()));
    const auto blockSize = EVP_CIPHER_CTX_block_size(ctx);
    QByteArray cipherText(plainText.size() + blockSize - 1, '\0');
    EVP_EncryptUpdate(ctx, reinterpret_cast<unsigned char*>(cipherText.data()),
                      &length,
                      reinterpret_cast<const unsigned char*>(plainText.data()),
                      plainText.size());
    EVP_EncryptFinal_ex(ctx,
                        reinterpret_cast<unsigned char*>(cipherText.data())
                            + length,
                        &length);
    EVP_CIPHER_CTX_free(ctx);

    auto hash = QCryptographicHash::hash(cipherText, QCryptographicHash::Sha256)
                    .toBase64();
    auto ivBase64 = iv.toBase64();
    EncryptedFileMetadata efm = { {},
                           key,
                           ivBase64.left(ivBase64.indexOf('=')),
                           { { QStringLiteral("sha256"),
                               hash.left(hash.indexOf('=')) } },
                           "v2"_ls };
    return { efm, cipherText };
#else
    return {};
#endif
}

void JsonObjectConverter<EncryptedFileMetadata>::dumpTo(QJsonObject& jo,
                                                const EncryptedFileMetadata& pod)
{
    addParam<>(jo, QStringLiteral("url"), pod.url);
    addParam<>(jo, QStringLiteral("key"), pod.key);
    addParam<>(jo, QStringLiteral("iv"), pod.iv);
    addParam<>(jo, QStringLiteral("hashes"), pod.hashes);
    addParam<>(jo, QStringLiteral("v"), pod.v);
}

void JsonObjectConverter<EncryptedFileMetadata>::fillFrom(const QJsonObject& jo,
                                                  EncryptedFileMetadata& pod)
{
    fromJson(jo.value("url"_ls), pod.url);
    fromJson(jo.value("key"_ls), pod.key);
    fromJson(jo.value("iv"_ls), pod.iv);
    fromJson(jo.value("hashes"_ls), pod.hashes);
    fromJson(jo.value("v"_ls), pod.v);
}

void JsonObjectConverter<JWK>::dumpTo(QJsonObject& jo, const JWK& pod)
{
    addParam<>(jo, QStringLiteral("kty"), pod.kty);
    addParam<>(jo, QStringLiteral("key_ops"), pod.keyOps);
    addParam<>(jo, QStringLiteral("alg"), pod.alg);
    addParam<>(jo, QStringLiteral("k"), pod.k);
    addParam<>(jo, QStringLiteral("ext"), pod.ext);
}

void JsonObjectConverter<JWK>::fillFrom(const QJsonObject& jo, JWK& pod)
{
    fromJson(jo.value("kty"_ls), pod.kty);
    fromJson(jo.value("key_ops"_ls), pod.keyOps);
    fromJson(jo.value("alg"_ls), pod.alg);
    fromJson(jo.value("k"_ls), pod.k);
    fromJson(jo.value("ext"_ls), pod.ext);
}

template <typename... FunctorTs>
struct Overloads : FunctorTs... {
    using FunctorTs::operator()...;
};

template <typename... FunctorTs>
Overloads(FunctorTs&&...) -> Overloads<FunctorTs...>;

QUrl Quotient::getUrlFromSourceInfo(const FileSourceInfo& fsi)
{
    return std::visit(Overloads { [](const QUrl& url) { return url; },
                                  [](const EncryptedFileMetadata& efm) {
                                      return efm.url;
                                  } },
                      fsi);
}

void Quotient::setUrlInSourceInfo(FileSourceInfo& fsi, const QUrl& newUrl)
{
    std::visit(Overloads { [&newUrl](QUrl& url) { url = newUrl; },
                           [&newUrl](EncryptedFileMetadata& efm) {
                               efm.url = newUrl;
                           } },
               fsi);
}

void Quotient::fillJson(QJsonObject& jo,
                        const std::array<QLatin1String, 2>& jsonKeys,
                        const FileSourceInfo& fsi)
{
    // NB: Keeping variant_size_v out of the function signature for readability.
    // NB2: Can't use jsonKeys directly inside static_assert as its value is
    // unknown so the compiler cannot ensure size() is constexpr (go figure...)
    static_assert(
        std::variant_size_v<FileSourceInfo> == decltype(jsonKeys) {}.size());
    jo.insert(jsonKeys[fsi.index()], toJson(fsi));
}
