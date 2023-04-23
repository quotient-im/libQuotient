// SPDX-FileCopyrightText: 2022 Tobias Fella <fella@posteo.de>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "testfilecrypto.h"

#include <Quotient/events/filesourceinfo.h>

#include <qtest.h>

using namespace Quotient;
void TestFileCrypto::encryptDecryptData()
{
    QByteArray data = "ABCDEF";
    auto [file, cipherText] = encryptFile(data);
    auto decrypted = decryptFile(cipherText, file);
    // AES CTR produces ciphertext of the same size as the original
    QCOMPARE(cipherText.size(), data.size());
    QCOMPARE(decrypted.size(), data.size());
    QCOMPARE(decrypted, data);
}
QTEST_APPLESS_MAIN(TestFileCrypto)
