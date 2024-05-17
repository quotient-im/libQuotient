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

QTEST_GUILESS_MAIN(TestKeyImport)
#include "testkeyimport.moc"
