// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
// SPDX-FileCopyrightText: 2020 mtxclient developers
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "testolmaccount.h"
#include "crypto/qolmaccount.h"
#include "crypto/qolmutility.h"
#include "connection.h"
#include "events/encryptedfile.h"
#include "networkaccessmanager.h"

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
    QVERIFY(QByteArray::fromBase64Encoding(curve25519).decodingStatus == QByteArray::Base64DecodingStatus::Ok);
    QVERIFY(QByteArray::fromBase64Encoding(ed25519).decodingStatus == QByteArray::Base64DecodingStatus::Ok);
}

void TestOlmAccount::signatureValid()
{
    QOlmAccount olmAccount(QStringLiteral("@foo:bar.com"), QStringLiteral("QuotientTestDevice"));
    olmAccount.createNewAccount();
    const auto message = "Hello world!";
    const auto signature = olmAccount.sign(message);
    QVERIFY(QByteArray::fromBase64Encoding(signature).decodingStatus == QByteArray::Base64DecodingStatus::Ok);

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
    device1.algorithms = QStringList {"m.olm.v1.curve25519-aes-sha2",
                           "m.megolm.v1.aes-sha2"};

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

#define CREATE_CONNECTION(VAR, USERNAME, SECRET, DEVICE_NAME) \
    NetworkAccessManager::instance()->ignoreSslErrors(true); \
    auto VAR = std::make_shared<Connection>(); \
    (VAR) ->resolveServer("@alice:localhost:" + QString::number(443)); \
    connect( (VAR) .get(), &Connection::loginFlowsChanged, this, [this, VAR ] () { \
        (VAR) ->loginWithPassword( (USERNAME) , SECRET , DEVICE_NAME , ""); \
    }); \
    connect( (VAR) .get(), &Connection::networkError, [=](QString error, const QString &, int, int) { \
        QFAIL("Network error: make sure synapse is running"); \
    }); \
    connect( (VAR) .get(), &Connection::loginError, [=](QString error, const QString &) { \
        QFAIL("Login failed"); \
    }); \
    QSignalSpy spy ## VAR ((VAR).get(), &Connection::loginFlowsChanged); \
    QSignalSpy spy2 ## VAR ((VAR).get(), &Connection::connected); \
    QVERIFY(spy ## VAR .wait(10000)); \
    QVERIFY(spy2 ## VAR .wait(10000));

void TestOlmAccount::uploadIdentityKey()
{
    CREATE_CONNECTION(conn, "alice", "secret", "AlicePhone")

    auto olmAccount = conn->olmAccount();
    auto idKeys = olmAccount->identityKeys();

    QVERIFY(idKeys.curve25519.size() > 10);

    OneTimeKeys unused;
    auto request = olmAccount->createUploadKeyRequest(unused);
    connect(request, &BaseJob::result, this, [request, conn](BaseJob *job) {
        auto job2 = static_cast<UploadKeysJob *>(job);
        QCOMPARE(job2->oneTimeKeyCounts().size(), 0);
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
    connect(request, &BaseJob::result, this, [request, conn](BaseJob *job) {
        auto job2 = static_cast<UploadKeysJob *>(job);
        QCOMPARE(job2->oneTimeKeyCounts().size(), 1);
        QCOMPARE(job2->oneTimeKeyCounts()["curve25519"], 5);
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
    connect(request, &BaseJob::result, this, [request, nKeys, conn](BaseJob *job) {
        auto job2 = static_cast<UploadKeysJob *>(job);
        QCOMPARE(job2->oneTimeKeyCounts().size(), 1);
        QCOMPARE(job2->oneTimeKeyCounts()["signed_curve25519"], nKeys);
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
    connect(request, &BaseJob::result, this, [request, conn](BaseJob *job) {
        auto job2 = static_cast<UploadKeysJob *>(job);
        QCOMPARE(job2->oneTimeKeyCounts().size(), 1);
        QCOMPARE(job2->oneTimeKeyCounts()["signed_curve25519"], 1);
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
        QCOMPARE(aliceRes->oneTimeKeyCounts()["signed_curve25519"], 1);
    });
    QSignalSpy spy(aliceRes, &BaseJob::result);
    alice->run(aliceRes);
    QVERIFY(spy.wait(10000));

    auto bobOlm = bob->olmAccount();
    bobOlm->generateOneTimeKeys(1);
    auto bobRes = bobOlm->createUploadKeyRequest(aliceOlm->oneTimeKeys());
    connect(bobRes, &BaseJob::result, this, [bobRes] {

        QCOMPARE(bobRes->oneTimeKeyCounts().size(), 1);
        QCOMPARE(bobRes->oneTimeKeyCounts()["signed_curve25519"], 1);
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
        connect(job, &BaseJob::result, this, [job, &bob, &bobOlm] {
            QCOMPARE(job->failures().size(), 0);

            auto aliceDevices = job->deviceKeys()[bob->userId()];
            QVERIFY(aliceDevices.size() > 0);

            auto devKeys = aliceDevices[bob->deviceId()];
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
        connect(job, &BaseJob::result, this, [job, &alice, &aliceOlm] {
            QCOMPARE(job->failures().size(), 0);

            auto bobDevices = job->deviceKeys()[alice->userId()];
            QVERIFY(bobDevices.size() > 0);

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
    CREATE_CONNECTION(bob, "alice", "secret", "AlicePhone")

    // Bob uploads his keys.
    auto *bobOlm = bob->olmAccount();
    bobOlm->generateOneTimeKeys(1);
    auto request = bobOlm->createUploadKeyRequest(bobOlm->oneTimeKeys());

    connect(request, &BaseJob::result, this, [request, bob](BaseJob *job) {
        auto job2 = static_cast<UploadKeysJob *>(job);
        QCOMPARE(job2->oneTimeKeyCounts().size(), 1);
        QCOMPARE(job2->oneTimeKeyCounts()["signed_curve25519"], 1);
    });
    bob->run(request);

    QSignalSpy requestSpy(request, &BaseJob::result);
    QVERIFY(requestSpy.wait(10000));

    // Alice retrieves bob's keys & claims one signed one-time key.
    auto *aliceOlm = alice->olmAccount();
    QHash<QString, QStringList> deviceKeys;
    deviceKeys[bob->userId()] = QStringList();
    auto job = alice->callApi<QueryKeysJob>(deviceKeys);
    connect(job, &BaseJob::result, this, [bob, alice, aliceOlm, job, this] {
        auto bobDevices = job->deviceKeys()[bob->userId()];
        QVERIFY(bobDevices.size() > 0);

        // Retrieve the identity key for the current device.
        auto bobEd25519 =
          bobDevices[bob->deviceId()].keys["ed25519:" + bob->deviceId()];

        const auto currentDevice = bobDevices[bob->deviceId()];

        // Verify signature.
        QVERIFY(verifyIdentitySignature(currentDevice, bob->deviceId(), bob->userId()));

        QHash<QString, QHash<QString, QString>> oneTimeKeys;
        oneTimeKeys[bob->userId()] = QHash<QString, QString>();
        oneTimeKeys[bob->userId()][bob->deviceId()] = SignedCurve25519Key;

        auto job = alice->callApi<ClaimKeysJob>(oneTimeKeys);
        connect(job, &BaseJob::result, this, [aliceOlm, bob, bobEd25519, job] {
            const auto userId = bob->userId();
            const auto deviceId = bob->deviceId();

            // The device exists.
            QCOMPARE(job->oneTimeKeys().size(), 1);
            QCOMPARE(job->oneTimeKeys()[userId].size(), 1);

            // The key is the one bob sent.
            auto oneTimeKey = job->oneTimeKeys()[userId][deviceId];
            QVERIFY(oneTimeKey.canConvert<QVariantMap>());

            QVariantMap varMap = oneTimeKey.toMap();
            bool found = false;
            for (const auto &key : varMap.keys()) {
                if (key.startsWith(QStringLiteral("signed_curve25519"))) {
                    found = true;
                }
            }
            QVERIFY(found);

            //auto algo = oneTimeKey.begin().key();
            //auto contents = oneTimeKey.begin().value();
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
        QCOMPARE(res->oneTimeKeyCounts()["signed_curve25519"], 10);
    });
    alice->run(res);

    auto olm1 = alice1->olmAccount();
    olm1->generateOneTimeKeys(10);
    auto res1 = olm1->createUploadKeyRequest(olm1->oneTimeKeys());
    QSignalSpy spy1(res1, &BaseJob::result);
    connect(res1, &BaseJob::result, this, [res1] {
        QCOMPARE(res1->oneTimeKeyCounts().size(), 1);
        QCOMPARE(res1->oneTimeKeyCounts()["signed_curve25519"], 10);
    });
    alice1->run(res1);

    auto olm2 = alice2->olmAccount();
    olm2->generateOneTimeKeys(10);
    auto res2 = olm2->createUploadKeyRequest(olm2->oneTimeKeys());
    QSignalSpy spy2(res2, &BaseJob::result);
    connect(res2, &BaseJob::result, this, [res2] {
        QCOMPARE(res2->oneTimeKeyCounts().size(), 1);
        QCOMPARE(res2->oneTimeKeyCounts()["signed_curve25519"], 10);
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
        const auto deviceId = bob->deviceId();

        // The device exists.
        QCOMPARE(job->oneTimeKeys().size(), 1);
        QCOMPARE(job->oneTimeKeys()[userId].size(), 3);
    });
}
QTEST_MAIN(TestOlmAccount)
