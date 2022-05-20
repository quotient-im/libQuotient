// SPDX-FileCopyrightText: 2022 Tobias Fella <fella@posteo.de>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "testfilecrypto.h"
#include "events/encryptedfile.h"
#include <qtest.h>

using namespace Quotient;
void TestFileCrypto::encryptDecryptData()
{
    QByteArray data = "ABCDEF";
    auto [file, cipherText] = EncryptedFile::encryptFile(data);
    auto decrypted = file.decryptFile(cipherText);
    // AES CTR produces ciphertext of the same size as the original
    QCOMPARE(cipherText.size(), data.size());
    QCOMPARE(decrypted.size(), data.size());
    QCOMPARE(decrypted, data);
}
QTEST_APPLESS_MAIN(TestFileCrypto)
