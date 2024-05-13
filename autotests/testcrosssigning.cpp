// SPDX-FileCopyrightText: 2023 Tobias Fella <tobias.fella@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later


#include <QTest>
#include "connection_p.h"
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
        path += QStringLiteral("/cross_signing_data.json");
        QFile file(path);
        file.open(QIODevice::ReadOnly);
        auto data = file.readAll();
        QVERIFY(!data.isEmpty());
        auto json = QJsonDocument::fromJson(data).object();

        auto connection = Connection::makeMockConnection("@tobiasfella:kde.org"_ls, true);
        connection->d->encryptionData->handleQueryKeys(fromJson<QHash<QString, QHash<QString, QueryKeysJob::DeviceInformation>>>(json["device_keys"_ls]),
            fromJson<QHash<QString, CrossSigningKey>>(json["master_keys"_ls]), fromJson<QHash<QString, CrossSigningKey>>(json["self_signing_keys"_ls]),
                                       fromJson<QHash<QString, CrossSigningKey>>(json["user_signing_keys"_ls]));

        QVERIFY(!connection->isUserVerified("@tobiasfella:kde.org"_ls));
        QVERIFY(!connection->isUserVerified("@carl:kde.org"_ls));
        QVERIFY(!connection->isUserVerified("@eve:foo.bar"_ls));
        QVERIFY(!connection->isUserVerified("@aloy:kde.org"_ls));
        QVERIFY(!connection->isVerifiedDevice("@tobiasfella:kde.org"_ls, "LTLVYDIVMO"_ls));
        connection->database()->setMasterKeyVerified("iiNvK2+mJtBXj6t+FVnaPBZ4e/M/n84wPJBfUVN38OE"_ls);
        QVERIFY(connection->isUserVerified("@tobiasfella:kde.org"_ls));
        connection->d->encryptionData->handleQueryKeys(fromJson<QHash<QString, QHash<QString, QueryKeysJob::DeviceInformation>>>(json["device_keys"_ls]),
            fromJson<QHash<QString, CrossSigningKey>>(json["master_keys"_ls]), fromJson<QHash<QString, CrossSigningKey>>(json["self_signing_keys"_ls]),
                                       fromJson<QHash<QString, CrossSigningKey>>(json["user_signing_keys"_ls]));
        QVERIFY(connection->isUserVerified("@tobiasfella:kde.org"_ls));
        QVERIFY(connection->isUserVerified("@aloy:kde.org"_ls));
        QVERIFY(!connection->isVerifiedDevice("@tobiasfella:kde.org"_ls, "IDEJTUJQAF"_ls));
        QVERIFY(!connection->isVerifiedDevice("@tobiasfella:kde.org"_ls, "DEADBEEF"_ls));
        QVERIFY(connection->isVerifiedDevice("@tobiasfella:kde.org"_ls, "LTLVYDIVMO"_ls));
        QVERIFY(connection->isVerifiedDevice("@aloy:kde.org"_ls, "BUJGIJTVJB"_ls));
        QVERIFY(!connection->isVerifiedDevice("@aloy:kde.org"_ls, "VJIBVDKIST"_ls));
    }
};
QTEST_GUILESS_MAIN(TestCrossSigning)
#include "testcrosssigning.moc"
