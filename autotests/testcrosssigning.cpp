// SPDX-FileCopyrightText: 2023 Tobias Fella <tobias.fella@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later


#include <QTest>
#include <Quotient/connection_p.h>
#include "testutils.h"

using namespace Quotient;

class TestCrossSigning : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void test1()
    {
        auto path = QString::fromUtf8(__FILE__);
        path = path.left(path.lastIndexOf(QDir::separator()));
        path += "/cross_signing_data.json"_L1;
        QFile file(path);
        file.open(QIODevice::ReadOnly);
        auto data = file.readAll();
        QVERIFY(!data.isEmpty());
        auto jobMock = Mocked<QueryKeysJob>(QHash<QString, QStringList>{});
        jobMock.setResult(QJsonDocument::fromJson(data));
        auto mockKeys = collectResponse(&jobMock);

        auto connection = Connection::makeMockConnection("@tobiasfella:kde.org"_L1, true);
        connection->d->encryptionData->handleQueryKeys(mockKeys);

        QVERIFY(!connection->isUserVerified("@tobiasfella:kde.org"_L1));
        QVERIFY(!connection->isUserVerified("@carl:kde.org"_L1));
        QVERIFY(!connection->isUserVerified("@eve:foo.bar"_L1));
        QVERIFY(!connection->isUserVerified("@aloy:kde.org"_L1));
        QVERIFY(!connection->isVerifiedDevice("@tobiasfella:kde.org"_L1, "LTLVYDIVMO"_L1));
        connection->database()->setMasterKeyVerified("iiNvK2+mJtBXj6t+FVnaPBZ4e/M/n84wPJBfUVN38OE"_L1);
        QVERIFY(connection->isUserVerified("@tobiasfella:kde.org"_L1));
        connection->d->encryptionData->handleQueryKeys(mockKeys);

        QVERIFY(connection->isUserVerified("@tobiasfella:kde.org"_L1));
        QVERIFY(connection->isUserVerified("@aloy:kde.org"_L1));
        QVERIFY(!connection->isVerifiedDevice("@tobiasfella:kde.org"_L1, "IDEJTUJQAF"_L1));
        QVERIFY(!connection->isVerifiedDevice("@tobiasfella:kde.org"_L1, "DEADBEEF"_L1));
        QVERIFY(connection->isVerifiedDevice("@tobiasfella:kde.org"_L1, "LTLVYDIVMO"_L1));
        QVERIFY(connection->isVerifiedDevice("@aloy:kde.org"_L1, "BUJGIJTVJB"_L1));
        QVERIFY(!connection->isVerifiedDevice("@aloy:kde.org"_L1, "VJIBVDKIST"_L1));
    }
};
QTEST_GUILESS_MAIN(TestCrossSigning)
#include "testcrosssigning.moc"
