// SPDX-FileCopyrightText: 2024 Joshua Goins <josh@redstrate.com>
// SPDX-License-Identifier: MIT

#include <blurhash.h>

#include <QTest>

class TestBlurHash : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void decodeImage();
    void encodeImage();
};
void TestBlurHash::decodeImage()
{
    const auto image = Quotient::BlurHash::decode(QStringLiteral("eBB4=;054UK$=402%s%|r^O%06#?*7RijMxGpYMzniVNT@rFN3#=Kt"), QSize(50, 50));
    QVERIFY(!image.isNull());

    QCOMPARE(image.width(), 50);
    QCOMPARE(image.height(), 50);
    QCOMPARE(image.pixelColor(0, 0), QColor(0xff005f00));
    QCOMPARE(image.pixelColor(30, 30), QColor(0xff99b76d));
}

void TestBlurHash::encodeImage()
{
    auto image = QImage(QSize(360, 200), QImage::Format_RGB888);
    image.fill(Qt::black);

    const auto encodedString = Quotient::BlurHash::encode(image, 4, 3);
    QCOMPARE(encodedString.size(), 28);
    QCOMPARE(encodedString, QStringLiteral("L00000fQfQfQfQfQfQfQfQfQfQfQ"));
}

QTEST_GUILESS_MAIN(TestBlurHash)
#include "testblurhash.moc"
