// SPDX-FileCopyrightText: 2021 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include <Quotient/omittable.h>
#include <Quotient/util.h>

#include <QtTest/QtTest>

// compile-time Omittable<> tests
using namespace Quotient;

Omittable<int> testFn(bool) { return 0; }
bool testFn2(int) { return false; }
static_assert(
    std::is_same_v<decltype(std::declval<Omittable<bool>>().then(testFn)),
                   Omittable<int>>);
static_assert(
    std::is_same_v<
        decltype(std::declval<Omittable<bool>>().then_or(testFn, 0)), int>);
static_assert(
    std::is_same_v<decltype(std::declval<Omittable<bool>>().then(testFn)),
                   Omittable<int>>);
static_assert(std::is_same_v<decltype(std::declval<Omittable<int>>()
                                          .then(testFn2)
                                          .then(testFn)),
                             Omittable<int>>);
static_assert(std::is_same_v<decltype(std::declval<Omittable<bool>>()
                                          .then(testFn)
                                          .then_or(testFn2, false)),
                             bool>);

constexpr auto visitTestFn(int, bool) { return false; }
static_assert(
    std::is_same_v<Omittable<bool>, decltype(lift(testFn2, Omittable<int>()))>);
static_assert(std::is_same_v<Omittable<bool>,
                             decltype(lift(visitTestFn, Omittable<int>(),
                                           Omittable<bool>()))>);

class TestUtils : public QObject {
    Q_OBJECT
private Q_SLOTS:
    void testLinkifyUrl();

private:
    void testLinkified(QString original, const QString& expected,
                       const int sourceLine) const
    {
        linkifyUrls(original);
        QTest::qCompare(original, expected, "", "", __FILE__, sourceLine);
    }
};

#ifndef Q_MOC_RUN // moc chokes on the code below, doesn't really need it
void TestUtils::testLinkifyUrl()
{
    // Pending rewrite once std::source_location() works everywhere
#    define T(Original, Expected) testLinkified(Original, Expected, __LINE__)

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

#    undef T
}
#endif

QTEST_APPLESS_MAIN(TestUtils)
#include "utiltests.moc"
