// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
// SPDX-FileCopyrightText: 2020 mtxclient developers
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "testolmaccount.h"

#include <connection.h>
#include <csapi/joining.h>
#include <e2ee/qolmaccount.h>
#include <e2ee/qolmutility.h>
#include <events/encryptionevent.h>
#include <events/filesourceinfo.h>
#include <networkaccessmanager.h>
#include <room.h>

#include "testutils.h"

using namespace Quotient;

void TestOlmAccount::pickleUnpickledTest()
{
    QOlmAccount olmAccount(QStringLiteral("@foo:bar.com"), QStringLiteral("QuotientTestDevice"));
    olmAccount.createNewAccount();
    auto identityKeys = olmAccount.identityKeys();
    auto pickled = olmAccount.pickle(Unencrypted{});
    QOlmAccount olmAccount2(QStringLiteral("@foo:bar.com"), QStringLiteral("QuotientTestDevice"));
    auto unpickleResult = olmAccount2.unpickle(std::move(pickled),
                                               Unencrypted{});
    QCOMPARE(unpickleResult, 0);
    auto identityKeys2 = olmAccount2.identityKeys();
    QCOMPARE(identityKeys.curve25519, identityKeys2.curve25519);
    QCOMPARE(identityKeys.ed25519, identityKeys2.ed25519);
}

void TestOlmAccount::identityKeysValid()
{
    QOlmAccount olmAccount(QStringLiteral("@foo:bar.com"), QStringLiteral("QuotientTestDevice"));
    olmAccount.createNewAccount();
    const auto identityKeys = olmAccount.identityKeys();
    const auto curve25519 = identityKeys.curve25519;
    const auto ed25519 = identityKeys.ed25519;
    // verify encoded keys length
    QCOMPARE(curve25519.size(), 43);
    QCOMPARE(ed25519.size(), 43);

    // encoded as valid base64?
    QVERIFY(QByteArray::fromBase64(curve25519).size() > 0);
    QVERIFY(QByteArray::fromBase64(ed25519).size() > 0);
}

void TestOlmAccount::signatureValid()
{
    QOlmAccount olmAccount(QStringLiteral("@foo:bar.com"), QStringLiteral("QuotientTestDevice"));
    olmAccount.createNewAccount();
    const auto message = "Hello world!";
    const auto signature = olmAccount.sign(message);
    QVERIFY(QByteArray::fromBase64(signature).size() > 0);

    QOlmUtility utility;
    const auto identityKeys = olmAccount.identityKeys();
    const auto ed25519Key = identityKeys.ed25519;
    const auto verify = utility.ed25519Verify(ed25519Key, message, signature);
    QVERIFY(verify.value_or(false));
}

void TestOlmAccount::oneTimeKeysValid()
{
    QOlmAccount olmAccount(QStringLiteral("@foo:bar.com"), QStringLiteral("QuotientTestDevice"));
    olmAccount.createNewAccount();
    const auto maxNumberOfOneTimeKeys = olmAccount.maxNumberOfOneTimeKeys();
    QCOMPARE(100, maxNumberOfOneTimeKeys);

    const auto oneTimeKeysEmpty = olmAccount.oneTimeKeys();
    QVERIFY(oneTimeKeysEmpty.curve25519().isEmpty());

    olmAccount.generateOneTimeKeys(20);
    const auto oneTimeKeysFilled = olmAccount.oneTimeKeys();
    QCOMPARE(20, oneTimeKeysFilled.curve25519().count());
}

void TestOlmAccount::deviceKeys()
{
    // copied from mtxclient
    DeviceKeys device1;
    device1.userId = "@alice:example.com";
    device1.deviceId = "JLAFKJWSCS";
    device1.keys = {{"curve25519:JLAFKJWSCS", "3C5BFWi2Y8MaVvjM8M22DBmh24PmgR0nPvJOIArzgyI"},
                    {"ed25519:JLAFKJWSCS", "lEuiRJBit0IG6nUf5pUzWTUEsRVVe/HJkoKuEww9ULI"}};

    // TODO that should be the default value
    device1.algorithms =
        QStringList { OlmV1Curve25519AesSha2AlgoKey, MegolmV1AesSha2AlgoKey };

    device1.signatures = {
      {"@alice:example.com",
       {{"ed25519:JLAFKJWSCS",
         "dSO80A01XiigH3uBiDVx/EjzaoycHcjq9lfQX0uWsqxl2giMIiSPR8a4d291W1ihKJL/"
         "a+myXS367WT6NAIcBA"}}}};

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

    QCOMPARE(file.v, "v2");
    QCOMPARE(file.iv, "w+sE15fzSc0AAAAAAAAAAA");
    QCOMPARE(file.hashes["sha256"], "fdSLu/YkRx3Wyh3KQabP3rd6+SFiKg5lsJZQHtkSAYA");
    QCOMPARE(file.key.alg, "A256CTR");
    QCOMPARE(file.key.ext, true);
    QCOMPARE(file.key.k, "aWF6-32KGYaC3A_FEUCk1Bt0JA37zP0wrStgmdCaW-0");
    QCOMPARE(file.key.keyOps.count(), 2);
    QCOMPARE(file.key.kty, "oct");
}

void TestOlmAccount::uploadIdentityKey()
{
    CREATE_CONNECTION(conn, "alice1", "secret", "AlicePhone")

    auto olmAccount = conn->olmAccount();
    auto idKeys = olmAccount->identityKeys();

    QVERIFY(idKeys.curve25519.size() > 10);

    UnsignedOneTimeKeys unused;
    auto request = olmAccount->createUploadKeyRequest(unused);
    connect(request, &BaseJob::result, this, [request, conn] {
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
    CREATE_CONNECTION(conn, "alice2", "secret", "AlicePhone")
    auto olmAccount = conn->olmAccount();

    auto nKeys = olmAccount->generateOneTimeKeys(5);
    QCOMPARE(nKeys, 5);

    auto oneTimeKeys = olmAccount->oneTimeKeys();

    OneTimeKeys oneTimeKeysHash;
    const auto curve = oneTimeKeys.curve25519();
    for (const auto &[keyId, key] : asKeyValueRange(curve)) {
        oneTimeKeysHash["curve25519:"+keyId] = key;
    }
    auto request = new UploadKeysJob(none, oneTimeKeysHash);
    connect(request, &BaseJob::result, this, [request, conn] {
        if (!request->status().good())
            QFAIL("upload failed");
        QCOMPARE(request->oneTimeKeyCounts().value(Curve25519Key), 5);
    });
    conn->run(request);
    QSignalSpy spy3(request, &BaseJob::result);
    QVERIFY(spy3.wait(10000));
}

void TestOlmAccount::uploadSignedOneTimeKeys()
{
    CREATE_CONNECTION(conn, "alice3", "secret", "AlicePhone")
    auto olmAccount = conn->olmAccount();
    auto nKeys = olmAccount->generateOneTimeKeys(5);
    QCOMPARE(nKeys, 5);

    auto oneTimeKeys = olmAccount->oneTimeKeys();
    OneTimeKeys oneTimeKeysHash;
    const auto signedKey = olmAccount->signOneTimeKeys(oneTimeKeys);
    for (const auto &[keyId, key] : asKeyValueRange(signedKey)) {
        oneTimeKeysHash[keyId] = key;
    }
    auto request = new UploadKeysJob(none, oneTimeKeysHash);
    connect(request, &BaseJob::result, this, [request, nKeys, conn] {
        if (!request->status().good())
            QFAIL("upload failed");
        QCOMPARE(request->oneTimeKeyCounts().value(SignedCurve25519Key), nKeys);
    });
    conn->run(request);
    QSignalSpy spy3(request, &BaseJob::result);
    QVERIFY(spy3.wait(10000));
}

void TestOlmAccount::uploadKeys()
{
    CREATE_CONNECTION(conn, "alice4", "secret", "AlicePhone")
    auto olmAccount = conn->olmAccount();
    auto idks = olmAccount->identityKeys();
    olmAccount->generateOneTimeKeys(1);
    auto otks = olmAccount->oneTimeKeys();
    auto request = olmAccount->createUploadKeyRequest(otks);
    connect(request, &BaseJob::result, this, [request, conn] {
        if (!request->status().good())
            QFAIL("upload failed");
        QCOMPARE(request->oneTimeKeyCounts().value(SignedCurve25519Key), 1);
    });
    conn->run(request);
    QSignalSpy spy3(request, &BaseJob::result);
    QVERIFY(spy3.wait(10000));
}

void TestOlmAccount::queryTest()
{
    CREATE_CONNECTION(alice, "alice5", "secret", "AlicePhone")
    CREATE_CONNECTION(bob, "bob1", "secret", "BobPhone")

    // Create and upload keys for both users.
    auto aliceOlm = alice->olmAccount();
    aliceOlm->generateOneTimeKeys(1);
    auto aliceRes = aliceOlm->createUploadKeyRequest(aliceOlm->oneTimeKeys());
    connect(aliceRes, &BaseJob::result, this, [aliceRes] {
        QCOMPARE(aliceRes->oneTimeKeyCounts().value(SignedCurve25519Key), 1);
    });
    QSignalSpy spy(aliceRes, &BaseJob::result);
    alice->run(aliceRes);
    QVERIFY(spy.wait(10000));

    auto bobOlm = bob->olmAccount();
    bobOlm->generateOneTimeKeys(1);
    auto bobRes = bobOlm->createUploadKeyRequest(aliceOlm->oneTimeKeys());
    connect(bobRes, &BaseJob::result, this, [bobRes] {
        QCOMPARE(bobRes->oneTimeKeyCounts().value(SignedCurve25519Key), 1);
    });
    QSignalSpy spy1(bobRes, &BaseJob::result);
    bob->run(bobRes);
    QVERIFY(spy1.wait(10000));

    {
        // Each user is requests each other's keys.
        QHash<QString, QStringList> deviceKeys;
        deviceKeys[bob->userId()] = QStringList();
        auto job = alice->callApi<QueryKeysJob>(deviceKeys);
        QSignalSpy spy(job, &BaseJob::result);
        connect(job, &BaseJob::result, this, [job, bob, bobOlm] {
            QCOMPARE(job->failures().size(), 0);

            const auto& aliceDevices = job->deviceKeys().value(bob->userId());
            QVERIFY(!aliceDevices.empty());

            const auto& devKeys = aliceDevices.value(bob->deviceId());
            QCOMPARE(devKeys.userId, bob->userId());
            QCOMPARE(devKeys.deviceId, bob->deviceId());
            QCOMPARE(devKeys.keys, bobOlm->deviceKeys().keys);
            QCOMPARE(devKeys.signatures, bobOlm->deviceKeys().signatures);
        });
        QVERIFY(spy.wait(10000));
    }

    {
        QHash<QString, QStringList> deviceKeys;
        deviceKeys[alice->userId()] = QStringList();
        auto job = bob->callApi<QueryKeysJob>(deviceKeys);
        QSignalSpy spy(job, &BaseJob::result);
        connect(job, &BaseJob::result, this, [job, alice, aliceOlm] {
            QCOMPARE(job->failures().size(), 0);

            const auto& bobDevices = job->deviceKeys().value(alice->userId());
            QVERIFY(!bobDevices.empty());

            auto devKeys = bobDevices[alice->deviceId()];
            QCOMPARE(devKeys.userId, alice->userId());
            QCOMPARE(devKeys.deviceId, alice->deviceId());
            QCOMPARE(devKeys.keys, aliceOlm->deviceKeys().keys);
            QCOMPARE(devKeys.signatures, aliceOlm->deviceKeys().signatures);
        });
        QVERIFY(spy.wait(10000));
    }
}

void TestOlmAccount::claimKeys()
{
    CREATE_CONNECTION(alice, "alice6", "secret", "AlicePhone")
    CREATE_CONNECTION(bob, "bob2", "secret", "BobPhone")

    // Bob uploads his keys.
    auto *bobOlm = bob->olmAccount();
    bobOlm->generateOneTimeKeys(1);
    auto request = bobOlm->createUploadKeyRequest(bobOlm->oneTimeKeys());

    connect(request, &BaseJob::result, this, [request, bob] {
        QCOMPARE(request->oneTimeKeyCounts().value(SignedCurve25519Key), 1);
    });
    bob->run(request);

    QSignalSpy requestSpy(request, &BaseJob::result);
    QVERIFY(requestSpy.wait(10000));

    // Alice retrieves bob's keys & claims one signed one-time key.
    QHash<QString, QStringList> deviceKeys;
    deviceKeys[bob->userId()] = QStringList();
    auto queryKeysJob = alice->callApi<QueryKeysJob>(deviceKeys);
    QSignalSpy requestSpy2(queryKeysJob, &BaseJob::result);
    QVERIFY(requestSpy2.wait(10000));

    const auto& bobDevices = queryKeysJob->deviceKeys().value(bob->userId());
    QVERIFY(!bobDevices.empty());

    const auto currentDevice = bobDevices[bob->deviceId()];

    // Verify signature.
    QVERIFY(verifyIdentitySignature(currentDevice, bob->deviceId(),
                                    bob->userId()));
    // Retrieve the identity key for the current device.
    const auto& bobEd25519 =
        bobDevices.value(bob->deviceId()).keys["ed25519:" + bob->deviceId()];

    QHash<QString, QHash<QString, QString>> oneTimeKeys;
    oneTimeKeys[bob->userId()] = QHash<QString, QString>();
    oneTimeKeys[bob->userId()][bob->deviceId()] = SignedCurve25519Key;

    auto claimKeysJob = alice->callApi<ClaimKeysJob>(oneTimeKeys);
    connect(claimKeysJob, &BaseJob::result, this, [bob, bobEd25519, claimKeysJob] {
        const auto userId = bob->userId();
        const auto deviceId = bob->deviceId();

        // The device exists.
        QCOMPARE(claimKeysJob->oneTimeKeys().size(), 1);
        QCOMPARE(claimKeysJob->oneTimeKeys().value(userId).size(), 1);

        // The key is the one bob sent.
        const auto& oneTimeKeys =
            claimKeysJob->oneTimeKeys().value(userId).value(deviceId);
        for (auto it = oneTimeKeys.begin(); it != oneTimeKeys.end(); ++it) {
            if (it.key().startsWith(SignedCurve25519Key)
                && std::holds_alternative<SignedOneTimeKey>(it.value()))
                return;
        }
        QFAIL("The claimed one time key is not in /claim response");
    });
    QSignalSpy completionSpy(claimKeysJob, &BaseJob::result);
    QVERIFY(completionSpy.wait(10000));
}

void TestOlmAccount::claimMultipleKeys()
{
    // Login with alice multiple times
    CREATE_CONNECTION(alice, "alice7", "secret", "AlicePhone")
    CREATE_CONNECTION(alice1, "alice7", "secret", "AlicePhone")
    CREATE_CONNECTION(alice2, "alice7", "secret", "AlicePhone")

    auto olm = alice->olmAccount();
    olm->generateOneTimeKeys(10);
    auto res = olm->createUploadKeyRequest(olm->oneTimeKeys());
    QSignalSpy spy(res, &BaseJob::result);
    connect(res, &BaseJob::result, this, [res] {
        QCOMPARE(res->oneTimeKeyCounts().value(SignedCurve25519Key), 10);
    });
    alice->run(res);
    QVERIFY(spy.wait(10000));

    auto olm1 = alice1->olmAccount();
    olm1->generateOneTimeKeys(10);
    auto res1 = olm1->createUploadKeyRequest(olm1->oneTimeKeys());
    QSignalSpy spy1(res1, &BaseJob::result);
    connect(res1, &BaseJob::result, this, [res1] {
        QCOMPARE(res1->oneTimeKeyCounts().value(SignedCurve25519Key), 10);
    });
    alice1->run(res1);
    QVERIFY(spy1.wait(10000));

    auto olm2 = alice2->olmAccount();
    olm2->generateOneTimeKeys(10);
    auto res2 = olm2->createUploadKeyRequest(olm2->oneTimeKeys());
    QSignalSpy spy2(res2, &BaseJob::result);
    connect(res2, &BaseJob::result, this, [res2] {
        QCOMPARE(res2->oneTimeKeyCounts().value(SignedCurve25519Key), 10);
    });
    alice2->run(res2);
    QVERIFY(spy2.wait(10000));

    // Bob will claim all keys from alice
    CREATE_CONNECTION(bob, "bob3", "secret", "BobPhone")

    QStringList devices_;
    devices_ << alice->deviceId()
             << alice1->deviceId()
             << alice2->deviceId();

    QHash<QString, QHash<QString, QString>> oneTimeKeys;
    oneTimeKeys[alice->userId()] = QHash<QString, QString>();
    for (const auto &d : devices_) {
        oneTimeKeys[alice->userId()][d] = SignedCurve25519Key;
    }
    auto job = bob->callApi<ClaimKeysJob>(oneTimeKeys);
    QSignalSpy jobSpy(job, &BaseJob::finished);
    QVERIFY(jobSpy.wait(10000));
    const auto userId = alice->userId();

    QCOMPARE(job->oneTimeKeys().value(userId).size(), 3);
}

void TestOlmAccount::enableEncryption()
{
    CREATE_CONNECTION(alice, "alice9", "secret", "AlicePhone")

    auto job = alice->createRoom(Connection::PublishRoom, {}, {}, {}, {});
    QSignalSpy createRoomSpy(job, &BaseJob::success);
    QVERIFY(createRoomSpy.wait(10000));
    alice->sync();
    connect(alice.get(), &Connection::syncDone, this, [alice](){
        qDebug() << "foo";
        alice->sync();
    });
    while(alice->roomsCount(JoinState::Join) == 0) {
        QThread::sleep(100);
    }
    auto room = alice->rooms(JoinState::Join)[0];
    room->activateEncryption();
    QSignalSpy encryptionSpy(room, &Room::encryption);
    QVERIFY(encryptionSpy.wait(10000));
    QVERIFY(room->usesEncryption());
}

QTEST_GUILESS_MAIN(TestOlmAccount)
