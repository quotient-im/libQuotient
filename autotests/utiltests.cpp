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
    QString linkified(QString toLinkify) const
    {
        linkifyUrls(toLinkify);
        return toLinkify;
    }
};

#ifndef Q_MOC_RUN // moc chokes on the code below, doesn't really need it
void TestUtils::testLinkifyUrl()
{
    QCOMPARE(
        linkified("mailto:example@example.com"_ls),
        "<a href='mailto:example@example.com'>mailto:example@example.com</a>"_ls);
    QCOMPARE(
        linkified("example@example.com"_ls),
        "<a href='mailto:example@example.com'>example@example.com</a>"_ls);
    QCOMPARE(linkified("https://example.com"_ls),
             "<a href='https://example.com'>https://example.com</a>"_ls);
    QCOMPARE(linkified("www.example.com"_ls),
             "<a href='www.example.com'>www.example.com</a>"_ls);
    QCOMPARE(linkified("www.example.com?email@example.com"_ls),
             "<a href='www.example.com?email@example.com'>"
             "www.example.com?email@example.com</a>"_ls);
}
#endif

QTEST_APPLESS_MAIN(TestUtils)
#include "utiltests.moc"
