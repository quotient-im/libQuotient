// SPDX-FileCopyrightText: 2024 Joshua Goins <josh@redstrate.com>
// SPDX-License-Identifier: MIT

#include <blurhash.h>

#include <QTest>

class TestBlurHash : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void encode83_data();
    void encode83();

    void decode83_data();
    void decode83();

    void unpackComponents();
    void packComponents();

    void decodeMaxAC();
    void encodeMaxAC();

    void decodeAverageColor_data();
    void decodeAverageColor();

    void decodeAC();
    void encodeAC();

    void decodeImage();
    void encodeImage();
};

void TestBlurHash::encode83_data()
{
    QTest::addColumn<int>("value");
    QTest::addColumn<QString>("expected");

    QTest::addRow("encoding 1") << 0 << "0";
    QTest::addRow("encoding 2") << 21 << "L";
    QTest::addRow("encoding 3") << 30 << "U";
    QTest::addRow("encoding 4") << 34 << "Y";
    QTest::addRow("encoding 5") << 1 << "1";
}

void TestBlurHash::encode83()
{
    QFETCH(int, value);
    QFETCH(QString, expected);

    QCOMPARE(Quotient::BlurHash::encode83(value), expected);
}

void TestBlurHash::decode83_data()
{
    QTest::addColumn<QString>("value");
    QTest::addColumn<std::optional<int>>("expected");

    // invalid base83 characters
    QTest::addRow("decoding 1") << "試し" << std::optional<int>(std::nullopt);
    QTest::addRow("decoding 2") << "(" << std::optional<int>(std::nullopt);

    QTest::addRow("decoding 3") << "0" << std::optional(0);
    QTest::addRow("decoding 4") << "L" << std::optional(21);
    QTest::addRow("decoding 5") << "U" << std::optional(30);
    QTest::addRow("decoding 6") << "Y" << std::optional(34);
    QTest::addRow("decoding 7") << "1" << std::optional(1);
}

void TestBlurHash::decode83()
{
    QFETCH(QString, value);
    QFETCH(std::optional<int>, expected);

    QCOMPARE(Quotient::BlurHash::decode83(value), expected);
}

void TestBlurHash::unpackComponents()
{
    QCOMPARE(Quotient::BlurHash::unpackComponents(50), Quotient::BlurHash::Components(6, 6));
}

void TestBlurHash::packComponents()
{
    QCOMPARE(Quotient::BlurHash::packComponents(Quotient::BlurHash::Components(6, 6)), 50);
}

void TestBlurHash::decodeMaxAC()
{
    QCOMPARE(Quotient::BlurHash::decodeMaxAC(50), 0.307229f);
}

void TestBlurHash::encodeMaxAC()
{
    QCOMPARE(Quotient::BlurHash::encodeMaxAC(0.307229f), 50);
}

void TestBlurHash::decodeAverageColor_data()
{
    QTest::addColumn<int>("value");
    QTest::addColumn<QColor>("expected");

    QTest::addRow("decoding 1") << 12688010 << QColor(0xffc19a8a);
    QTest::addRow("decoding 2") << 9934485 << QColor(0xff979695);
    QTest::addRow("decoding 3") << 8617624 << QColor(0xff837e98);
    QTest::addRow("decoding 4") << 14604757 << QColor(0xffded9d5);
    QTest::addRow("decoding 5") << 13742755 << QColor(0xffd1b2a3);
}

void TestBlurHash::decodeAverageColor()
{
    QFETCH(int, value);
    QFETCH(QColor, expected);

    QCOMPARE(Quotient::BlurHash::decodeAverageColor(value), expected);
}

void TestBlurHash::decodeAC()
{
    constexpr auto maxAC = 0.289157f;
    QCOMPARE(Quotient::BlurHash::decodeAC(0, maxAC), QColor::fromRgbF(-0.289063f, -0.289063f, -0.289063f));
}

void TestBlurHash::encodeAC()
{
    constexpr auto maxAC = 0.289157f;
    QCOMPARE(Quotient::BlurHash::encodeAC(QColor::fromRgbF(-0.289063f, -0.289063f, -0.289063f), maxAC), 0);
}

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