// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "testgroupsession.h"
#include "crypto/qolminboundsession.h"
#include "crypto/qolmoutboundsession.h"
#include "crypto/qolmutils.h"

using namespace Quotient;

void TestOlmSession::groupSessionPicklingValid()
{
    auto ogs = QOlmOutboundGroupSession::create();
    const auto ogsId = ogs->sessionId();
    QVERIFY(QByteArray::fromBase64Encoding(ogsId).decodingStatus == QByteArray::Base64DecodingStatus::Ok);
    QCOMPARE(0, ogs->sessionMessageIndex());

    auto ogsPickled = std::get<QByteArray>(ogs->pickle(Unencrypted {}));
    auto ogs2 = std::get<QOlmOutboundGroupSessionPtr>(QOlmOutboundGroupSession::unpickle(ogsPickled, Unencrypted {}));
    QCOMPARE(ogsId, ogs2->sessionId());

    auto igs = QOlmInboundGroupSession::create(std::get<QByteArray>(ogs->sessionKey()));
    const auto igsId = igs->sessionId();
    // ID is valid base64?
    QVERIFY(QByteArray::fromBase64Encoding(igsId).decodingStatus == QByteArray::Base64DecodingStatus::Ok);

    //// no messages have been sent yet
    QCOMPARE(0, igs->firstKnownIndex());

    auto igsPickled = igs->pickle(Unencrypted {});
    igs = std::get<QOlmInboundGroupSessionPtr>(QOlmInboundGroupSession::unpickle(igsPickled, Unencrypted {}));
    QCOMPARE(igsId, igs->sessionId());
}

void TestOlmSession::groupSessionCryptoValid()
{
    auto ogs = QOlmOutboundGroupSession::create();
    auto igs = QOlmInboundGroupSession::create(std::get<QByteArray>(ogs->sessionKey()));
    QCOMPARE(ogs->sessionId(), igs->sessionId());

    const auto plainText = QStringLiteral("Hello world!");
    const auto ciphertext = std::get<QByteArray>(ogs->encrypt(plainText));
    qDebug() << ciphertext;
    // ciphertext valid base64?
    QVERIFY(QByteArray::fromBase64Encoding(ciphertext).decodingStatus == QByteArray::Base64DecodingStatus::Ok);

    const auto decryptionResult = std::get<std::pair<QString, uint32_t>>(igs->decrypt(ciphertext));

    //// correct plaintext?
    QCOMPARE(plainText, decryptionResult.first);

    QCOMPARE(0, decryptionResult.second);
}
QTEST_MAIN(TestOlmSession)
