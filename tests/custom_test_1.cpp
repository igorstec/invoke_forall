#include "../invoke_forall.h"
#include <array>
#include <functional>
#include <string>
#include <tuple>
#include <cassert>

static constexpr int sum1(const std::array<int, 3> &t) {
    int res = 0;
    for (int x : t)
        res += x;
    return res;
}

static int sum2(std::array<int, 3> &t, int d) {
    int res = 0;
    for (int &x : t) {
        x -= d;
        res += x;
    }
    return res;
}

void test_1() {
    // no tuple-like arguments: simple invoke
    constexpr auto res1 = invoke_forall(std::plus<int>{}, 2, 3);
    static_assert(res1 == 5);

    // tuple-like arguments: two arrays of same arity
    constexpr std::array<int, 3> A = {1, 2, 3};
    constexpr std::array<int, 3> B = {10, 20, 30};
    constexpr auto res2 = invoke_forall(std::plus<int>{}, A, B);
    static_assert(res2.at(0) == 11);
    static_assert(res2.at(1) == 22);
    static_assert(res2.at(2) == 33);

    // mixed arguments: call f(scalar, tuple)
    constexpr std::array<int, 3> C = {4, 5, 6};
    constexpr auto res3 = invoke_forall([](int s, int t){ return s * t; }, 10, C);
    static_assert(res3.at(0) == 40);
    static_assert(res3.at(1) == 50);
    static_assert(res3.at(2) == 60);

    // tuple-like heterogeneous arguments
    static constexpr std::tuple<int, double, std::string> T1 = {1, 2.5, std::string("abc")};
    static constexpr auto res4 = invoke_forall([](auto a){ return a; }, T1);
    // It writes out "res4 = 1 2.5 abc\n".
    static_assert(get<0>(res4) == 1);
    static_assert(get<1>(res4) == 2.5);
    static_assert(get<2>(res4) == std::string("abc"));

    // It writes out "sum1 = 6\n".
    static_assert(invoke_forall(sum1, protect_arg(std::array{1, 2, 3})) == 6);

    // code evaluated at compile time
    static_assert(91 == invoke_forall(std::multiplies<int>{}, 7, 13));
}

void test_2() {
    std::array<int, 3> A = {1, 2, 3};
    std::array<int, 3> B = {4, 5, 6};
    std::array<int, 3> C = {12, 6, -3};
    int c = 0;
    for (int x : invoke_forall(sum2, protect_arg(B), A))
        assert(x == C.at(c++));

    assert(invoke_forall(sum1, protect_arg(B)) == -3);

    // void return type test
    // auto z = std::get<0>(invoke_forall([](int &x){ x += 5; }, A));
    // The behavior of the above line is undefined – it may not compile.
    std::array<int, 3> D = {6, 7, 8};
    int d = 0;
    invoke_forall([](int &x) { x += 5; }, A);
    // It writes out "A = 6 7 8\n".
    for (int x : A)
        assert(x == D.at(d++));
    assert(A == D);
}
