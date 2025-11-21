#ifndef INVOKE_FORALL_H
#define INVOKE_FORALL_H

#include <cstddef>
#include <tuple>
#include <functional>
#include <type_traits>
#include <utility>

namespace detail {
    template<typename... Args>
    concept notEmpty = (sizeof...(Args) > 0);

    template<class T, class = void>
    struct isTupleLike : std::false_type {};

    // T nie wspiera operacji tuple_size_v.
    template<class T>
    struct isTupleLike<T, std::void_t<decltype(std::tuple_size_v<T>)>> : std::true_type {};

    template<class T, std::size_t Index, class = void>
    struct isIndexGettable : std::false_type {};

    // T nie wspiera operacji get<Index>.
    template<class T, std::size_t Index>
    struct isIndexGettable<T, Index, std::void_t<decltype(std::get<Index>(std::declval<T>()))>> : std::true_type {};


    template<typename T, T... ints>
    /*
     *  Jeśli wszyskie indexy są gettable, zwraca true,
     *  w przeciwnym wypadku zwraca false.
     */
    constexpr bool allIndexesGettable(std::integer_sequence<T, ints...> indexes) {
        return (... && isIndexGettable<T, ints>::value);
    }

    template<typename T>
    constexpr bool isGettable(T t) {
        if (!isTupleLike<T>::value)
            return false;

        static constexpr std::size_t arity = std::tuple_size_v<T>;
        auto indexes = std::make_index_sequence<arity>();
        return allIndexesGettable(indexes);
    }
}

template<typename... Args> requires detail::notEmpty<Args...>
constexpr auto invoke_forall(Args&&... args) {
    // Wystarczy, żeby chociaż jeden był wyciągalny
    // constexpr bool allArgsGettable = (... && detail::isGettable(args));

    return std::invoke(args...);
}

#endif
