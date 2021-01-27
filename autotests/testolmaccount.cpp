// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "testolmaccount.h"
#include "olm/qolmaccount.h"

using namespace Quotient;

void TestOlmAccount::pickleUnpickedTest()
{
    QOlmAccount olmAccount(QStringLiteral("@foo:bar.com"), QStringLiteral("QuotientTestDevice"));
    olmAccount.createNewAccount();
    auto identityKeys = olmAccount.identityKeys();
    auto pickled = std::get<QByteArray>(olmAccount.pickle(Unencrypted{}));
    QOlmAccount olmAccount2(QStringLiteral("@foo:bar.com"), QStringLiteral("QuotientTestDevice"));
    olmAccount2.unpickle(pickled, Unencrypted{});
    auto identityKeys2 = olmAccount2.identityKeys();
    QCOMPARE(identityKeys.curve25519, identityKeys2.curve25519);
    QCOMPARE(identityKeys.ed25519, identityKeys2.ed25519);
}

void TestOlmAccount::identityKeysValid()
{
    QOlmAccount olmAccount(QStringLiteral("@foo:bar.com"), QStringLiteral("QuotientTestDevice"));
    olmAccount.createNewAccount();
    const auto identityKeys = olmAccount.identityKeys();
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
    QOlmAccount olmAccount(QStringLiteral("@foo:bar.com"), QStringLiteral("QuotientTestDevice"));
    olmAccount.createNewAccount();
    const auto message = "Hello world!";
    const auto signature = olmAccount.sign(message);
    QVERIFY(QByteArray::fromBase64Encoding(signature).decodingStatus == QByteArray::Base64DecodingStatus::Ok);

    //let utility = OlmUtility::new();
    //let identity_keys = olm_account.parsed_identity_keys();
    //let ed25519_key = identity_keys.ed25519();
    //assert!(utility
    //    .ed25519_verify(&ed25519_key, message, &signature)
    //    .unwrap());
}

void TestOlmAccount::oneTimeKeysValid()
{
    QOlmAccount olmAccount(QStringLiteral("@foo:bar.com"), QStringLiteral("QuotientTestDevice"));
    olmAccount.createNewAccount();
    const auto maxNumberOfOneTimeKeys = olmAccount.maxNumberOfOneTimeKeys();
    QCOMPARE(100, maxNumberOfOneTimeKeys);

    const auto oneTimeKeysEmpty = olmAccount.oneTimeKeys();
    QVERIFY(oneTimeKeysEmpty.curve25519().isEmpty());

    olmAccount.generateOneTimeKeys(20);
    const auto oneTimeKeysFilled = olmAccount.oneTimeKeys();
    QCOMPARE(20, oneTimeKeysFilled.curve25519().count());
}

QTEST_MAIN(TestOlmAccount)
