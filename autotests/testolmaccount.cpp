// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifdef Quotient_E2EE_ENABLED
#include "testolmaccount.h"
#include "olm/qolmaccount.h"

void TestOlmAccount::pickleUnpickedTest()
{
    auto olmAccount = QOlmAccount::create().value();
    auto identityKeys = std::get<IdentityKeys>(olmAccount.identityKeys());
    auto pickled = std::get<QByteArray>(olmAccount.pickle(Unencrypted{}));
    auto olmAccount2 = std::get<QOlmAccount>(QOlmAccount::unpickle(pickled, Unencrypted{}));
    auto identityKeys2 = std::get<IdentityKeys>(olmAccount2.identityKeys());
    QCOMPARE(identityKeys.curve25519, identityKeys2.curve25519);
    QCOMPARE(identityKeys.ed25519, identityKeys2.ed25519);
}

void TestOlmAccount::identityKeysValid()
{
    auto olmAccount = QOlmAccount::create().value();
    const auto identityKeys = std::get<IdentityKeys>(olmAccount.identityKeys());
    const auto curve25519 = identityKeys.curve25519;
    const auto ed25519 = identityKeys.ed25519;
    // verify encoded keys length
    QCOMPARE(curve25519.size(), 43);
    QCOMPARE(ed25519.size(), 43);

    // encoded as valid base64?
    QVERIFY(QByteArray::fromBase64Encoding(curve25519).decodingStatus == QByteArray::Base64DecodingStatus::Ok);
    QVERIFY(QByteArray::fromBase64Encoding(ed25519).decodingStatus == QByteArray::Base64DecodingStatus::Ok);
}

void TestOlmAccount::signatureValid()
{
    const auto olmAccount = QOlmAccount::create().value();
    const auto message = "Hello world!";
    const auto signature = std::get<QString>(olmAccount.sign(message));
    QVERIFY(QByteArray::fromBase64Encoding(signature.toUtf8()).decodingStatus == QByteArray::Base64DecodingStatus::Ok);

    //let utility = OlmUtility::new();
    //let identity_keys = olm_account.parsed_identity_keys();
    //let ed25519_key = identity_keys.ed25519();
    //assert!(utility
    //    .ed25519_verify(&ed25519_key, message, &signature)
    //    .unwrap());
}

void TestOlmAccount::oneTimeKeysValid()
{
    const auto olmAccount = QOlmAccount::create().value();
    const auto maxNumberOfOneTimeKeys = olmAccount.maxNumberOfOneTimeKeys();
    QCOMPARE(100, maxNumberOfOneTimeKeys);

    const auto oneTimeKeysEmpty = std::get<OneTimeKeys>(olmAccount.oneTimeKeys());
    QVERIFY(oneTimeKeysEmpty.curve25519().isEmpty());

    olmAccount.generateOneTimeKeys(20);
    const auto oneTimeKeysFilled = std::get<OneTimeKeys>(olmAccount.oneTimeKeys());
    QCOMPARE(20, oneTimeKeysFilled.curve25519().count());
}

QTEST_MAIN(TestOlmAccount)
#endif
