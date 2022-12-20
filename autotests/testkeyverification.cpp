// SPDX-FileCopyrightText: 2022 Tobias Fella <fella@posteo.de>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "connection.h"

#include <QTest>
#include <QtCore/QDateTime>

#include <e2ee/qolmaccount.h>
#include <olm/sas.h>

using namespace Quotient;

class TestKeyVerificationSession : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testOutgoing1()
    {
        auto connection = Connection::makeMockConnection("@carl:localhost"_ls);
        const auto transactionId = "other_transaction_id"_ls;
        auto session = connection->startKeyVerificationSession("@alice:localhost"_ls, "ABCDEF"_ls);
        QVERIFY(session->state() == KeyVerificationSession::WAITINGFORREADY);
        session->handleEvent(KeyVerificationReadyEvent(transactionId, "ABCDEF"_ls, {SasV1Method}));
        QVERIFY(session->state() == KeyVerificationSession::WAITINGFORACCEPT);
        session->handleEvent(KeyVerificationAcceptEvent(transactionId, "commitment_TODO"));
        QVERIFY(session->state() == KeyVerificationSession::WAITINGFORKEY);
        // Since we can't get the events sent by the session, we're limited by what we can test. This means that continuing here would force us to test
        // the exact same path as the other test, which is useless.
        // TODO: Test some other path once we're able to.
    }

    void testIncoming1()
    {
        const QString userId{ "@bob:localhost"_ls };
        const QString deviceId{ "DEFABC"_ls };
        const QString transactionId{ "trans123action123id"_ls };
        auto connection = Connection::makeMockConnection("@carl:localhost"_ls);
        auto session = new KeyVerificationSession(userId, KeyVerificationRequestEvent(transactionId, deviceId, {SasV1Method}, QDateTime::currentDateTime()), connection, false);
        QVERIFY(session->state() == KeyVerificationSession::INCOMING);
        session->setReady();
        QVERIFY(session->state() == KeyVerificationSession::WAITINGFORACCEPT);
        session->handleEvent(KeyVerificationStartEvent(transactionId, deviceId));
        QVERIFY(session->state() == KeyVerificationSession::ACCEPTED);
        auto account = new QOlmAccount(userId, deviceId);
        account->setupNewAccount();

        auto sas = makeCStruct(olm_sas, olm_sas_size, olm_clear_sas);
        const auto randomLength = olm_create_sas_random_length(sas.get());
        olm_create_sas(sas.get(), getRandom(randomLength).data(), randomLength);
        const auto keyBytesLength = olm_sas_pubkey_length(sas.get());
        auto keyBytes = byteArrayForOlm(keyBytesLength);
        olm_sas_get_pubkey(sas.get(), keyBytes.data(), keyBytesLength);
        session->handleEvent(KeyVerificationKeyEvent(transactionId, keyBytes));
        QVERIFY(session->state() == KeyVerificationSession::WAITINGFORVERIFICATION);
        session->sasVerified();
        QVERIFY(session->state() == KeyVerificationSession::WAITINGFORMAC);
        //TODO: Send and verify the mac once we have a way of getting the KeyVerificationKeyEvent sent by the session.
    }
};
QTEST_GUILESS_MAIN(TestKeyVerificationSession)
#include "testkeyverification.moc"
