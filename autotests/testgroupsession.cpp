// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "testgroupsession.h"
#include <Quotient/e2ee/qolminboundsession.h>
#include <Quotient/e2ee/qolmoutboundsession.h>

using namespace Quotient;

void TestGroupSession::groupSessionPicklingValid()
{
    auto ogs = QOlmOutboundGroupSession::create().value();
    const auto ogsId = ogs.sessionId();
    QVERIFY(QByteArray::fromBase64(ogsId).size() > 0);
    QCOMPARE(0, ogs.sessionMessageIndex());

    auto&& ogsPickled = ogs.pickle(PicklingKey::mock());
    auto ogs2 = QOlmOutboundGroupSession::unpickle(std::move(ogsPickled),
                                                   PicklingKey::mock())
                    .value();
    QCOMPARE(ogsId, ogs2.sessionId());

    auto igs = QOlmInboundGroupSession::create(ogs.sessionKey());
    QVERIFY(igs.has_value());
    const auto igsId = igs->sessionId();
    // ID is valid base64?
    QVERIFY(QByteArray::fromBase64(igsId).size() > 0);

    //// no messages have been sent yet
    QCOMPARE(0, igs->firstKnownIndex());

    auto igsPickled = igs->pickle(PicklingKey::mock());
    igs = QOlmInboundGroupSession::unpickle(std::move(igsPickled),
                                            PicklingKey::mock()).value();
    QVERIFY(igs.has_value());
    QCOMPARE(igsId, igs->sessionId());
}

void TestGroupSession::groupSessionCryptoValid()
{
    auto ogs = QOlmOutboundGroupSession::create().value();
    auto igs = QOlmInboundGroupSession::create(ogs.sessionKey());
    QVERIFY(igs.has_value());
    QCOMPARE(ogs.sessionId(), igs->sessionId());

    const auto plainText = "Hello world!";
    const auto ciphertext = ogs.encrypt(plainText);
    // ciphertext valid base64?
    QVERIFY(QByteArray::fromBase64(ciphertext).size() > 0);

    const auto decryptionResult = igs->decrypt(ciphertext).value();

    //// correct plaintext?
    QCOMPARE(plainText, decryptionResult.first);

    QCOMPARE(0, decryptionResult.second);
}
QTEST_GUILESS_MAIN(TestGroupSession)
