// SPDX-FileCopyrightText: 2024 Tobias Fella <tobias.fella@kde.org>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include <Quotient/e2ee/cryptoutils.h>

#include <QTest>

#include <Quotient/keyimport.h>

class TestKeyImport : public QObject
{
    Q_OBJECT
private slots:
    void testImport();
    void testExport();
};

using namespace Quotient;

void TestKeyImport::testImport()
{
    KeyImport keyImport;

    auto path = QString::fromUtf8(__FILE__);
    path = path.left(path.lastIndexOf(QDir::separator()));
    path += QStringLiteral("/key-export.data");
    QFile file(path);
    file.open(QIODevice::ReadOnly);
    auto data = file.readAll();
    QVERIFY(!data.isEmpty());
    const auto result = keyImport.decrypt(QString::fromUtf8(data), QStringLiteral("123passphrase"));
    QVERIFY(result.has_value());
    const auto &json = result.value();
    QCOMPARE(json.size(), 2);
}

void TestKeyImport::testExport()
{
    KeyImport keyImport;

    QJsonArray sessions;
    sessions += QJsonObject {
        {"algorithm"_ls, "m.megolm.v1.aes-sha2"_ls},
        {"forwarding_curve25519_key_chain"_ls, QJsonArray()},
        {"room_id"_ls, "!asdf:foo.bar"_ls},
        {"sender_claimed_keys"_ls, QJsonObject {
            {"ed25519"_ls, "asdfkey"_ls}
        }},
        {"sender_key"_ls, "senderkey"_ls},
        {"session_id"_ls, "sessionidasdf"_ls},
        {"session_key"_ls, "sessionkeyfoo"_ls},

    };
    auto result = keyImport.encrypt(sessions, QStringLiteral("a passphrase"));
    QVERIFY(result.has_value());
    QVERIFY(result.value().size() > 0);

    auto plain = keyImport.decrypt(QString::fromLatin1(result.value()), "a passphrase"_ls);
    QVERIFY(plain.has_value());
    auto value = plain.value();
    QCOMPARE(value.size(), 1);
    QCOMPARE(value[0]["algorithm"_ls].toString(), "m.megolm.v1.aes-sha2"_ls);
}

QTEST_GUILESS_MAIN(TestKeyImport)
#include "testkeyimport.moc"
