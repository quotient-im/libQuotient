// SPDX-FileCopyrightText: 2022 Tobias Fella <fella@posteo.de>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "olm/sas.h"

#include <QTest>
#include <QtCore/QDateTime>

#include <Quotient/connection.h>
#include <Quotient/e2ee/qolmaccount.h>
#include <Quotient/accountregistry.h>
#include <Quotient/pendingconnection.h>

using namespace Quotient;

class TestKeyVerificationSession : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testOutgoing1()
    {
        auto userId = QStringLiteral("@bob:localhost");
        auto deviceId = QStringLiteral("DEFABC");

        auto connection = (new AccountRegistry())->mockConnection("@carl:localhost"_ls)->connection();
        const auto transactionId = "other_transaction_id"_ls;
        auto session = connection->startKeyVerificationSession("@alice:localhost"_ls, "ABCDEF"_ls);
        session->sendRequest();
        QVERIFY(session->state() == KeyVerificationSession::WAITINGFORREADY);
        session->handleEvent(KeyVerificationReadyEvent(transactionId, "ABCDEF"_ls, {SasV1Method}));
        QVERIFY(session->state() == KeyVerificationSession::WAITINGFORACCEPT);
        session->handleEvent(KeyVerificationAcceptEvent(transactionId, "commitment_TODO"_ls));
        QVERIFY(session->state() == KeyVerificationSession::WAITINGFORKEY);
        // Since we can't get the events sent by the session, we're limited by what we can test. This means that continuing here would force us to test
        // the exact same path as the other test, which is useless.
        // TODO: Test some other path once we're able to.
    }

    void testIncoming1()
    {
        auto userId = QStringLiteral("@bob:localhost");
        auto deviceId = QStringLiteral("DEFABC");
        const auto transactionId = "trans123action123id"_ls;
        auto connection = (new AccountRegistry())->mockConnection("@carl:localhost"_ls)->connection();
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
        QByteArray keyBytes(olm_sas_pubkey_length(sas), '\0');
        olm_sas_get_pubkey(sas, keyBytes.data(), keyBytes.size());
        session->handleEvent(KeyVerificationKeyEvent(transactionId, QString::fromLatin1(keyBytes)));
        QVERIFY(session->state() == KeyVerificationSession::WAITINGFORVERIFICATION);
        session->sendMac();
        QVERIFY(session->state() == KeyVerificationSession::WAITINGFORMAC);
        //TODO: Send and verify the mac once we have a way of getting the KeyVerificationKeyEvent sent by the session.
    }
};
QTEST_GUILESS_MAIN(TestKeyVerificationSession)
#include "testkeyverification.moc"
