// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "testolmutility.h"
#include <Quotient/e2ee/qolmaccount.h>
#include <Quotient/e2ee/qolmutility.h>

using namespace Quotient;

void TestOlmUtility::canonicalJSON()
{
    // Examples taken from
    // https://matrix.org/docs/spec/appendices.html#canonical-json
    auto data = QJsonDocument::fromJson(QByteArrayLiteral(R"({
    "auth": {
      "success": true,
      "mxid": "@john.doe:example.com",
      "profile": {
        "display_name": "John Doe",
        "three_pids": [{
          "medium": "email",
          "address": "john.doe@example.org"
        }, {
          "medium": "msisdn",
          "address": "123456789"
        }]
      }}})"));

    QCOMPARE(data.toJson(QJsonDocument::Compact),
      "{\"auth\":{\"mxid\":\"@john.doe:example.com\",\"profile\":{\"display_name\":\"John "
      "Doe\",\"three_pids\":[{\"address\":\"john.doe@example.org\",\"medium\":\"email\"},{"
      "\"address\":\"123456789\",\"medium\":\"msisdn\"}]},\"success\":true}}");

    auto data0 = QJsonDocument::fromJson(QByteArrayLiteral(R"({"b":"2","a":"1"})"));
    QCOMPARE(data0.toJson(QJsonDocument::Compact), "{\"a\":\"1\",\"b\":\"2\"}");

    auto data1 = QJsonDocument::fromJson(QByteArrayLiteral(R"({ "本": 2, "日": 1 })"));
    QCOMPARE(data1.toJson(QJsonDocument::Compact), "{\"日\":1,\"本\":2}");

    auto data2 = QJsonDocument::fromJson(QByteArrayLiteral(R"({"a": "\u65E5"})"));
    QCOMPARE(data2.toJson(QJsonDocument::Compact), "{\"a\":\"日\"}");

    auto data3 = QJsonDocument::fromJson(QByteArrayLiteral(R"({ "a": null })"));
    QCOMPARE(data3.toJson(QJsonDocument::Compact), "{\"a\":null}");
}

void TestOlmUtility::verifySignedOneTimeKey()
{
    auto aliceOlm = QOlmAccount::newAccount(this, QStringLiteral("RESTORE"), QStringLiteral("RESTORE"));
    aliceOlm->generateOneTimeKeys(1);
    auto keys = aliceOlm->oneTimeKeys();

    auto firstKey = *keys.keys.begin();
    auto msgObj = QJsonObject({{"key"_ls, firstKey}});
    auto sig = aliceOlm->sign(msgObj);

    auto msg = QJsonDocument(msgObj).toJson(QJsonDocument::Compact);


    QByteArray signatureBuf1(sig.size(), '\0');
    std::copy(sig.begin(), sig.end(), signatureBuf1.begin());

    // auto res =
    //     olm_ed25519_verify(utility,
    //                        aliceOlm.identityKeys().ed25519.toLatin1().data(),
    //                        aliceOlm.identityKeys().ed25519.size(), msg.data(),
    //                        msg.size(), sig.data(), sig.size());
    //
    // QCOMPARE(olm_utility_last_error_code(utility), OLM_SUCCESS);
    // QCOMPARE(res, 0);
    //
    // delete[](reinterpret_cast<uint8_t *>(utility));
    //
    // QOlmUtility utility2;
    // auto res2 =
    //     utility2.ed25519Verify(aliceOlm.identityKeys().ed25519.toLatin1(), msg,
    //                            signatureBuf1);
    //
    // //QCOMPARE(std::string(olm_utility_last_error(utility)), "SUCCESS");
    // QVERIFY(res2);
}

void TestOlmUtility::validUploadKeysRequest()
{
    const auto userId = u"@alice:matrix.org"_s;
    const auto deviceId = u"FKALSOCCC"_s;

    auto alice = QOlmAccount::newAccount(this, userId, deviceId);
    alice->generateOneTimeKeys(1);

    auto idSig = alice->signIdentityKeys();

    const QJsonObject body{
<<<<<<< HEAD
        { "algorithms"_L1, toJson(SupportedAlgorithms) },
        { "user_id"_L1, userId },
        { "device_id"_L1, deviceId },
        { "keys"_L1, QJsonObject{ { "curve25519:"_L1 + deviceId, alice.identityKeys().curve25519 },
                                  { "ed25519:"_L1 + deviceId, alice.identityKeys().ed25519 } } },
        { "signatures"_L1, QJsonObject{ { userId, QJsonObject{ { "ed25519:"_L1 + deviceId,
                                                                 QString::fromLatin1(idSig) } } } } }
||||||| parent of 4d89474e (libOlm is dead, long live vodozemac)
        { "algorithms"_ls, toJson(SupportedAlgorithms) },
        { "user_id"_ls, userId },
        { "device_id"_ls, deviceId },
        { "keys"_ls,
          QJsonObject{
              { "curve25519:"_ls + deviceId, alice.identityKeys().curve25519 },
              { "ed25519:"_ls + deviceId, alice.identityKeys().ed25519 } } },
        { "signatures"_ls,
          QJsonObject{
              { userId, QJsonObject{ { "ed25519:"_ls + deviceId,
                                       QString::fromLatin1(idSig) } } } } }
=======
        { "algorithms"_ls, toJson(SupportedAlgorithms) },
        { "user_id"_ls, userId },
        { "device_id"_ls, deviceId },
        { "keys"_ls,
          QJsonObject{
              { "curve25519:"_ls + deviceId, alice->identityKeys().curve25519 },
              { "ed25519:"_ls + deviceId, alice->identityKeys().ed25519 } } },
        { "signatures"_ls,
          QJsonObject{
              { userId, QJsonObject{ { "ed25519:"_ls + deviceId,
                                       QString::fromLatin1(idSig) } } } } }
>>>>>>> 4d89474e (libOlm is dead, long live vodozemac)
    };

    const auto deviceKeys = alice->deviceKeys();
    QCOMPARE(QJsonDocument(toJson(deviceKeys)).toJson(QJsonDocument::Compact),
            QJsonDocument(body).toJson(QJsonDocument::Compact));

    QVERIFY(verifyIdentitySignature(fromJson<DeviceKeys>(body), deviceId, userId));
    QVERIFY(verifyIdentitySignature(deviceKeys, deviceId, userId));
}
QTEST_GUILESS_MAIN(TestOlmUtility)
