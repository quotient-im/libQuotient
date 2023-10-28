// SPDX-FileCopyrightText: 2022 Tobias Fella <fella@posteo.de>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include <Quotient/e2ee/cryptoutils.h>
#include <Quotient/e2ee/e2ee_common.h>

#include <Quotient/events/filesourceinfo.h>

#include <QTest>

#include <olm/pk.h>

class TestCryptoUtils : public QObject
{
    Q_OBJECT
private slots:
    void aesCtrEncryptDecryptData();
    void hkdfSha256ExpandKeys();
    void encryptDecryptFile();
    void pbkdfGenerateKey();
    void hmac();
    void curve25519AesEncryptDecrypt();
    void decodeBase58();
};

namespace {
inline QByteArray zeroedByteArray(int n = 32) { return { n, u'\0' }; }
}

using namespace Quotient;

void TestCryptoUtils::aesCtrEncryptDecryptData()
{
    const QByteArray plain = "ABCDEF";
    const auto key = zeroedByteArray();
    const auto iv = zeroedByteArray();
    auto cipher = aesCtr256Encrypt(plain, key, iv);
    auto decrypted = aesCtr256Decrypt(cipher, key, iv);
    QCOMPARE(plain, decrypted);
}

void TestCryptoUtils::encryptDecryptFile()
{
    const QByteArray data = "ABCDEF";
    auto [file, cipherText] = encryptFile(data);
    auto decrypted = decryptFile(cipherText, file);
    // AES CTR produces ciphertext of the same size as the original
    QCOMPARE(cipherText.size(), data.size());
    QCOMPARE(decrypted.size(), data.size());
    QCOMPARE(decrypted, data);
}

void TestCryptoUtils::hkdfSha256ExpandKeys()
{
    auto keys = hkdfSha256(zeroedByteArray(), zeroedByteArray(), zeroedByteArray());
    QCOMPARE(keys.aes, QByteArray::fromBase64("WQvd7OvHEaSFkO5nPBLDHK9F0UW5r11S6MS83AjhHx8="));
    QCOMPARE(keys.mac, QByteArray::fromBase64("hZhUYGZQRYj4src+HzLcKRruQQ0wSr9kC/g105lej+s="));
}

void TestCryptoUtils::pbkdfGenerateKey()
{
    auto key = pbkdf2HmacSha512(QByteArrayLiteral("PASSWORD"), zeroedByteArray(32), 50000);
    QCOMPARE(key, QByteArray::fromBase64("ejq90XW/J2J+cgi1ASgBj94M/YrEtWRKAPnsG+rdG4w="));
}

void TestCryptoUtils::hmac()
{
    auto hmac = hmacSha256(zeroedByteArray(),  QByteArray(64, 1));
    QCOMPARE(hmac, QByteArray::fromBase64("GfJTpEMByWSMA/NXBYH/KHW2qlKxSZu4r//jRsUuz24="));
}

void TestCryptoUtils::curve25519AesEncryptDecrypt()
{
    const auto plain = QByteArrayLiteral("ABCDEF");
    auto privateKey = zeroedByteArray();

    auto context = makeCStruct(olm_pk_decryption, olm_pk_decryption_size,
                               olm_clear_pk_decryption);
    const auto publicKeySize = olm_pk_key_length();
    auto publicKey = byteArrayForOlm(publicKeySize);
    olm_pk_key_from_private(context.get(), publicKey.data(), publicKeySize, privateKey.data(), unsignedSize(privateKey));

    auto encrypted = curve25519AesSha2Encrypt(plain, publicKey);
    auto decrypted = curve25519AesSha2Decrypt(encrypted.ciphertext, privateKey, encrypted.ephemeral, encrypted.mac);
    QCOMPARE(plain, decrypted);
}

void TestCryptoUtils::decodeBase58()
{
    QCOMPARE(base58Decode(QByteArrayLiteral("ABCDEFabcdef")).toBase64(), QByteArrayLiteral("DG3GmkxFR1TQ"));
}

QTEST_APPLESS_MAIN(TestCryptoUtils)
#include "testcryptoutils.moc"
