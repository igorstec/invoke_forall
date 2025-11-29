#include "../invoke_forall.h"
#include "tuple"

struct callable_1 {
    constexpr int operator()(int n) const {
        return n + 2;
    }
};

struct callable_2 {
    constexpr int operator()(int n) const {
        return n + 156;
    }
};

constexpr int callable_3(int n) {
    return n * 5;
}

void test_3() {

    constexpr auto t1 = invoke_forall(
        std::tuple{callable_1{}, callable_2{}, &callable_3, [](int n) {
            return -n;
        }},
        std::tuple{10, 20, 112, 2}
    );

    static_assert(get<0>(t1) == 12);
    static_assert(get<1>(t1) == 176);
    static_assert(get<2>(t1) == 560);
    static_assert(get<3>(t1) == -2);
}
