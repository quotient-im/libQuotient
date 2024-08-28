// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "filesourceinfo.h"

#include "../e2ee/cryptoutils.h"
#include "../e2ee/e2ee_common.h"
#include "../logging_categories_p.h"
#include "../util.h"

#include <QtCore/QReadWriteLock>
#include <QtCore/QCryptographicHash>

using namespace Quotient;

QByteArray Quotient::decryptFile(const QByteArray& ciphertext,
                                 const EncryptedFileMetadata& metadata)
{
    if (QByteArray::fromBase64(metadata.hashes["sha256"_L1].toLatin1())
        != QCryptographicHash::hash(ciphertext, QCryptographicHash::Sha256)) {
        qCWarning(E2EE) << "Hash verification failed for file";
        return {};
    }
    const auto key = QByteArray::fromBase64(metadata.key.k.toLatin1(),
                                            QByteArray::Base64UrlEncoding);
    if (key.size() < Aes256KeySize) {
        qCWarning(E2EE) << "Decoded key is too short for AES, need"
                        << Aes256KeySize << "bytes, got" << key.size();
        return {};
    }
    const auto iv = QByteArray::fromBase64(metadata.iv.toLatin1());
    if (iv.size() < AesBlockSize) {
        qCWarning(E2EE) << "Decoded iv is too short for AES, need"
                        << AesBlockSize << "bytes, got" << iv.size();
        return {};
    }

    return aesCtr256Decrypt(ciphertext, asCBytes<32>(key), asCBytes<16>(iv))
        .move_value_or({});
}

std::pair<EncryptedFileMetadata, QByteArray> Quotient::encryptFile(
    const QByteArray& plainText)
{
    auto k = getRandom<Aes256KeySize>();
    auto kBase64 = k.toBase64(QByteArray::Base64UrlEncoding
                              | QByteArray::OmitTrailingEquals);
    auto iv = getRandom<AesBlockSize>();
    const JWK key = {
        "oct"_L1, { "encrypt"_L1, "decrypt"_L1 }, "A256CTR"_L1, QString::fromLatin1(kBase64), true
    };
    auto result = aesCtr256Encrypt(plainText, k, iv);
    if (!result.has_value())
        return {};

    auto hash = QCryptographicHash::hash(result.value(), QCryptographicHash::Sha256)
                    .toBase64(QByteArray::OmitTrailingEquals);
    auto ivBase64 = iv.toBase64(QByteArray::OmitTrailingEquals);
    const EncryptedFileMetadata efm = {
        {}, key, QString::fromLatin1(ivBase64),
        { { "sha256"_L1, QString::fromLatin1(hash) } }, "v2"_L1
    };
    return { efm, result.value() };
}

void JsonObjectConverter<EncryptedFileMetadata>::dumpTo(
    QJsonObject& jo, const EncryptedFileMetadata& pod)
{
    addParam<>(jo, "url"_L1, pod.url);
    addParam<>(jo, "key"_L1, pod.key);
    addParam<>(jo, "iv"_L1, pod.iv);
    addParam<>(jo, "hashes"_L1, pod.hashes);
    addParam<>(jo, "v"_L1, pod.v);
}

void JsonObjectConverter<EncryptedFileMetadata>::fillFrom(
    const QJsonObject& jo, EncryptedFileMetadata& pod)
{
    fromJson(jo.value("url"_L1), pod.url);
    fromJson(jo.value("key"_L1), pod.key);
    fromJson(jo.value("iv"_L1), pod.iv);
    fromJson(jo.value("hashes"_L1), pod.hashes);
    fromJson(jo.value("v"_L1), pod.v);
}

void JsonObjectConverter<JWK>::dumpTo(QJsonObject& jo, const JWK& pod)
{
    addParam<>(jo, "kty"_L1, pod.kty);
    addParam<>(jo, "key_ops"_L1, pod.keyOps);
    addParam<>(jo, "alg"_L1, pod.alg);
    addParam<>(jo, "k"_L1, pod.k);
    addParam<>(jo, "ext"_L1, pod.ext);
}

void JsonObjectConverter<JWK>::fillFrom(const QJsonObject& jo, JWK& pod)
{
    fromJson(jo.value("kty"_L1), pod.kty);
    fromJson(jo.value("key_ops"_L1), pod.keyOps);
    fromJson(jo.value("alg"_L1), pod.alg);
    fromJson(jo.value("k"_L1), pod.k);
    fromJson(jo.value("ext"_L1), pod.ext);
}

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

namespace {
    // A map from roomId/eventId pair to file source info
    QHash<std::pair<QString, QString>, EncryptedFileMetadata> infos;
    QReadWriteLock lock;
}

void FileMetadataMap::add(const QString& roomId, const QString& eventId,
                          const EncryptedFileMetadata& fileMetadata)
{
    const QWriteLocker l(&lock);
    infos.insert({ roomId, eventId }, fileMetadata);
}

void FileMetadataMap::remove(const QString& roomId, const QString& eventId)
{
    const QWriteLocker l(&lock);
    infos.remove({ roomId, eventId });
}

EncryptedFileMetadata FileMetadataMap::lookup(const QString& roomId,
                                              const QString& eventId)
{
    const QReadLocker l(&lock);
    return infos.value({ roomId, eventId });
}
