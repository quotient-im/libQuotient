// SPDX-FileCopyrightText: 2018 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "function_traits.h"

// Tests for function_traits<>

using namespace Quotient;

int f_();
static_assert(std::is_same<fn_return_t<decltype(f_)>, int>::value,
              "Test fn_return_t<>");

void f1_(int, float);
static_assert(std::is_same<fn_arg_t<decltype(f1_), 1>, float>::value,
              "Test fn_arg_t<>");

struct Fo {
    int operator()();
    static constexpr auto l = [] { return 0.0f; };
    bool memFn();
    void constMemFn() const&;
    double field;
    const double field2;
};
static_assert(std::is_same_v<fn_return_t<Fo>, int>,
              "Test return type of function object");
static_assert(std::is_same_v<fn_return_t<decltype(Fo::l)>, float>,
              "Test return type of lambda");
static_assert(std::is_same_v<fn_arg_t<decltype(&Fo::memFn)>, Fo>,
              "Test first argument type of member function");
static_assert(std::is_same_v<fn_return_t<decltype(&Fo::memFn)>, bool>,
              "Test return type of member function");
static_assert(std::is_same_v<fn_arg_t<decltype(&Fo::constMemFn)>, const Fo&>,
              "Test first argument type of const member function");
static_assert(std::is_void_v<fn_return_t<decltype(&Fo::constMemFn)>>,
              "Test return type of const member function");
static_assert(std::is_same_v<fn_return_t<decltype(&Fo::field)>, double&>,
              "Test return type of a class member");
static_assert(std::is_same_v<fn_return_t<decltype(&Fo::field2)>, const double&>,
              "Test return type of a const class member");

struct Fo1 {
    void operator()(int);
};
static_assert(std::is_same<fn_arg_t<Fo1>, int>(),
              "Test fn_arg_t defaulting to first argument");

template <typename T>
static void ft(const std::vector<T>&);
static_assert(
    std::is_same<fn_arg_t<decltype(ft<double>)>, const std::vector<double>&>(),
    "Test function templates");
