// SPDX-FileCopyrightText: 2021 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include <Quotient/util.h>

#include <QtCore/QStringBuilder>
#include <QtTest/QtTest>

using namespace Quotient;
using std::optional, std::pair;

template <typename T>
consteval auto testMerge(T lhs, auto rhs, bool expectedResult,
                         const std::type_identity_t<T>& expectedLhs)
{
    auto result = merge(lhs, rhs);
    return result == expectedResult && lhs == expectedLhs;
}
static_assert(testMerge(1, optional{ 2 }, true, 2));
static_assert(testMerge(1, optional<int>{}, false, 1));
static_assert(testMerge(optional{ 1 }, optional{ 2 }, true, { 2 }));
static_assert(testMerge(optional{ 1 }, optional<int>{}, false, { 1 }));
static_assert(testMerge(optional<int>{}, optional{ 1 }, true, { 1 }));

class TestUtils : public QObject {
    Q_OBJECT
private Q_SLOTS:
    void testLinkifyUrl();
    void testIsGuestUserId();
    void testQuoCStr();

private:
    void testLinkified(QString original, const QString& expected, const int sourceLine) const
    {
        linkifyUrls(original);
        QTest::qCompare(original, expected, "", "", __FILE__, sourceLine);
    }
};

void TestUtils::testLinkifyUrl()
{
    // Pending rewrite once std::source_location() works everywhere
#define T(Original, Expected) testLinkified(Original, Expected, __LINE__)

    T("https://www.matrix.org"_ls,
      "<a href='https://www.matrix.org'>https://www.matrix.org</a>"_ls);
//    T("www.matrix.org"_ls, // Doesn't work yet
//      "<a href='https://www.matrix.org'>www.matrix.org</a>"_ls);
    T("smb://some/file"_ls, "smb://some/file"_ls); // Disallowed scheme
    T("https:/something"_ls, "https:/something"_ls); // Malformed URL
    T("https://matrix.to/#/!roomid:example.org"_ls,
      "<a href='https://matrix.to/#/!roomid:example.org'>https://matrix.to/#/!roomid:example.org</a>"_ls);
    T("https://matrix.to/#/@user_id:example.org"_ls,
      "<a href='https://matrix.to/#/@user_id:example.org'>https://matrix.to/#/@user_id:example.org</a>"_ls);
    T("https://matrix.to/#/#roomalias:example.org"_ls,
      "<a href='https://matrix.to/#/#roomalias:example.org'>https://matrix.to/#/#roomalias:example.org</a>"_ls);
    T("https://matrix.to/#/##ircroomalias:example.org"_ls,
      "<a href='https://matrix.to/#/##ircroomalias:example.org'>https://matrix.to/#/##ircroomalias:example.org</a>"_ls);
    T("me@example.org"_ls,
      "<a href='mailto:me@example.org'>me@example.org</a>"_ls);
    T("mailto:me@example.org"_ls,
      "<a href='mailto:me@example.org'>mailto:me@example.org</a>"_ls);
    T("www.example.com?email@example.com"_ls,
      "<a href='www.example.com?email@example.com'>www.example.com?email@example.com</a>"_ls);
    T("!room_id:example.org"_ls,
      "<a href='https://matrix.to/#/!room_id:example.org'>!room_id:example.org</a>"_ls);
    T("@user_id:example.org"_ls,
      "<a href='https://matrix.to/#/@user_id:example.org'>@user_id:example.org</a>"_ls);
    T("#room_alias:example.org"_ls,
      "<a href='https://matrix.to/#/#room_alias:example.org'>#room_alias:example.org</a>"_ls);

#undef T
}

void TestUtils::testIsGuestUserId()
{
    QVERIFY(isGuestUserId("@123:example.org"_ls));
    QVERIFY(!isGuestUserId("@normal:example.org"_ls));
}

void TestUtils::testQuoCStr()
{
    using namespace std::string_view_literals; // strncmp() is clumsy
    QVERIFY("Test const char[]"sv == QUO_CSTR("Test const char[]"));
#define T(Wrapper_) QVERIFY("Test " #Wrapper_##sv == QUO_CSTR(Wrapper_("Test " #Wrapper_)))
    T(std::string);
    T(QStringLiteral);
    T(QByteArrayLiteral);
    T(QLatin1StringView); // clazy:exclude=qt6-qlatin1stringchar-to-u
    T(QUtf8StringView);
#undef T
    QVERIFY("Test QStringBuilder<QString>"sv
            == QUO_CSTR(QStringLiteral("Test ") % u"QStringBuilder<QString>"));
    QVERIFY("Test QStringBuilder<QByteArray>"sv
            == QUO_CSTR(QByteArrayLiteral("Test ") % "QStringBuilder<QByteArray>"));
}

QTEST_APPLESS_MAIN(TestUtils)
#include "utiltests.moc"
