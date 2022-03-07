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
    QCOMPARE(data, decrypted);
}
QTEST_APPLESS_MAIN(TestFileCrypto)
