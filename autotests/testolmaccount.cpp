// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "testolmaccount.h"
#include "crypto/qolmaccount.h"
#include "connection.h"
#include "events/encryptedfile.h"

using namespace Quotient;

void TestOlmAccount::pickleUnpickedTest()
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

    //let utility = OlmUtility::new();
    //let identity_keys = olm_account.parsed_identity_keys();
    //let ed25519_key = identity_keys.ed25519();
    //assert!(utility
    //    .ed25519_verify(&ed25519_key, message, &signature)
    //    .unwrap());
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
    device1.userId   = "@alice:example.com";
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

void TestOlmAccount::uploadIdentityKey()
{
    auto conn = new Connection();
    conn->resolveServer("@alice:localhost:" + QString::number(443));
    connect(conn, &Connection::loginFlowsChanged, this, [this, conn]() {
        conn->loginWithPassword("alice", "secret", "AlicePhone", "");
        connect(conn, &Connection::connected, this, [this, conn] {
            auto olmAccount = conn->olmAccount();
            auto idKeys = olmAccount->identityKeys();

            QVERIFY(idKeys.curve25519.size() > 10);
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
        });
        connect(conn, &Connection::networkError, [=](QString error, const QString &, int, int) {
            QFAIL("Network error: make sure synapse is running");
        });
        connect(conn, &Connection::loginError, [=](QString error, const QString &) {
            QFAIL("Login failed");
        });
    });

    connect(conn, &Connection::resolveError, this, [=](QString error) {
        QFAIL("Network error: make sure synapse is running");
    });
    connect(conn, &Connection::loginError, this, [=] {
        QFAIL("Network error: make sure synapse is running");
    });

    QSignalSpy spy(conn, &Connection::loginFlowsChanged);
    QSignalSpy spy2(conn, &Connection::connected);
    QVERIFY(spy.wait(10000));
    QVERIFY(spy2.wait(10000));
    delete conn;
}

void TestOlmAccount::uploadOneTimeKeys()
{
    auto conn = new Connection();
    conn->resolveServer("@alice:localhost:" + QString::number(443));
    connect(conn, &Connection::loginFlowsChanged, this, [this, conn]() {
        conn->loginWithPassword("alice", "secret", "AlicePhone", "");
        connect(conn, &Connection::connected, this, [this, conn] {
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
        });
        connect(conn, &Connection::networkError, [=](QString error, const QString &, int, int) {
            QFAIL("Network error: make sure synapse is running");
        });
        connect(conn, &Connection::loginError, [=](QString error, const QString &) {
            QFAIL("Login failed");
        });
    });

    connect(conn, &Connection::resolveError, this, [=](QString error) {
        QFAIL("Network error: make sure synapse is running");
    });
    connect(conn, &Connection::loginError, this, [=] {
        QFAIL("Network error: make sure synapse is running");
    });

    QSignalSpy spy(conn, &Connection::loginFlowsChanged);
    QSignalSpy spy2(conn, &Connection::connected);
    QVERIFY(spy.wait(10000));
    QVERIFY(spy2.wait(10000));
    delete conn;
}

void TestOlmAccount::uploadSignedOneTimeKeys()
{
    auto conn = new Connection();
    conn->resolveServer("@alice:localhost:" + QString::number(443));
    connect(conn, &Connection::loginFlowsChanged, this, [this, conn]() {
        conn->loginWithPassword("alice", "secret", "AlicePhone", "");
        connect(conn, &Connection::connected, this, [this, conn] {
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
        });
        connect(conn, &Connection::networkError, [=](QString error, const QString &, int, int) {
            QFAIL("Network error: make sure synapse is running");
        });
        connect(conn, &Connection::loginError, [=](QString error, const QString &) {
            QFAIL("Login failed");
        });
    });

    connect(conn, &Connection::resolveError, this, [=](QString error) {
        QFAIL("Network error: make sure synapse is running");
    });
    connect(conn, &Connection::loginError, this, [=] {
        QFAIL("Network error: make sure synapse is running");
    });

    QSignalSpy spy(conn, &Connection::loginFlowsChanged);
    QSignalSpy spy2(conn, &Connection::connected);
    QVERIFY(spy.wait(10000));
    QVERIFY(spy2.wait(10000));
    delete conn;
}

void TestOlmAccount::uploadKeys()
{
    auto conn = new Connection();
    conn->resolveServer("@alice:localhost:" + QString::number(443));
    connect(conn, &Connection::loginFlowsChanged, this, [this, conn]() {
        conn->loginWithPassword("alice", "secret", "AlicePhone", "");
        connect(conn, &Connection::connected, this, [this, conn] {
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
        });
        connect(conn, &Connection::networkError, [=](QString error, const QString &, int, int) {
            QFAIL("Network error: make sure synapse is running");
        });
        connect(conn, &Connection::loginError, [=](QString error, const QString &) {
            QFAIL("Login failed");
        });
    });

    connect(conn, &Connection::resolveError, this, [=](QString error) {
        QFAIL("Network error: make sure synapse is running");
    });
    connect(conn, &Connection::loginError, this, [=] {
        QFAIL("Network error: make sure synapse is running");
    });

    QSignalSpy spy(conn, &Connection::loginFlowsChanged);
    QSignalSpy spy2(conn, &Connection::connected);
    QVERIFY(spy.wait(10000));
    QVERIFY(spy2.wait(10000));
    delete conn;
}

QTEST_MAIN(TestOlmAccount)
