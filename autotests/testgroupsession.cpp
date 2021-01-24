// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifdef Quotient_E2EE_ENABLED
#include "testgroupsession.h"
#include "olm/qolminboundsession.h"
#include "olm/qolmoutboundsession.h"
#include "olm/utils.h"

using namespace Quotient;

void TestOlmSession::groupSessionPicklingValid()
{
    auto ogs = std::get<QOlmOutboundGroupSession>(QOlmOutboundGroupSession::create());
    const auto ogsId = std::get<QByteArray>(ogs.sessionId());
    QVERIFY(QByteArray::fromBase64Encoding(ogsId).decodingStatus == QByteArray::Base64DecodingStatus::Ok);
    QCOMPARE(0, ogs.sessionMessageIndex());

    auto ogsPickled = std::get<QByteArray>(ogs.pickle(Unencrypted {}));
    ogs = std::get<QOlmOutboundGroupSession>(QOlmOutboundGroupSession::unpickle(ogsPickled, Unencrypted {}));
    QCOMPARE(ogsId, std::get<QByteArray>(ogs.sessionId()));

    qDebug() << std::get<QByteArray>(ogs.sessionKey());
    auto igs = std::get<QOlmInboundGroupSession>(QOlmInboundGroupSession::create(std::get<QByteArray>(ogs.sessionKey())));
    const auto igsId = std::get<QByteArray>(igs.sessionId());
    // ID is valid base64?
    QVERIFY(QByteArray::fromBase64Encoding(igsId).decodingStatus == QByteArray::Base64DecodingStatus::Ok);

    //// no messages have been sent yet
    QCOMPARE(0, igs.firstKnownIndex());

    auto igsPickled = std::get<QByteArray>(igs.pickle(Unencrypted {}));
    igs = std::get<QOlmInboundGroupSession>(QOlmInboundGroupSession::unpickle(igsPickled, Unencrypted {}));
    QCOMPARE(igsId, std::get<QByteArray>(igs.sessionId()));
}

QTEST_MAIN(TestOlmSession)
#endif
