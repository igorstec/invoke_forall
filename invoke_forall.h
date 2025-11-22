#ifndef INVOKE_FORALL_H
#define INVOKE_FORALL_H

#include <cstddef>
#include <tuple>
#include <functional>
#include <type_traits>
#include <utility>

namespace detail {

    template <typename T>
    struct bare_type {
        using type = std::remove_cvref_t<T>;
    };

    template <typename T>
    using bare_type_t = typename bare_type<T>::type;

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
    constexpr bool isGettable(T) {
        if (!isTupleLike<bare_type_t<T>>::value)
            return false;

        static constexpr std::size_t arity = std::tuple_size_v<T>;
        auto indexes = std::make_index_sequence<arity>();
        return allIndexesGettable(indexes);
    }

    // Sprawdzanie arity i gettability

    // Weż arity ze wszystkich gettable argumentów - muszą mieć tę samą arity
    template <typename T>
    constexpr std::size_t get_gettable_arity() {
        if constexpr (isGettable<T>) {
            return std::tuple_size_v<bare_type_t<T>>;
        }
        return 0;
    }

    template <typename... Args>
    constexpr std::size_t compute_arity() {
        std::size_t result = 0;
        ((result = get_gettable_arity<Args>() != 0 ? get_gettable_arity<Args>() : result), ...);
        return result;
    }

} // namespace detail

template<detail::notEmpty... Args>
constexpr auto invoke_forall(Args&&... args) {
    // Wystarczy, żeby chociaż jeden był wyciągalny
    // constexpr bool allArgsGettable = (... && detail::isGettable(args));
    //constexpr bool hasGettableArgs = (... || detail::isGettable(args));
    if constexpr ((... || detail::isGettable(args))) {
        // Tutaj implementacja dla przypadku, gdy przynajmniej jeden argument jest wyciągalny
        // Na razie zostawiamy puste



    } else {
        // Przypadek, gdy żaden argument nie jest wyciągalny
        return std::invoke(std::forward<Args>(args)...);
    }
}

#endif
