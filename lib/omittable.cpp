// SPDX-FileCopyrightText: 2021 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "omittable.h"

// Omittable<> tests
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
