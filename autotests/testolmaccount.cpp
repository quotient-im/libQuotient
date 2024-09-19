// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
// SPDX-FileCopyrightText: 2020 mtxclient developers
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "testolmaccount.h"

#include <Quotient/connection.h>
#include <Quotient/csapi/joining.h>
#include <Quotient/e2ee/qolmaccount.h>
#include <Quotient/e2ee/qolmutility.h>
#include <Quotient/events/encryptionevent.h>
#include <Quotient/events/filesourceinfo.h>
#include <Quotient/networkaccessmanager.h>
#include <Quotient/room.h>

#include "testutils.h"

using namespace Quotient;

void TestOlmAccount::pickleUnpickledTest()
{
    QOlmAccount olmAccount(u"@foo:bar.com"_s, u"QuotientTestDevice"_s);
    olmAccount.setupNewAccount();
    auto identityKeys = olmAccount.identityKeys();
    auto pickled = olmAccount.pickle(PicklingKey::mock());
    QOlmAccount olmAccount2(u"@foo:bar.com"_s, u"QuotientTestDevice"_s);
    auto unpickleResult = olmAccount2.unpickle(std::move(pickled),
                                               PicklingKey::mock());
    QCOMPARE(unpickleResult, 0);
    auto identityKeys2 = olmAccount2.identityKeys();
    QCOMPARE(identityKeys.curve25519, identityKeys2.curve25519);
    QCOMPARE(identityKeys.ed25519, identityKeys2.ed25519);
}

void TestOlmAccount::identityKeysValid()
{
    QOlmAccount olmAccount(u"@foo:bar.com"_s, u"QuotientTestDevice"_s);
    olmAccount.setupNewAccount();
    const auto identityKeys = olmAccount.identityKeys();
    const auto curve25519 = identityKeys.curve25519;
    const auto ed25519 = identityKeys.ed25519;
    // verify encoded keys length
    QCOMPARE(curve25519.size(), 43);
    QCOMPARE(ed25519.size(), 43);

    // encoded as valid base64?
    QVERIFY(QByteArray::fromBase64(curve25519.toLatin1()).size() > 0);
    QVERIFY(QByteArray::fromBase64(ed25519.toLatin1()).size() > 0);
}

void TestOlmAccount::signatureValid()
{
    QOlmAccount olmAccount(u"@foo:bar.com"_s, u"QuotientTestDevice"_s);
    olmAccount.setupNewAccount();
    const auto message = "Hello world!";
    const auto signature = olmAccount.sign(message);
    QVERIFY(QByteArray::fromBase64(signature).size() > 0);

    QOlmUtility utility;
    const auto identityKeys = olmAccount.identityKeys();
    const auto ed25519Key = identityKeys.ed25519;
    QVERIFY(utility.ed25519Verify(ed25519Key.toLatin1(), message, signature));
}

void TestOlmAccount::oneTimeKeysValid()
{
    QOlmAccount olmAccount(u"@foo:bar.com"_s, u"QuotientTestDevice"_s);
    olmAccount.setupNewAccount();
    const auto maxNumberOfOneTimeKeys = olmAccount.maxNumberOfOneTimeKeys();
    QCOMPARE(100, maxNumberOfOneTimeKeys);

    const auto oneTimeKeysEmpty = olmAccount.oneTimeKeys();
    QVERIFY(oneTimeKeysEmpty.curve25519().isEmpty());

    olmAccount.generateOneTimeKeys(20);
    const auto oneTimeKeysFilled = olmAccount.oneTimeKeys();
    QCOMPARE(20, oneTimeKeysFilled.curve25519().size());
}

void TestOlmAccount::deviceKeys()
{
    // copied from mtxclient
    DeviceKeys device1;
    device1.userId = "@alice:example.com"_L1;
    device1.deviceId = "JLAFKJWSCS"_L1;
    device1.keys = {{"curve25519:JLAFKJWSCS"_L1, "3C5BFWi2Y8MaVvjM8M22DBmh24PmgR0nPvJOIArzgyI"_L1},
                    {"ed25519:JLAFKJWSCS"_L1, "lEuiRJBit0IG6nUf5pUzWTUEsRVVe/HJkoKuEww9ULI"_L1}};

    // TODO that should be the default value
    device1.algorithms =
        QStringList { OlmV1Curve25519AesSha2AlgoKey, MegolmV1AesSha2AlgoKey };

    device1.signatures = {
      {"@alice:example.com"_L1,
       {{"ed25519:JLAFKJWSCS"_L1,
         "dSO80A01XiigH3uBiDVx/EjzaoycHcjq9lfQX0uWsqxl2giMIiSPR8a4d291W1ihKJL/"
         "a+myXS367WT6NAIcBA"_L1}}}};

    QJsonObject j;
    JsonObjectConverter<DeviceKeys>::dumpTo(j, device1);
    QJsonDocument doc(j);
    QCOMPARE(doc.toJson(QJsonDocument::Compact), "{\"algorithms\":[\"m.olm.v1.curve25519-aes-sha2\",\"m.megolm.v1.aes-sha2\"],"
              "\"device_id\":\"JLAFKJWSCS\",\"keys\":{\"curve25519:JLAFKJWSCS\":"
              "\"3C5BFWi2Y8MaVvjM8M22DBmh24PmgR0nPvJOIArzgyI\",\"ed25519:JLAFKJWSCS\":"
              "\"lEuiRJBit0IG6nUf5pUzWTUEsRVVe/"
              "HJkoKuEww9ULI\"},\"signatures\":{\"@alice:example.com\":{\"ed25519:JLAFKJWSCS\":"
              "\"dSO80A01XiigH3uBiDVx/EjzaoycHcjq9lfQX0uWsqxl2giMIiSPR8a4d291W1ihKJL/"
              "a+myXS367WT6NAIcBA\"}},\"user_id\":\"@alice:example.com\"}");

    auto doc2 = QJsonDocument::fromJson(R"({
      "user_id": "@alice:example.com",
      "device_id": "JLAFKJWSCS",
      "algorithms": [
        "m.olm.v1.curve25519-aes-sha2",
        "m.megolm.v1.aes-sha2"
      ],
      "keys": {
        "curve25519:JLAFKJWSCS": "3C5BFWi2Y8MaVvjM8M22DBmh24PmgR0nPvJOIArzgyI",
        "ed25519:JLAFKJWSCS": "lEuiRJBit0IG6nUf5pUzWTUEsRVVe/HJkoKuEww9ULI"
      },
      "signatures": {
        "@alice:example.com": {
          "ed25519:JLAFKJWSCS": "dSO80A01XiigH3uBiDVx/EjzaoycHcjq9lfQX0uWsqxl2giMIiSPR8a4d291W1ihKJL/a+myXS367WT6NAIcBA"
        }
      },
      "unsigned": {
        "device_display_name": "Alice's mobile phone"
      }
    })");

    DeviceKeys device2;
    JsonObjectConverter<DeviceKeys>::fillFrom(doc2.object(), device2);

    QCOMPARE(device2.userId, device1.userId);
    QCOMPARE(device2.deviceId, device1.deviceId);
    QCOMPARE(device2.keys, device1.keys);
    QCOMPARE(device2.algorithms, device1.algorithms);
    QCOMPARE(device2.signatures, device1.signatures);

    // UnsignedDeviceInfo is missing from the generated DeviceKeys object :(
    // QCOMPARE(device2.unsignedInfo.deviceDisplayName, "Alice's mobile phone");
}

void TestOlmAccount::encryptedFile()
{
    auto doc = QJsonDocument::fromJson(R"({
      "url": "mxc://example.org/FHyPlCeYUSFFxlgbQYZmoEoe",
      "v": "v2",
      "key": {
        "alg": "A256CTR",
        "ext": true,
        "k": "aWF6-32KGYaC3A_FEUCk1Bt0JA37zP0wrStgmdCaW-0",
        "key_ops": ["encrypt","decrypt"],
        "kty": "oct"
      },
      "iv": "w+sE15fzSc0AAAAAAAAAAA",
      "hashes": {
        "sha256": "fdSLu/YkRx3Wyh3KQabP3rd6+SFiKg5lsJZQHtkSAYA"
      }})");

    const auto file = fromJson<EncryptedFileMetadata>(doc);

    QCOMPARE(file.v, "v2"_L1);
    QCOMPARE(file.iv, "w+sE15fzSc0AAAAAAAAAAA"_L1);
    QCOMPARE(file.hashes["sha256"_L1], "fdSLu/YkRx3Wyh3KQabP3rd6+SFiKg5lsJZQHtkSAYA"_L1);
    QCOMPARE(file.key.alg, "A256CTR"_L1);
    QCOMPARE(file.key.ext, true);
    QCOMPARE(file.key.k, "aWF6-32KGYaC3A_FEUCk1Bt0JA37zP0wrStgmdCaW-0"_L1);
    QCOMPARE(file.key.keyOps.size(), 2);
    QCOMPARE(file.key.kty, "oct"_L1);
}

void TestOlmAccount::uploadIdentityKey()
{
    CREATE_CONNECTION(conn, "alice1"_L1, "secret"_L1, "AlicePhone"_L1)

    auto olmAccount = conn->olmAccount();
    auto idKeys = olmAccount->identityKeys();

    QVERIFY(idKeys.curve25519.size() > 10);

    UnsignedOneTimeKeys unused;
    auto request = olmAccount->createUploadKeyRequest(unused);
    connect(request, &BaseJob::result, this, [request] {
        if (!request->status().good())
            QFAIL("upload failed");
        const auto& oneTimeKeyCounts = request->oneTimeKeyCounts();
        // Allow the response to have entries with zero counts
        QCOMPARE(std::accumulate(oneTimeKeyCounts.begin(),
                                 oneTimeKeyCounts.end(), 0),
                 0);
    });
    conn->run(request);
    QSignalSpy spy3(request, &BaseJob::result);
    QVERIFY(spy3.wait(10000));
}

void TestOlmAccount::uploadOneTimeKeys()
{
    CREATE_CONNECTION(conn, "alice2"_L1, "secret"_L1, "AlicePhone"_L1)
    auto olmAccount = conn->olmAccount();

    auto nKeys = olmAccount->generateOneTimeKeys(5);
    QCOMPARE(nKeys, 5);

    const auto oneTimeKeys = olmAccount->oneTimeKeys();

    OneTimeKeys oneTimeKeysHash;
    for (const auto& [keyId, key] : oneTimeKeys.curve25519().asKeyValueRange())
        oneTimeKeysHash["curve25519:"_L1 + keyId] = key;

    const auto uploadFuture = conn->callApi<UploadKeysJob>(std::nullopt, oneTimeKeysHash)
                                  .then(
                                      [](const QHash<QString, int>& oneTimeKeyCounts) {
                                          QCOMPARE(oneTimeKeyCounts.value(Curve25519Key), 5);
                                      },
                                      [] { QFAIL("upload failed"); });
    QVERIFY(waitForFuture(uploadFuture));
}

void TestOlmAccount::uploadSignedOneTimeKeys()
{
    CREATE_CONNECTION(conn, "alice3"_L1, "secret"_L1, "AlicePhone"_L1)
    auto olmAccount = conn->olmAccount();
    auto nKeys = olmAccount->generateOneTimeKeys(5);
    QCOMPARE(nKeys, 5);

    auto oneTimeKeys = olmAccount->oneTimeKeys();
    auto oneTimeKeysHash = olmAccount->signOneTimeKeys(oneTimeKeys);
    const auto uploadFuture =
        conn->callApi<UploadKeysJob>(std::nullopt, oneTimeKeysHash)
            .then(
                [nKeys](const QHash<QString, int>& oneTimeKeyCounts) {
                    QCOMPARE(oneTimeKeyCounts.value(SignedCurve25519Key), nKeys);
                },
                [] { QFAIL("upload failed"); });
    QVERIFY(waitForFuture(uploadFuture));
}

void TestOlmAccount::uploadKeys()
{
    CREATE_CONNECTION(conn, "alice4"_L1, "secret"_L1, "AlicePhone"_L1)
    auto olmAccount = conn->olmAccount();
    auto idks = olmAccount->identityKeys();
    olmAccount->generateOneTimeKeys(1);
    auto otks = olmAccount->oneTimeKeys();
    auto request = conn->run(olmAccount->createUploadKeyRequest(otks))
                       .then(
                           [](const QHash<QString, int>& oneTimeKeyCounts) {
                               QCOMPARE(oneTimeKeyCounts.value(SignedCurve25519Key), 1);
                           },
                           [] { QFAIL("upload failed"); });
    QVERIFY(waitForFuture(request));
}

void TestOlmAccount::queryTest()
{
    CREATE_CONNECTION(alice, "alice5"_L1, "secret"_L1, "AlicePhone"_L1)
    CREATE_CONNECTION(bob, "bob1"_L1, "secret"_L1, "BobPhone"_L1)

    // Create and upload keys for both users.
    auto aliceOlm = alice->olmAccount();
    aliceOlm->generateOneTimeKeys(1);
    const auto aliceUploadKeysRequest =
        alice->run(aliceOlm->createUploadKeyRequest(aliceOlm->oneTimeKeys()))
            .then([](const QHash<QString, int>& aliceOneTimeKeyCounts) {
                QCOMPARE(aliceOneTimeKeyCounts.value(SignedCurve25519Key), 1);
            });
    QVERIFY(waitForFuture(aliceUploadKeysRequest));

    auto bobOlm = bob->olmAccount();
    bobOlm->generateOneTimeKeys(1);
    const auto bobUploadKeysRequest =
        bob->run(bobOlm->createUploadKeyRequest(aliceOlm->oneTimeKeys()))
            .then([](const QHash<QString, int>& bobOneTimeKeyCounts) {
                QCOMPARE(bobOneTimeKeyCounts.value(SignedCurve25519Key), 1);
            });
    QVERIFY(waitForFuture(bobUploadKeysRequest));

    // Each user is requests each other's keys.
    const QHash<QString, QStringList> deviceKeysForBob{ { bob->userId(), {} } };
    const auto queryBobKeysResult =
        alice->callApi<QueryKeysJob>(deviceKeysForBob)
            .then([bob, bobOlm](const QueryKeysJob::Response& r) {
                QCOMPARE(r.failures.size(), 0);

                const auto& aliceDevices = r.deviceKeys.value(bob->userId());
                QVERIFY(!aliceDevices.empty());

                const auto& aliceDevKeys = aliceDevices.value(bob->deviceId());
                QCOMPARE(aliceDevKeys.userId, bob->userId());
                QCOMPARE(aliceDevKeys.deviceId, bob->deviceId());
                QCOMPARE(aliceDevKeys.keys, bobOlm->deviceKeys().keys);
                QCOMPARE(aliceDevKeys.signatures, bobOlm->deviceKeys().signatures);
            });
    QVERIFY(waitForFuture(queryBobKeysResult));

    const QHash<QString, QStringList> deviceKeysForAlice{ { alice->userId(), {} } };
    const auto queryAliceKeysResult =
        bob->callApi<QueryKeysJob>(deviceKeysForAlice)
            .then([alice, aliceOlm](const QueryKeysJob::Response& r) {
                QCOMPARE(r.failures.size(), 0);

                const auto& bobDevices = r.deviceKeys.value(alice->userId());
                QVERIFY(!bobDevices.empty());

                auto devKeys = bobDevices[alice->deviceId()];
                QCOMPARE(devKeys.userId, alice->userId());
                QCOMPARE(devKeys.deviceId, alice->deviceId());
                QCOMPARE(devKeys.keys, aliceOlm->deviceKeys().keys);
                QCOMPARE(devKeys.signatures, aliceOlm->deviceKeys().signatures);
            });
    QVERIFY(waitForFuture(queryAliceKeysResult));
}

void TestOlmAccount::claimKeys()
{
    CREATE_CONNECTION(alice, "alice6"_L1, "secret"_L1, "AlicePhone"_L1)
    CREATE_CONNECTION(bob, "bob2"_L1, "secret"_L1, "BobPhone"_L1)

    // Bob uploads his keys.
    auto *bobOlm = bob->olmAccount();
    bobOlm->generateOneTimeKeys(1);
    auto request = bob->run(bobOlm->createUploadKeyRequest(bobOlm->oneTimeKeys()))
                       .then([bob](const QHash<QString, int>& oneTimeKeyCounts) {
                           QCOMPARE(oneTimeKeyCounts.value(SignedCurve25519Key), 1);
                       });
    QVERIFY(waitForFuture(request));

    // Alice retrieves bob's keys & claims one signed one-time key.
    const QHash<QString, QStringList> deviceKeysToQuery{ { bob->userId(), {} } };
    const auto queryKeysJob =
        alice->callApi<QueryKeysJob>(deviceKeysToQuery).then([bob](const QueryKeysJob::Response& r) {
            const auto& bobDevices = r.deviceKeys.value(bob->userId());
            QVERIFY(!bobDevices.empty());

            QVERIFY(verifyIdentitySignature(bobDevices.value(bob->deviceId()), bob->deviceId(),
                                            bob->userId()));
        });
    QVERIFY(waitForFuture(queryKeysJob));

    // Retrieve the identity key for the current device to check after claiming
    // const auto& bobEd25519 =
    //     bobDevices.value(bob->deviceId()).keys.value("ed25519:"_L1 + bob->deviceId());

    const QHash<QString, QHash<QString, QString>> oneTimeKeys{
        { bob->userId(), { { bob->deviceId(), SignedCurve25519Key } } }
    };
    const auto claimKeysJob =
        alice->callApi<ClaimKeysJob>(oneTimeKeys)
            .then(
                [bob](const ClaimKeysJob::Response& r) {
                    // The device exists.
                    QCOMPARE(r.oneTimeKeys.size(), 1);
                    const auto& allClaimedKeys = r.oneTimeKeys.value(bob->userId());
                    QCOMPARE(allClaimedKeys.size(), 1);

                    // The key is the one bob sent.
                    const auto& claimedDeviceKeys = allClaimedKeys.value(bob->deviceId());
                    for (const auto& claimedKey : claimedDeviceKeys.asKeyValueRange()) {
                        if (!claimedKey.first.startsWith(SignedCurve25519Key))
                            continue;
                        QVERIFY(std::holds_alternative<SignedOneTimeKey>(claimedKey.second));
                    }
                },
                [] { QFAIL("Claim job failed"); });
    QVERIFY(waitForFuture(claimKeysJob));
}

void TestOlmAccount::claimMultipleKeys()
{
    // Login with alice multiple times
    CREATE_CONNECTION(alice, "alice7"_L1, "secret"_L1, "AlicePhone"_L1)
    CREATE_CONNECTION(alice1, "alice7"_L1, "secret"_L1, "AlicePhone"_L1)
    CREATE_CONNECTION(alice2, "alice7"_L1, "secret"_L1, "AlicePhone"_L1)

    auto olm = alice->olmAccount();
    olm->generateOneTimeKeys(10);
    const auto aliceUploadKeyRequest =
        alice->run(olm->createUploadKeyRequest(olm->oneTimeKeys()))
            .then([](const QHash<QString, int>& oneTimeKeyCounts) {
                QCOMPARE(oneTimeKeyCounts.value(SignedCurve25519Key), 10);
            });
    QVERIFY(waitForFuture(aliceUploadKeyRequest));

    auto olm1 = alice1->olmAccount();
    olm1->generateOneTimeKeys(10);
    const auto alice1UploadKeyRequest =
        alice1->run(olm1->createUploadKeyRequest(olm1->oneTimeKeys()))
            .then([](const QHash<QString, int>& oneTimeKeyCounts) {
                QCOMPARE(oneTimeKeyCounts.value(SignedCurve25519Key), 10);
            });
    QVERIFY(waitForFuture(alice1UploadKeyRequest));

    auto olm2 = alice2->olmAccount();
    olm2->generateOneTimeKeys(10);
    const auto alice2UploadKeyRequest =
        alice2->run(olm2->createUploadKeyRequest(olm2->oneTimeKeys()))
            .then([](const QHash<QString, int>& oneTimeKeyCounts) {
                QCOMPARE(oneTimeKeyCounts.value(SignedCurve25519Key), 10);
            });
    QVERIFY(waitForFuture(alice2UploadKeyRequest));

    // Bob will claim keys from all Alice's devices
    CREATE_CONNECTION(bob, "bob3"_L1, "secret"_L1, "BobPhone"_L1)

    QHash<QString, QHash<QString, QString>> oneTimeKeys;
    auto keyRequests = oneTimeKeys.insert(alice->userId(), {});
    for (const auto& c : { alice, alice1, alice2 })
        keyRequests->insert(c->deviceId(), SignedCurve25519Key);

    const auto claimResult =
        bob->callApi<ClaimKeysJob>(oneTimeKeys).then([alice](const ClaimKeysJob::Response& r) {
            QCOMPARE(r.oneTimeKeys.value(alice->userId()).size(), 3);
        });
    QVERIFY(waitForFuture(claimResult));
}

void TestOlmAccount::enableEncryption()
{
    CREATE_CONNECTION(alice, "alice9"_L1, "secret"_L1, "AlicePhone"_L1)

    const auto futureRoom = alice->createRoom(Connection::PublishRoom, {}, {}, {}, {})
                                .then([alice](const QString& roomId) {
                                    auto room = alice->room(roomId);
                                    room->activateEncryption();
                                    return room;
                                });
    QVERIFY(waitForFuture(futureRoom));
    alice->syncLoop();
    QVERIFY(QTest::qWaitFor([room = futureRoom.result()] { return room->usesEncryption(); }, 20000));
}

QTEST_GUILESS_MAIN(TestOlmAccount)
