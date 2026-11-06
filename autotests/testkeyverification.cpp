// SPDX-FileCopyrightText: 2022 Tobias Fella <fella@posteo.de>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "olm/sas.h"

#include <QTest>
#include <QtCore/QDateTime>

#include <memory>

#include <Quotient/connection.h>
#include <Quotient/e2ee/qolmaccount.h>
#include <Quotient/e2ee/cryptoutils.h>

using namespace Quotient;

class TestKeyVerificationSession : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testSimultaneousStart_data()
    {
        QTest::addColumn<QString>("localUser");
        QTest::addColumn<QString>("remoteUser");
        QTest::addColumn<QString>("localDevice");
        QTest::addColumn<QString>("remoteDevice");
        QTest::addColumn<bool>("keepOurStart");
        QTest::newRow("self-local-smaller")
            << u"@self:localhost"_s << u"@self:localhost"_s
            << u"AAA"_s << u"ZZZ"_s << true;
        QTest::newRow("self-local-larger")
            << u"@self:localhost"_s << u"@self:localhost"_s
            << u"ZZZ"_s << u"AAA"_s << false;
        QTest::newRow("different-local-smaller")
            << u"@alice:localhost"_s << u"@bob:localhost"_s
            << u"ZZZ"_s << u"AAA"_s << true;
        QTest::newRow("different-local-larger")
            << u"@bob:localhost"_s << u"@alice:localhost"_s
            << u"AAA"_s << u"ZZZ"_s << false;
    }

    void testSimultaneousStart()
    {
        QFETCH(QString, localUser);
        QFETCH(QString, remoteUser);
        QFETCH(QString, localDevice);
        QFETCH(QString, remoteDevice);
        QFETCH(bool, keepOurStart);
        auto connection = std::unique_ptr<Connection>(
            Connection::makeMockConnection(localUser, true, localDevice));
        QCOMPARE(connection->userId(), localUser);
        QCOMPARE(connection->deviceId(), localDevice);
        auto session = connection->startKeyVerificationSession(remoteUser, remoteDevice);
        session->sendRequest();
        const auto transactionId = session->transactionId();
        session->handleEvent(
            KeyVerificationReadyEvent(transactionId, remoteDevice, {SasV1Method}));
        QCOMPARE(session->state(), KeyVerificationSession::WAITINGFORACCEPT);
        session->handleEvent(KeyVerificationStartEvent(transactionId, remoteDevice));
        QCOMPARE(session->state(), keepOurStart ? KeyVerificationSession::WAITINGFORACCEPT
                                               : KeyVerificationSession::ACCEPTED);
        if (keepOurStart) {
            session->handleEvent(KeyVerificationAcceptEvent(
            transactionId, QString::fromLatin1("commitment_TODO"_ba.toBase64())));
            QCOMPARE(session->state(), KeyVerificationSession::WAITINGFORKEY);
        }
    }

    void testSelfVerificationCrossedStartThenAccept()
    {
        const auto user = u"@self-sequence:localhost"_s;
        auto connection = std::unique_ptr<Connection>(
            Connection::makeMockConnection(user, true, u"AAA"_s));
        QCOMPARE(connection->deviceId(), u"AAA"_s);
        auto session = connection->startKeyVerificationSession(user, u"ZZZ"_s);
        session->sendRequest();
        const auto transactionId = session->transactionId();
        session->handleEvent(KeyVerificationReadyEvent(transactionId, u"ZZZ"_s, {SasV1Method}));
        session->handleEvent(KeyVerificationStartEvent(transactionId, u"ZZZ"_s));
        session->handleEvent(KeyVerificationAcceptEvent(
            transactionId, QString::fromLatin1("commitment_TODO"_ba.toBase64())));
        QCOMPARE(session->state(), KeyVerificationSession::WAITINGFORKEY);
        QCOMPARE(session->error(), KeyVerificationSession::NONE);
    }

    void testOutgoing1()
    {
        auto connection = Connection::makeMockConnection("@carl:localhost"_L1);
        const auto transactionId = "other_transaction_id"_L1;
        auto session = connection->startKeyVerificationSession("@alice:localhost"_L1, "ABCDEF"_L1);
        session->sendRequest();
        QVERIFY(session->state() == KeyVerificationSession::WAITINGFORREADY);
        session->handleEvent(KeyVerificationReadyEvent(transactionId, "ABCDEF"_L1, {SasV1Method}));
        QVERIFY(session->state() == KeyVerificationSession::WAITINGFORACCEPT);
        session->handleEvent(KeyVerificationAcceptEvent(
            transactionId, QString::fromLatin1("commitment_TODO"_ba.toBase64())));
        QVERIFY(session->state() == KeyVerificationSession::WAITINGFORKEY);
        // Since we can't get the events sent by the session, we're limited by what we can test. This means that continuing here would force us to test
        // the exact same path as the other test, which is useless.
        // TODO: Test some other path once we're able to.
    }

    void testIncoming1()
    {
        auto userId = u"@bob:localhost"_s;
        auto deviceId = u"DEFABC"_s;
        const auto transactionId = "trans123action123id"_L1;
        auto connection = Connection::makeMockConnection("@carl:localhost"_L1);
        auto session = new KeyVerificationSession(userId, KeyVerificationRequestEvent(transactionId, deviceId, {SasV1Method}, QDateTime::currentDateTime()), connection, false);
        QVERIFY(session->state() == KeyVerificationSession::INCOMING);
        session->sendReady();
        QVERIFY(session->state() == KeyVerificationSession::WAITINGFORACCEPT);
        session->handleEvent(KeyVerificationStartEvent(transactionId, deviceId));
        QVERIFY(session->state() == KeyVerificationSession::ACCEPTED);
        auto account = new QOlmAccount(userId, deviceId);
        account->setupNewAccount();

        auto sas = olm_sas(new std::byte[olm_sas_size()]);
        const auto randomLength = olm_create_sas_random_length(sas);
        olm_create_sas(sas, getRandom(randomLength).data(), randomLength);
        auto keyBytes = byteArrayForOlm(olm_sas_pubkey_length(sas));
        olm_sas_get_pubkey(sas, keyBytes.data(), unsignedSize(keyBytes));
        session->handleEvent(KeyVerificationKeyEvent(transactionId, QString::fromLatin1(keyBytes)));
        QVERIFY(session->state() == KeyVerificationSession::WAITINGFORVERIFICATION);
        session->sendMac();
        QVERIFY(session->state() == KeyVerificationSession::WAITINGFORMAC);
        //TODO: Send and verify the mac once we have a way of getting the KeyVerificationKeyEvent sent by the session.
    }
};
QTEST_GUILESS_MAIN(TestKeyVerificationSession)
#include "testkeyverification.moc"
