// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "testgroupsession.h"
#include "e2ee/qolminboundsession.h"
#include "e2ee/qolmoutboundsession.h"
#include "e2ee/qolmutils.h"

using namespace Quotient;

void TestGroupSession::groupSessionPicklingValid()
{
    auto ogs = QOlmOutboundGroupSession::create();
    const auto ogsId = ogs->sessionId();
    QVERIFY(QByteArray::fromBase64(ogsId).size() > 0);
    QCOMPARE(0, ogs->sessionMessageIndex());

    auto ogsPickled = std::get<QByteArray>(ogs->pickle(Unencrypted {}));
    auto ogs2 = std::get<QOlmOutboundGroupSessionPtr>(QOlmOutboundGroupSession::unpickle(ogsPickled, Unencrypted {}));
    QCOMPARE(ogsId, ogs2->sessionId());

    auto igs = QOlmInboundGroupSession::create(std::get<QByteArray>(ogs->sessionKey()));
    const auto igsId = igs->sessionId();
    // ID is valid base64?
    QVERIFY(QByteArray::fromBase64(igsId).size() > 0);

    //// no messages have been sent yet
    QCOMPARE(0, igs->firstKnownIndex());

    auto igsPickled = igs->pickle(Unencrypted {});
    igs = std::get<QOlmInboundGroupSessionPtr>(QOlmInboundGroupSession::unpickle(igsPickled, Unencrypted {}));
    QCOMPARE(igsId, igs->sessionId());
}

void TestGroupSession::groupSessionCryptoValid()
{
    auto ogs = QOlmOutboundGroupSession::create();
    auto igs = QOlmInboundGroupSession::create(std::get<QByteArray>(ogs->sessionKey()));
    QCOMPARE(ogs->sessionId(), igs->sessionId());

    const auto plainText = QStringLiteral("Hello world!");
    const auto ciphertext = std::get<QByteArray>(ogs->encrypt(plainText));
    // ciphertext valid base64?
    QVERIFY(QByteArray::fromBase64(ciphertext).size() > 0);

    const auto decryptionResult = std::get<std::pair<QString, uint32_t>>(igs->decrypt(ciphertext));

    //// correct plaintext?
    QCOMPARE(plainText, decryptionResult.first);

    QCOMPARE(0, decryptionResult.second);
}
QTEST_GUILESS_MAIN(TestGroupSession)
