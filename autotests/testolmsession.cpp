// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "e2ee/qolmsession.h"
#include "e2ee/qolmaccount.h"
#include "testolmsession.h"

using namespace Quotient;

std::pair<QOlmSession, QOlmSession> createSessionPair()
{
    QByteArray pickledAccountA("eOBXIKivUT6YYowRH031BNv7zNmzqM5B7CpXdyeaPvala5mt7/OeqrG1qVA7vA1SYloFyvJPIy0QNkD3j1HiPl5vtZHN53rtfZ9exXDok03zjmssqn4IJsqcA7Fbo1FZeKafG0NFcWwCPTdmcV7REqxjqGm3I4K8MQFa45AdTGSUu2C12cWeOcbSMlcINiMral+Uyah1sgPmLJ18h1qcnskXUXQvpffZ5DiUw1Iz5zxnwOQF1GVyowPJD7Zdugvj75RQnDxAn6CzyvrY2k2CuedwqDC3fIXM2xdUNWttW4nC2g4InpBhCVvNwhZYxlUb5BUEjmPI2AB3dAL5ry6o9MFncmbN6x5x");
    QByteArray pickledAccountB("eModTvoFi9oOIkax4j4nuxw9Tcl/J8mOmUctUWI68Q89HSaaPTqR+tdlKQ85v2GOs5NlZCp7EuycypN9GQ4fFbHUCrS7nspa3GFBWsR8PnM8+wez5PWmfFZLg3drOvT0jbMjpDx0MjGYClHBqcrEpKx9oFaIRGBaX6HXzT4lRaWSJkXxuX92q8iGNrLn96PuAWFNcD+2JXpPcNFntslwLUNgqzpZ04aIFYwL80GmzyOgq3Bz1GO6u3TgCQEAmTIYN2QkO0MQeuSfe7UoMumhlAJ6R8GPcdSSPtmXNk4tdyzzlgpVq1hm7ZLKto+g8/5Aq3PvnvA8wCqno2+Pi1duK1pZFTIlActr");
    auto accountA = QOlmAccount(u"accountA:foo.com", u"Device1UserA");
    if (accountA.unpickle(std::move(pickledAccountA), Unencrypted{})
        != OLM_SUCCESS)
        qFatal("Failed to unpickle account A: %s", accountA.lastError());

    auto accountB = QOlmAccount(u"accountB:foo.com", u"Device1UserB");
    if (accountB.unpickle(std::move(pickledAccountB), Unencrypted{})
        != OLM_SUCCESS)
        qFatal("Failed to unpickle account B: %s", accountB.lastError());


    const QByteArray identityKeyA("qIEr3TWcJQt4CP8QoKKJcCaukByIOpgh6erBkhLEa2o");
    const QByteArray oneTimeKeyA("WzsbsjD85iB1R32iWxfJdwkgmdz29ClMbJSJziECYwk");
    const QByteArray identityKeyB("q/YhJtog/5VHCAS9rM9uUf6AaFk1yPe4GYuyUOXyQCg");
    const QByteArray oneTimeKeyB("oWvzryma+B2onYjo3hM6A3Mgo/Yepm8HvgSvwZMTnjQ");
    auto outbound =
        accountA.createOutboundSession(identityKeyB, oneTimeKeyB).value();

    const auto preKey = outbound.encrypt(""); // Payload does not matter for PreKey

    if (preKey.type() != QOlmMessage::PreKey) {
        // We can't call QFail here because it's an helper function returning a value
        throw "Wrong first message type received, can't create session";
    }
    auto inbound = accountB.createInboundSession(preKey).value();
    return { std::move(inbound), std::move(outbound) };
}

void TestOlmSession::olmOutboundSessionCreation()
{
    const auto [_, outboundSession] = createSessionPair();
    QCOMPARE(0, outboundSession.hasReceivedMessage());
}

void TestOlmSession::olmEncryptDecrypt()
{
    const auto [inboundSession, outboundSession] = createSessionPair();
    const auto encrypted = outboundSession.encrypt("Hello world!");
    if (encrypted.type() == QOlmMessage::PreKey) {
        QOlmMessage m(encrypted); // clone
        QVERIFY(inboundSession.matchesInboundSession(m));
    }

    const auto decrypted = inboundSession.decrypt(encrypted).value();

    QCOMPARE(decrypted, "Hello world!");
}

void TestOlmSession::correctSessionOrdering()
{
    // n0W5IJ2ZmaI9FxKRj/wohUQ6WEU0SfoKsgKKHsr4VbM
    auto session1 = QOlmSession::unpickle("7g5cfQRsDk2ROXf9S01n2leZiFRon+EbvXcMOADU0UGvlaV6t/0ihD2/0QGckDIvbmE1aV+PxB0zUtHXh99bI/60N+PWkCLA84jEY4sz3d45ui/TVoFGLDHlymKxvlj7XngXrbtlxSkVntsPzDiNpKEXCa26N2ubKpQ0fbjrV5gbBTYWfU04DXHPXFDTksxpNALYt/h0eVMVhf6hB0ZzpLBsOG0mpwkLufwub0CuDEDGGmRddz3TcNCLq5NnI8R9udDWvHAkTS1UTbHuIf/y6cZg875nJyXpAvd8/XhL8TOo8ot2sE1fElBa4vrH/m9rBQMC1GPkhLBIizmY44C+Sq9PQRnF+uCZ", Unencrypted{}).value();
    // +9pHJhP3K4E5/2m8PYBPLh8pS9CJodwUOh8yz3mnmw0
    auto session2 = QOlmSession::unpickle("7g5cfQRsDk2ROXf9S01n2leZiFRon+EbvXcMOADU0UFD+q37/WlfTAzQsSjCdD07FcErZ4siEy5vpiB+pyO8i53ptZvb2qRvqNKFzPaXuu33PS2PBTmmnR+kJt+DgDNqWadyaj/WqEAejc7ALqSs5GuhbZtpoLe+lRSRK0rwVX3gzz4qrl8pm0pD5pSZAUWRXDRlieGWMclz68VUvnSaQH7ElTo4S634CJk+xQfFFCD26v0yONPSN6rwouS1cWPuG5jTlnV8vCFVTU2+lduKh54Ko6FUJ/ei4xR8Nk2duBGSc/TdllX9e2lDYHSUkWoD4ti5xsFioB8Blus7JK9BZfcmRmdlxIOD", Unencrypted {}).value();
    // MC7n8hX1l7WlC2/WJGHZinMocgiBZa4vwGAOredb/ME
    auto session3 = QOlmSession::unpickle("7g5cfQRsDk2ROXf9S01n2leZiFRon+EbvXcMOADU0UGNk2TmVDJ95K0Nywf24FNklNVtXtFDiFPHFwNSmCbHNCp3hsGtZlt0AHUkMmL48XklLqzwtVk5/v2RRmSKR5LqYdIakrtuK/fY0ENhBZIbI1sRetaJ2KMbY9l6rCJNfFg8VhpZ4KTVvEZVuP9g/eZkCnP5NxzXiBRF6nfY3O/zhcKxa3acIqs6BMhyLsfuJ80t+hQ1HvVyuhBerGujdSDzV9tJ9SPidOwfYATk81LVF9hTmnI0KaZa7qCtFzhG0dU/Z3hIWH9HOaw1aSB/IPmughbwdJOwERyhuo3YHoznlQnJ7X252BlI", Unencrypted{}).value();

    const auto session1Id = session1.sessionId();
    const auto session2Id = session2.sessionId();
    const auto session3Id = session3.sessionId();

    std::vector<QOlmSession> sessionList;
    sessionList.push_back(std::move(session1));
    sessionList.push_back(std::move(session2));
    sessionList.push_back(std::move(session3));

    std::sort(sessionList.begin(), sessionList.end());
    QCOMPARE(sessionList[0].sessionId(), session2Id);
    QCOMPARE(sessionList[1].sessionId(), session3Id);
    QCOMPARE(sessionList[2].sessionId(), session1Id);
}

QTEST_GUILESS_MAIN(TestOlmSession)
