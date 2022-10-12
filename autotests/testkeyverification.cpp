// SPDX-FileCopyrightText: 2022 Tobias Fella <fella@posteo.de>
//
// SPDX-License-Identifier: LGPL-2.1-or-later


#include <QTest>
#include "testutils.h"
#include <qt_connection_util.h>
#include <QtCore/QTimer>
#include <room.h>

class TestKeyVerificationSession : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testVerification()
    {
        CREATE_CONNECTION(a, "alice1", "secret", "AliceDesktop")
        CREATE_CONNECTION(b, "alice2", "secret", "AlicePhone")

        a->requestDirectChat(b->userId());
        connectSingleShot(b.get(), &Connection::invitedRoom, this, [=](Quotient::Room* room) {
            b->joinRoom(room->id());
            a->room(room->id())->activateEncryption();
        });

        QPointer<KeyVerificationSession> aSession{};
        connect(a.get(), &Connection::newKeyVerificationSession, this, [&](KeyVerificationSession* session) {
            aSession = session;
            QVERIFY(aSession);
            connect(session, &KeyVerificationSession::stateChanged, this, [session] {
                QVERIFY(session->state() != KeyVerificationSession::CANCELED);
            });
            QVERIFY(session->remoteDeviceId() == b->deviceId());
            QVERIFY(session->state() == KeyVerificationSession::WAITINGFORREADY);
            connectSingleShot(session, &KeyVerificationSession::stateChanged, this, [=](){
                QVERIFY(session->state() == KeyVerificationSession::ACCEPTED || session->state() == KeyVerificationSession::READY);
                connectSingleShot(session, &KeyVerificationSession::stateChanged, this, [=](){
                    QVERIFY(session->state() == KeyVerificationSession::WAITINGFORVERIFICATION);
                });
            });
        });
        QTimer::singleShot(3000, this, [a, b] {
            a->startKeyVerificationSession(b->userId(), b->deviceId());
        });
        connect(b.get(), &Connection::newKeyVerificationSession, this, [&](KeyVerificationSession* session) {
            connect(session, &KeyVerificationSession::stateChanged, this, [session] {
                QVERIFY(session->state() != KeyVerificationSession::CANCELED);
            });
            QVERIFY(session->remoteDeviceId() == a->deviceId());
            QVERIFY(session->state() == KeyVerificationSession::INCOMING);
            session->sendReady();
            // KeyVerificationSession::READY is skipped because we have only one method
            QVERIFY(session->state() == KeyVerificationSession::WAITINGFORACCEPT);
            connectSingleShot(session, &KeyVerificationSession::stateChanged, this, [=](){
                QVERIFY(session->state() == KeyVerificationSession::WAITINGFORKEY || session->state() == KeyVerificationSession::ACCEPTED);
                connectSingleShot(session, &KeyVerificationSession::stateChanged, this, [=]() {
                    QVERIFY(session->state() == KeyVerificationSession::WAITINGFORVERIFICATION);
                    QVERIFY(aSession);
                    QVERIFY(aSession->sasEmojis() == session->sasEmojis());
                    session->sendMac();
                    aSession->sendMac();
                    QVERIFY(session->state() == KeyVerificationSession::WAITINGFORMAC);
                });
            });

        });
        b->syncLoop();
        a->syncLoop();
        QSignalSpy spy1(b.get(), &Connection::newKeyVerificationSession);
        spy1.wait(10000);
        QSignalSpy spy2(aSession, &KeyVerificationSession::finished);
        spy2.wait(10000);
    }
};
QTEST_GUILESS_MAIN(TestKeyVerificationSession)
#include "testkeyverification.moc"
