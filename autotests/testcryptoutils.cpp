// SPDX-FileCopyrightText: 2022 Tobias Fella <fella@posteo.de>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include <Quotient/connection.h>
#include <Quotient/database.h>
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
    void testEncrypted();
};

using namespace Quotient;

void TestCryptoUtils::aesCtrEncryptDecryptData()
{
    const QByteArray plain = "ABCDEF";
    const FixedBuffer<AesKeySize> key{};
    const FixedBuffer<AesBlockSize> iv{};
    auto cipher = aesCtr256Encrypt(plain, key, iv);
    QVERIFY(cipher.has_value());
    auto decrypted = aesCtr256Decrypt(cipher.value(), key, iv);
    QVERIFY(decrypted.has_value());
    QCOMPARE(plain, decrypted.value());
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
    auto result = hkdfSha256(zeroedByteArray(), zeroedByteArray(), zeroedByteArray());
    QVERIFY(result.has_value());
    auto&& keys = result.value();
    QCOMPARE(viewAsByteArray(keys.aes()), QByteArray::fromBase64("WQvd7OvHEaSFkO5nPBLDHK9F0UW5r11S6MS83AjhHx8="));
    QCOMPARE(viewAsByteArray(keys.mac()), QByteArray::fromBase64("hZhUYGZQRYj4src+HzLcKRruQQ0wSr9kC/g105lej+s="));
}

void TestCryptoUtils::pbkdfGenerateKey()
{
    auto key = pbkdf2HmacSha512(QByteArrayLiteral("PASSWORD"), zeroedByteArray(32), 50000);
    QVERIFY(key.has_value());
    QCOMPARE(key.value(), QByteArray::fromBase64("ejq90XW/J2J+cgi1ASgBj94M/YrEtWRKAPnsG+rdG4w="));
}

void TestCryptoUtils::hmac()
{
    auto result = hmacSha256(FixedBuffer<HmacKeySize>{}, QByteArray(64, 1));
    QVERIFY(result.has_value());
    QCOMPARE(result.value(), QByteArray::fromBase64("GfJTpEMByWSMA/NXBYH/KHW2qlKxSZu4r//jRsUuz24="));
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
    QVERIFY(encrypted.has_value());
    auto decrypted = curve25519AesSha2Decrypt(encrypted.value().ciphertext, privateKey, encrypted.value().ephemeral, encrypted.value().mac);
    QVERIFY(decrypted.has_value());
    QCOMPARE(plain, decrypted.value());
}

void TestCryptoUtils::decodeBase58()
{
    QCOMPARE(base58Decode(QByteArrayLiteral("ABCDEFabcdef")).toBase64(), QByteArrayLiteral("DG3GmkxFR1TQ"));
}

void TestCryptoUtils::testEncrypted()
{
    QByteArray key(32, '\0');
    auto text = QByteArrayLiteral("This is a message");
    auto connection = Connection::makeMockConnection("@foo:bar.com"_ls, true);
    connection->database()->storeEncrypted("testKey"_ls, text);
    auto decrypted = connection->database()->loadEncrypted("testKey"_ls);
    QCOMPARE(text, decrypted);
}

QTEST_GUILESS_MAIN(TestCryptoUtils)
#include "testcryptoutils.moc"
