// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "filesourceinfo.h"

#include "../logging_categories_p.h"

#include "../util.h"

#include <QtCore/QReadWriteLock>

#ifdef Quotient_E2EE_ENABLED
#    include <Quotient/e2ee/e2ee_common.h>

#    include <QtCore/QCryptographicHash>
#    include "e2ee/cryptoutils.h"
#    include <openssl/evp.h>
#endif

using namespace Quotient;

#ifdef Quotient_E2EE_ENABLED
QByteArray Quotient::decryptFile(const QByteArray& ciphertext,
                                 const EncryptedFileMetadata& metadata)
{
    if (QByteArray::fromBase64(metadata.hashes["sha256"_ls].toLatin1())
        != QCryptographicHash::hash(ciphertext, QCryptographicHash::Sha256)) {
        qCWarning(E2EE) << "Hash verification failed for file";
        return {};
    }

    auto _key = metadata.key.k;
    const auto keyBytes = QByteArray::fromBase64(
        _key.replace(u'_', u'/').replace(u'-', u'+').toLatin1());

    return aesCtr256Decrypt(ciphertext, keyBytes, QByteArray::fromBase64(metadata.iv.toLatin1()));
}

std::pair<EncryptedFileMetadata, QByteArray> Quotient::encryptFile(
    const QByteArray& plainText)
{
    auto k = getRandom<32>();
    auto kBase64 = k.toBase64(QByteArray::Base64UrlEncoding
                              | QByteArray::OmitTrailingEquals);
    auto iv = getRandom<16>();
    const JWK key = {
        "oct"_ls, { "encrypt"_ls, "decrypt"_ls }, "A256CTR"_ls, QString::fromLatin1(kBase64), true
    };
    auto cipherText = aesCtr256Encrypt(plainText, k.viewAsByteArray(), iv.viewAsByteArray());

    auto hash = QCryptographicHash::hash(cipherText, QCryptographicHash::Sha256)
                    .toBase64(QByteArray::OmitTrailingEquals);
    auto ivBase64 = iv.toBase64(QByteArray::OmitTrailingEquals);
    const EncryptedFileMetadata efm = {
        {}, key, QString::fromLatin1(ivBase64), { { QStringLiteral("sha256"), QString::fromLatin1(hash) } }, "v2"_ls
    };
    return { efm, cipherText };
}
#endif

void JsonObjectConverter<EncryptedFileMetadata>::dumpTo(
    QJsonObject& jo, const EncryptedFileMetadata& pod)
{
    addParam<>(jo, QStringLiteral("url"), pod.url);
    addParam<>(jo, QStringLiteral("key"), pod.key);
    addParam<>(jo, QStringLiteral("iv"), pod.iv);
    addParam<>(jo, QStringLiteral("hashes"), pod.hashes);
    addParam<>(jo, QStringLiteral("v"), pod.v);
}

void JsonObjectConverter<EncryptedFileMetadata>::fillFrom(
    const QJsonObject& jo, EncryptedFileMetadata& pod)
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
