// SPDX-FileCopyrightText: 2022 Tobias Fella <fella@posteo.de>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "testcryptoutils.h"

#include <Quotient/events/filesourceinfo.h>
#include <Quotient/e2ee/cryptoutils.h>

#include <olm/pk.h>

#include <qtest.h>

using namespace Quotient;

void TestCryptoUtils::aesCtrEncryptDecryptData()
{
    QByteArray plain = "ABCDEF";
    QByteArray key(32, u'\0');
    QByteArray iv(32, u'\0');
    auto cipher = aesCtr256Encrypt(plain, key, iv);
    auto decrypted = aesCtr256Decrypt(cipher, key, iv);
    QCOMPARE(plain, decrypted);
}

void TestCryptoUtils::encryptDecryptFile()
{
    QByteArray data = "ABCDEF";
    auto [file, cipherText] = encryptFile(data);
    auto decrypted = decryptFile(cipherText, file);
    // AES CTR produces ciphertext of the same size as the original
    QCOMPARE(cipherText.size(), data.size());
    QCOMPARE(decrypted.size(), data.size());
    QCOMPARE(decrypted, data);
}

void TestCryptoUtils::hkdfSha256ExpandKeys()
{
    auto keys = hkdfSha256(QByteArray(32, u'\0'), QByteArray(32, u'\0'), QByteArray(32, u'\0'));
    QCOMPARE(keys.aes, QByteArray::fromBase64("WQvd7OvHEaSFkO5nPBLDHK9F0UW5r11S6MS83AjhHx8="));
    QCOMPARE(keys.mac, QByteArray::fromBase64("hZhUYGZQRYj4src+HzLcKRruQQ0wSr9kC/g105lej+s="));
}

void TestCryptoUtils::pbkdfGenerateKey()
{
    auto key = pbkdf2HmacSha512(QStringLiteral("PASSWORD").toLatin1(), QByteArray(32, u'\0'), 50000, 32);
    QCOMPARE(key, QByteArray::fromBase64("ejq90XW/J2J+cgi1ASgBj94M/YrEtWRKAPnsG+rdG4w="));
}

void TestCryptoUtils::hmac()
{
    auto hmac = hmacSha256(QByteArray(32, 0),  QByteArray(64, 1));
    QCOMPARE(hmac, QByteArray::fromBase64("GfJTpEMByWSMA/NXBYH/KHW2qlKxSZu4r//jRsUuz24="));
}

void TestCryptoUtils::curve25519AesEncryptDecrypt()
{
    auto plain = QByteArrayLiteral("ABCDEF");
    auto privateKey = QByteArray(32, 0);

    auto context = makeCStruct(olm_pk_decryption, olm_pk_decryption_size, olm_clear_pk_decryption);
    QByteArray publicKey(olm_pk_key_length(), 0);
    olm_pk_key_from_private(context.get(), publicKey.data(), publicKey.size(), privateKey.data(), privateKey.size());

    auto encrypted = curve25519AesSha2Encrypt(plain, publicKey);
    auto decrypted = curve25519AesSha2Decrypt(encrypted.ciphertext, privateKey, encrypted.ephemeral, encrypted.mac);
    QCOMPARE(plain, decrypted);
}

void TestCryptoUtils::decodeBase58()
{
    QCOMPARE(base58Decode(QByteArrayLiteral("ABCDEFabcdef")).toBase64(), QByteArrayLiteral("DG3GmkxFR1TQ"));
}

QTEST_APPLESS_MAIN(TestCryptoUtils)
