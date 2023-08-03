// SPDX-FileCopyrightText: 2022 Tobias Fella <fella@posteo.de>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include <QtTest/QtTest>

class TestCryptoUtils : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void aesCtrEncryptDecryptData();
    void hkdfSha256ExpandKeys();
    void encryptDecryptFile();
    void pbkdfGenerateKey();
    void hmac();
    void curve25519AesEncryptDecrypt();

};
