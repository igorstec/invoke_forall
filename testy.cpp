// Autor testów: Olaf Targowski
// Wersja 1.0.
#include "invoke_forall.h"
#include <array>
#include <functional>
#include <iostream>
#include <ranges>
#include <string>
#include <tuple>
#include <cassert>

static size_t ile_kopii=0;
struct ciezki_obiekt{
    bool czy_wyjete=0;
    ciezki_obiekt() = default;
    ciezki_obiekt(ciezki_obiekt&& o) {
        assert(!o.czy_wyjete);
        o.czy_wyjete=1;
    }
    ciezki_obiekt(const ciezki_obiekt& o) {
        assert(!o.czy_wyjete);
        ++ile_kopii;
        czy_wyjete=0;
    }
    ciezki_obiekt& operator=(ciezki_obiekt&&) = default;
    ciezki_obiekt& operator=(const ciezki_obiekt&) = default;
};
ciezki_obiekt przesuwacz(ciezki_obiekt&& o1){
    return std::move(o1);
}
ciezki_obiekt& dawacz_referencji(ciezki_obiekt& o1){
    return o1;
}
const ciezki_obiekt& dawacz_stalej_referencji(const ciezki_obiekt& o1){
    return o1;
}
ciezki_obiekt dawacz_z_kopii(ciezki_obiekt o1){
    return o1;
}
void sprawdz(std::ranges::range auto r){
    for (auto &&o : r)
        assert(!o.czy_wyjete);
}
void sprawdz(ciezki_obiekt &&o){
    assert(!o.czy_wyjete);
}
void sprawdz(const ciezki_obiekt &o){
    assert(!o.czy_wyjete);
}
//
// // 2. Specyficzne - tuple
// template <typename... Ts>
// void sprawdz(const std::tuple<Ts...>& t) {
//     std::apply([](const auto&... elems) {
//         (sprawdz(elems), ...);
//     }, t);
// }
//
// // 3. Specyficzne - array
// template <typename T, std::size_t N>
// void sprawdz(const std::array<T, N>& arr) {
//     for (const auto& elem : arr) {
//         sprawdz(elem);
//     }
// }
//
// // 4. Ogólne - inne ranges (wykluczając tuple i array)
// template <std::ranges::range R>
//     requires (!std::is_same_v<std::remove_cvref_t<R>, std::tuple<typename std::tuple_element<0, std::remove_cvref_t<R>>::type>> &&
//               !requires { typename std::tuple_size<std::remove_cvref_t<R>>::type; })
// void sprawdz(R&& r){
//     for (auto &&o : r)
//         assert(!o.czy_wyjete);
// }



int main() {
    using std::make_tuple;
    // Shut up warnings.
    //using std::move;
#define move std::move
    using std::invoke;
    ciezki_obiekt a,b;
    auto spr=[&](size_t oczekiwana_liczba_kopii){
        assert(ile_kopii==oczekiwana_liczba_kopii); ile_kopii=0;
        a=ciezki_obiekt();
        b=ciezki_obiekt();
    };
    spr(0);
    
    sprawdz(invoke(przesuwacz, move(a)));
    spr(0);
    sprawdz(invoke_forall(przesuwacz, move(a)));
    spr(0);
    sprawdz(invoke_forall(przesuwacz, protect_arg(move(a))));
    spr(0);

//    sprawdz(invoke_forall(make_tuple(przesuwacz, przesuwacz), make_tuple(move(a), move(b))));
//    spr(0);
//    sprawdz(invoke_forall(przesuwacz, make_tuple(move(a), move(b))));
//    spr(0);
//    sprawdz(invoke_forall(make_tuple(przesuwacz, przesuwacz), move(a)));
//    // Tak.
//    spr(1);
//
//    sprawdz(invoke_forall(make_tuple(przesuwacz, przesuwacz), protect_arg(move(a))));
//    // Tak.
//    spr(1);
//
//    sprawdz(invoke_forall(dawacz_z_kopii, std::tie(a, a)));
//    spr(2);
//    auto idk=[&](auto &&fun){
//        sprawdz(invoke_forall(fun, a));
//        spr(0);
//        sprawdz(invoke_forall(fun, protect_arg(a)));
//        spr(0);
//        // NOTE: ta część mi (autorowi) jeszcze nie przechodzi...
//        // SZKOpuł leży w przypadku, gdy mamy zwrócić random_access_range
//        // lvalue referencji.
//        sprawdz(invoke_forall(fun, std::tie(a, a)));
//        spr(0);
//        sprawdz(invoke_forall(make_tuple(fun, fun), a));
//        spr(0);
//        sprawdz(invoke_forall(make_tuple(fun, fun), protect_arg(a)));
//        spr(0);
//    };
//    idk(dawacz_referencji);
//    idk(dawacz_stalej_referencji);
}
