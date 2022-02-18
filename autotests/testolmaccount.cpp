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
#include <events/encryptedfile.h>
#include <networkaccessmanager.h>
#include <room.h>

using namespace Quotient;

void TestOlmAccount::pickleUnpickledTest()
{
    QOlmAccount olmAccount(QStringLiteral("@foo:bar.com"), QStringLiteral("QuotientTestDevice"));
    olmAccount.createNewAccount();
    auto identityKeys = olmAccount.identityKeys();
    auto pickled = std::get<QByteArray>(olmAccount.pickle(Unencrypted{}));
    QOlmAccount olmAccount2(QStringLiteral("@foo:bar.com"), QStringLiteral("QuotientTestDevice"));
    olmAccount2.unpickle(pickled, Unencrypted{});
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
    QVERIFY(std::holds_alternative<bool>(verify));
    QVERIFY(std::get<bool>(verify) == true);
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

    EncryptedFile file;
    JsonObjectConverter<EncryptedFile>::fillFrom(doc.object(), file);

    QCOMPARE(file.v, "v2");
    QCOMPARE(file.iv, "w+sE15fzSc0AAAAAAAAAAA");
    QCOMPARE(file.hashes["sha256"], "fdSLu/YkRx3Wyh3KQabP3rd6+SFiKg5lsJZQHtkSAYA");
    QCOMPARE(file.key.alg, "A256CTR");
    QCOMPARE(file.key.ext, true);
    QCOMPARE(file.key.k, "aWF6-32KGYaC3A_FEUCk1Bt0JA37zP0wrStgmdCaW-0");
    QCOMPARE(file.key.keyOps.count(), 2);
    QCOMPARE(file.key.kty, "oct");
}

#define CREATE_CONNECTION(VAR, USERNAME, SECRET, DEVICE_NAME)                  \
    NetworkAccessManager::instance()->ignoreSslErrors(true);                   \
    auto VAR = std::make_shared<Connection>();                                 \
    (VAR)->resolveServer("@" USERNAME ":localhost:1234");                      \
    connect((VAR).get(), &Connection::loginFlowsChanged, this, [=] {           \
        (VAR)->loginWithPassword((USERNAME), SECRET, DEVICE_NAME, "");         \
    });                                                                        \
    connect((VAR).get(), &Connection::networkError, [](const QString& error) { \
        QWARN(qUtf8Printable(error));                                          \
        QFAIL("Network error: make sure synapse is running");                  \
    });                                                                        \
    connect((VAR).get(), &Connection::loginError, [](const QString& error) {   \
        QWARN(qUtf8Printable(error));                                          \
        QFAIL("Login failed");                                                 \
    });                                                                        \
    QSignalSpy spy##VAR((VAR).get(), &Connection::loginFlowsChanged);          \
    QSignalSpy spy2##VAR((VAR).get(), &Connection::connected);                 \
    QVERIFY(spy##VAR.wait(10000));                                             \
    QVERIFY(spy2##VAR.wait(10000));

void TestOlmAccount::uploadIdentityKey()
{
    CREATE_CONNECTION(conn, "alice", "secret", "AlicePhone")

    auto olmAccount = conn->olmAccount();
    auto idKeys = olmAccount->identityKeys();

    QVERIFY(idKeys.curve25519.size() > 10);

    OneTimeKeys unused;
    auto request = olmAccount->createUploadKeyRequest(unused);
    connect(request, &BaseJob::result, this, [request, conn] {
        QCOMPARE(request->oneTimeKeyCounts().size(), 0);
    });
    connect(request, &BaseJob::failure, this, [] {
        QFAIL("upload failed");
    });
    conn->run(request);
    QSignalSpy spy3(request, &BaseJob::result);
    QVERIFY(spy3.wait(10000));
}

void TestOlmAccount::uploadOneTimeKeys()
{
    CREATE_CONNECTION(conn, "alice", "secret", "AlicePhone")
    auto olmAccount = conn->olmAccount();

    auto nKeys = olmAccount->generateOneTimeKeys(5);
    QCOMPARE(nKeys, 5);

    auto oneTimeKeys = olmAccount->oneTimeKeys();

    QHash<QString, QVariant> oneTimeKeysHash;
    const auto curve = oneTimeKeys.curve25519();
    for (const auto &[keyId, key] : asKeyValueRange(curve)) {
        oneTimeKeysHash["curve25519:"+keyId] = key;
    }
    auto request = new UploadKeysJob(none, oneTimeKeysHash);
    connect(request, &BaseJob::result, this, [request, conn] {
        QCOMPARE(request->oneTimeKeyCounts().size(), 1);
        QCOMPARE(request->oneTimeKeyCounts().value(Curve25519Key), 5);
    });
    connect(request, &BaseJob::failure, this, [] {
        QFAIL("upload failed");
    });
    conn->run(request);
    QSignalSpy spy3(request, &BaseJob::result);
    QVERIFY(spy3.wait(10000));
}

void TestOlmAccount::uploadSignedOneTimeKeys()
{
    CREATE_CONNECTION(conn, "alice", "secret", "AlicePhone")
    auto olmAccount = conn->olmAccount();
    auto nKeys = olmAccount->generateOneTimeKeys(5);
    QCOMPARE(nKeys, 5);

    auto oneTimeKeys = olmAccount->oneTimeKeys();
    QHash<QString, QVariant> oneTimeKeysHash;
    const auto signedKey = olmAccount->signOneTimeKeys(oneTimeKeys);
    for (const auto &[keyId, key] : asKeyValueRange(signedKey)) {
        QVariant var;
        var.setValue(key);
        oneTimeKeysHash[keyId] = var;
    }
    auto request = new UploadKeysJob(none, oneTimeKeysHash);
    connect(request, &BaseJob::result, this, [request, nKeys, conn] {
        QCOMPARE(request->oneTimeKeyCounts().size(), 1);
        QCOMPARE(request->oneTimeKeyCounts().value(SignedCurve25519Key), nKeys);
    });
    connect(request, &BaseJob::failure, this, [] {
        QFAIL("upload failed");
    });
    conn->run(request);
    QSignalSpy spy3(request, &BaseJob::result);
    QVERIFY(spy3.wait(10000));
}

void TestOlmAccount::uploadKeys()
{
    CREATE_CONNECTION(conn, "alice", "secret", "AlicePhone")
    auto olmAccount = conn->olmAccount();
    auto idks = olmAccount->identityKeys();
    olmAccount->generateOneTimeKeys(1);
    auto otks = olmAccount->oneTimeKeys();
    auto request = olmAccount->createUploadKeyRequest(otks);
    connect(request, &BaseJob::result, this, [request, conn] {
        QCOMPARE(request->oneTimeKeyCounts().size(), 1);
        QCOMPARE(request->oneTimeKeyCounts().value(SignedCurve25519Key), 1);
    });
    connect(request, &BaseJob::failure, this, [] {
        QFAIL("upload failed");
    });
    conn->run(request);
    QSignalSpy spy3(request, &BaseJob::result);
    QVERIFY(spy3.wait(10000));
}

void TestOlmAccount::queryTest()
{
    CREATE_CONNECTION(alice, "alice", "secret", "AlicePhone")
    CREATE_CONNECTION(bob, "bob", "secret", "BobPhone")

    // Create and upload keys for both users.
    auto aliceOlm = alice->olmAccount();
    aliceOlm->generateOneTimeKeys(1);
    auto aliceRes = aliceOlm->createUploadKeyRequest(aliceOlm->oneTimeKeys());
    connect(aliceRes, &BaseJob::result, this, [aliceRes] {
        QCOMPARE(aliceRes->oneTimeKeyCounts().size(), 1);
        QCOMPARE(aliceRes->oneTimeKeyCounts().value(SignedCurve25519Key), 1);
    });
    QSignalSpy spy(aliceRes, &BaseJob::result);
    alice->run(aliceRes);
    QVERIFY(spy.wait(10000));

    auto bobOlm = bob->olmAccount();
    bobOlm->generateOneTimeKeys(1);
    auto bobRes = bobOlm->createUploadKeyRequest(aliceOlm->oneTimeKeys());
    connect(bobRes, &BaseJob::result, this, [bobRes] {
        QCOMPARE(bobRes->oneTimeKeyCounts().size(), 1);
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
    CREATE_CONNECTION(alice, "alice", "secret", "AlicePhone")
    CREATE_CONNECTION(bob, "bob", "secret", "BobPhone")

    // Bob uploads his keys.
    auto *bobOlm = bob->olmAccount();
    bobOlm->generateOneTimeKeys(1);
    auto request = bobOlm->createUploadKeyRequest(bobOlm->oneTimeKeys());

    connect(request, &BaseJob::result, this, [request, bob] {
        QCOMPARE(request->oneTimeKeyCounts().size(), 1);
        QCOMPARE(request->oneTimeKeyCounts().value(SignedCurve25519Key), 1);
    });
    bob->run(request);

    QSignalSpy requestSpy(request, &BaseJob::result);
    QVERIFY(requestSpy.wait(10000));

    // Alice retrieves bob's keys & claims one signed one-time key.
    QHash<QString, QStringList> deviceKeys;
    deviceKeys[bob->userId()] = QStringList();
    auto job = alice->callApi<QueryKeysJob>(deviceKeys);
    connect(job, &BaseJob::result, this, [bob, alice, job, this] {
        const auto& bobDevices = job->deviceKeys().value(bob->userId());
        QVERIFY(!bobDevices.empty());

        // Retrieve the identity key for the current device.
        const auto& bobEd25519 =
            bobDevices.value(bob->deviceId()).keys["ed25519:" + bob->deviceId()];

        const auto currentDevice = bobDevices[bob->deviceId()];

        // Verify signature.
        QVERIFY(verifyIdentitySignature(currentDevice, bob->deviceId(),
                                        bob->userId()));

        QHash<QString, QHash<QString, QString>> oneTimeKeys;
        oneTimeKeys[bob->userId()] = QHash<QString, QString>();
        oneTimeKeys[bob->userId()][bob->deviceId()] = SignedCurve25519Key;

        auto job = alice->callApi<ClaimKeysJob>(oneTimeKeys);
        connect(job, &BaseJob::result, this, [bob, bobEd25519, job] {
            const auto userId = bob->userId();
            const auto deviceId = bob->deviceId();

            // The device exists.
            QCOMPARE(job->oneTimeKeys().size(), 1);
            QCOMPARE(job->oneTimeKeys().value(userId).size(), 1);

            // The key is the one bob sent.
            const auto& oneTimeKey =
                job->oneTimeKeys().value(userId).value(deviceId);
            QVERIFY(oneTimeKey.canConvert<QVariantMap>());

            const auto varMap = oneTimeKey.toMap();
            QVERIFY(std::any_of(varMap.constKeyValueBegin(),
                                varMap.constKeyValueEnd(), [](const auto& kv) {
                                    return kv.first.startsWith(
                                        SignedCurve25519Key);
                                }));
        });
    });
}

void TestOlmAccount::claimMultipleKeys()
{
    // Login with alice multiple times
    CREATE_CONNECTION(alice, "alice", "secret", "AlicePhone")
    CREATE_CONNECTION(alice1, "alice", "secret", "AlicePhone")
    CREATE_CONNECTION(alice2, "alice", "secret", "AlicePhone")

    auto olm = alice->olmAccount();
    olm->generateOneTimeKeys(10);
    auto res = olm->createUploadKeyRequest(olm->oneTimeKeys());
    QSignalSpy spy(res, &BaseJob::result);
    connect(res, &BaseJob::result, this, [res] {
        QCOMPARE(res->oneTimeKeyCounts().size(), 1);
        QCOMPARE(res->oneTimeKeyCounts().value(SignedCurve25519Key), 10);
    });
    alice->run(res);

    auto olm1 = alice1->olmAccount();
    olm1->generateOneTimeKeys(10);
    auto res1 = olm1->createUploadKeyRequest(olm1->oneTimeKeys());
    QSignalSpy spy1(res1, &BaseJob::result);
    connect(res1, &BaseJob::result, this, [res1] {
        QCOMPARE(res1->oneTimeKeyCounts().size(), 1);
        QCOMPARE(res1->oneTimeKeyCounts().value(SignedCurve25519Key), 10);
    });
    alice1->run(res1);

    auto olm2 = alice2->olmAccount();
    olm2->generateOneTimeKeys(10);
    auto res2 = olm2->createUploadKeyRequest(olm2->oneTimeKeys());
    QSignalSpy spy2(res2, &BaseJob::result);
    connect(res2, &BaseJob::result, this, [res2] {
        QCOMPARE(res2->oneTimeKeyCounts().size(), 1);
        QCOMPARE(res2->oneTimeKeyCounts().value(SignedCurve25519Key), 10);
    });
    alice2->run(res2);

    QVERIFY(spy.wait(10000));
    QVERIFY(spy1.wait(10000));
    QVERIFY(spy2.wait(1000)); // TODO this is failing even with 10000

    // Bob will claim all keys from alice
    CREATE_CONNECTION(bob, "bob", "secret", "BobPhone")

    QStringList devices_;
    devices_ << alice->deviceId()
             << alice1->deviceId()
             << alice2->deviceId();

    QHash<QString, QHash<QString, QString>> oneTimeKeys;
    for (const auto &d : devices_) {
        oneTimeKeys[alice->userId()] = QHash<QString, QString>();
        oneTimeKeys[alice->userId()][d] = SignedCurve25519Key;
    }
    auto job = bob->callApi<ClaimKeysJob>(oneTimeKeys);
    connect(job, &BaseJob::result, this, [bob, job] {
        const auto userId = bob->userId();

        // The device exists.
        QCOMPARE(job->oneTimeKeys().size(), 1);
        QCOMPARE(job->oneTimeKeys().value(userId).size(), 3);
    });
}

void TestOlmAccount::keyChange()
{
    CREATE_CONNECTION(alice, "alice", "secret", "AlicePhone")

    auto job = alice->createRoom(Connection::PublishRoom, {}, {}, {}, {});
    connect(job, &BaseJob::result, this, [alice, job, this] {
        QVERIFY(job->status().good());
        // Alice syncs to get the first next_batch token.
        alice->sync();
        connect(alice.get(), &Connection::syncDone, this, [alice, this] {
            const auto nextBatchToken = alice->nextBatchToken();

            // generate keys and change existing one
            auto aliceOlm = alice->olmAccount();
            aliceOlm->generateOneTimeKeys(1);
            auto aliceRes = aliceOlm->createUploadKeyRequest(aliceOlm->oneTimeKeys());
            QSignalSpy spy(aliceRes, &BaseJob::result);

            alice->run(aliceRes);
            QVERIFY(spy.wait(10000));

            // The key changes should contain her username
            // because of the key uploading.

            auto changeJob = alice->callApi<GetKeysChangesJob>(nextBatchToken, "");
            connect(changeJob, &BaseJob::result, this, [changeJob, alice] {
                QCOMPARE(changeJob->changed().size(), 1);
                QCOMPARE(changeJob->left().size(), 0);
                QCOMPARE(*changeJob->changed().cbegin(), alice->userId());
            });
            QSignalSpy spy2(changeJob, &BaseJob::result);
            QVERIFY(spy2.wait(10000));
        });
        QSignalSpy spy2(alice.get(), &Connection::syncDone);
        QVERIFY(spy2.wait(10000));
    });
    QSignalSpy spy(job, &BaseJob::result);
    QVERIFY(spy.wait(10000));
}

void TestOlmAccount::enableEncryption()
{
    CREATE_CONNECTION(alice, "alice", "secret", "AlicePhone")
    CREATE_CONNECTION(bob, "bob", "secret", "BobPhone")

    QString joinedRoomId;

    auto job = alice->createRoom(Connection::PublishRoom, {}, {}, {},
                                 { "@bob:localhost" });
    connect(alice.get(), &Connection::newRoom, this,
            [alice, bob, &joinedRoomId](Quotient::Room* room) {
                room->activateEncryption();
                QSignalSpy spy(room, &Room::encryption);

                joinedRoomId = room->id();
                auto job = bob->joinRoom(room->id());
                QSignalSpy spy1(job, &BaseJob::result);
                QVERIFY(spy.wait(10000));
                QVERIFY(spy1.wait(10000));
            });

    QSignalSpy spy(job, &BaseJob::result);
    QVERIFY(spy.wait(10000));

    bob->sync();
    connect(bob.get(), &Connection::syncDone, this, [bob, joinedRoomId] {
        const auto* joinedRoom = bob->room(joinedRoomId);
        const auto& events = joinedRoom->messageEvents();
        bool hasEncryption = false;
        for (auto it = events.rbegin(); it != events.rend(); ++it) {
            const auto& event = it->event();
            if (eventCast<const EncryptedEvent>(event)) {
                hasEncryption = true;
            } else {
                qDebug() << event->matrixType() << typeId<EncryptedEvent>()
                         << event->type();
                if (is<EncryptionEvent>(*event)) {
                    qDebug() << event->contentJson();
                }
            }
        }
        QVERIFY(bob->room(joinedRoomId)->usesEncryption());
        QVERIFY(hasEncryption);
    });
    QSignalSpy spy2(bob.get(), &Connection::syncDone);
    QVERIFY(spy2.wait(10000));
}

QTEST_GUILESS_MAIN(TestOlmAccount)
