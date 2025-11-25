#ifndef INVOKE_FORALL_H
#define INVOKE_FORALL_H

#include <cstddef>
#include <tuple>
#include <functional>
#include <type_traits>
#include <utility>

namespace detail {
    // ============================================================================
    // Pomocnicze concepty i type traity
    // ============================================================================

    template <typename T>
    struct bare_type {
        using type = std::remove_cvref_t<T>;
    };

    template <typename T>
    using bare_type_t = typename bare_type<T>::type;

    template<typename... Args>
    concept notEmpty = (sizeof...(Args) > 0);

    // template<class T, class = void>
    // struct isTupleLike : std::false_type {};
    //
    // // T nie wspiera operacji tuple_size_v.
    // template<class T>
    //     requires requires { typename std::tuple_size<T>; }
    // struct isTupleLike<T, std::void_t<decltype(std::tuple_size_v<T>)>> : std::true_type {};

    template<typename T>
        concept isTupleLikeConcept = requires {
        typename std::tuple_size<T>;
         { std::tuple_size_v<T> } -> std::convertible_to<std::size_t>;
    };
    template<typename T>
        concept isGettableConcept = requires {
        isTupleLikeConcept<T>;
        std::get<0>(std::declval<T>());
    };




    template<typename T>
    concept Protected = requires (T t){
        t.get_protected_value();
    };


    // =============================================================================
    // Sprawdzenie Gettable i Arity
    // =============================================================================

    template<class T, std::size_t Index, class = void>
    struct isIndexGettable : std::false_type {};

    // T nie wspiera operacji get<Index>.
    template<class T, std::size_t Index>
    struct isIndexGettable<T, Index, std::void_t<decltype(std::get<Index>(std::declval<T>()))>> : std::true_type {};

    template<typename TupleType, std::size_t... Indices>
        constexpr bool allIndexesGettable(std::index_sequence<Indices...>) {
        return (... && isIndexGettable<TupleType, Indices>::value);
    }



    /*
    *namespace std {
  template <class T, T v>
  struct integral_constant {
    static constexpr T value = v;
    using value_type = T;
    using type = integral_constant<T, v>;
    constexpr operator value_type() const noexcept { return value; }
    constexpr value_type operator()() const noexcept { return value; }
  };
}
     *
     */
    // template<typename T>
    // struct isGettable {
    //     static constexpr bool value = []() {
    //         using Bare = bare_type_t<T>;
    //
    //         if constexpr (!isTupleLikeConcept<Bare>)
    //             return false;
    //         else {
    //             constexpr std::size_t arity = std::tuple_size_v<Bare>;
    //             return allIndexesGettable<Bare>(std::make_index_sequence<arity>{});
    //         }
    //     }();
    // };


    // template<typename T>
    // inline constexpr bool is_gettable_v = isGettable<T>::value;
    //
    //
    // // Weż arity ze wszystkich gettable argumentów - muszą mieć tę samą arity
    // template <typename T>
    // constexpr std::size_t get_gettable_arity() {
    //     if constexpr (isGettable<T>::value) {
    //         return std::tuple_size_v<T>;
    //     }
    //     return 0;
    // }
    //
    template <typename... Args>
    constexpr std::size_t compute_arity() {
        std::size_t result = 3;
        //((result = result != 0 ? result : get_gettable_arity<Args>()), ...);
        return result;
    }

    // ==============================================================================
    // template <typename T> constexpr typ protect_arg(T&&) wykonuje konstruktor na tym
    // ==============================================================================

    // Opakowuje argument, aby zapobiec jego rozbiciu przez std::invoke.
    // Używane do przekazywania argumentów krotkowych jako pojedynczych argumentów.
    template <typename T>
    class ProtectedArg {
    private:
        T&& value_;
    public:
        explicit ProtectedArg(T&& value) : value_(std::forward<T>(value)) {}

        T&& get_protected_value() const {
            return (std::forward<T>(value_));
        }

    };

    // =============================================================================
    // Typy odpowiedzi i funckcje pomocnicze do ich okreslenia
    // =============================================================================

    template <std::size_t I, typename T>
    // /* zwraca dla nongettable pp typ arg a
    //  dla gettable pp std::get<I>(arg)
    //  a dla protected get_protected value*/
    constexpr decltype(auto) extract_for_invoke(T && arg) {
        if constexpr (isGettableConcept<T>) {
            // Gettable argument: extract element at index I
            return std::get<I>(std::forward<T>(arg));
        }
        if constexpr (Protected<T>) {
            // Protected argument: use the wrapped value directly, not indexed
            return arg.get_protected_value();
        } else {
            // Regular argument: use as-is
            return std::forward<T>(arg);
        }

    }


    template <typename Func, std::size_t I, typename... Args>
    using invoke_result_at_i = std::invoke_result_t<
        Func,
        decltype(extract_for_invoke<I>(std::declval<Args>()))...
    >;

    template < std::size_t Arity, typename... Args>
    constexpr bool all_results_same_type() {
        // if constexpr (Arity == 0) {
        //     return true;
        // }
        // bool same = true;
        // //TODO
        //using first_result = invoke_result_at_i< 0, Args...>;
        return false;
    }

    template <std::size_t Arity, typename Func, typename... Args>
    constexpr auto make_tuple_of_results(Func&& func, Args&&... args) {
        return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            return std::tuple{
                std::invoke(std::forward<Func>(func),
                           extract_for_invoke<Is>(std::forward<Args>(args))...)...
            };
        }(std::make_index_sequence<Arity>{});
    }


    // func: zrob tuple odpowiedz
    // func: zrob array odpowiedz
    // co jak wszytskie to void??? nwm


} // namespace detail

// ============================================================================
// To co wywoluje example
template <typename T>
constexpr auto protect_arg(T&& arg) {
    return detail::ProtectedArg<T>(std::forward<T>(arg));
}
// ============================================================================

template<detail::notEmpty... Args>
constexpr decltype(auto) invoke_forall(Args&&... args) {
    if constexpr ((... || detail::isGettableConcept<detail::bare_type_t<Args>>)) {

        //constexpr size_t arity = 5;  // tymczasowo na sztywno
        // Tutaj implementacja dla przypadku, gdy przynajmniej jeden argument jest wyciągalny
        constexpr std::size_t arity = detail::compute_arity<Args...>(); // 3

        if constexpr (arity==0){
            // Brak wyciągalnych argumentów
            // To chyba nie moze sie stać:
            return std::invoke(std::forward<Args>(args)...);
        }
        if constexpr (detail::all_results_same_type<arity, Args...>()) {
            // Wszystkie wyniki mają ten sam typ wiec bedzie Array od arrity oraz first_result
            return std::invoke(std::forward<Args>(args)...);
        } else {
            // hard 3 arity
            return detail::build_tuple_result<arity>(std::forward<Args>(args)...);
            // Wyniki mają różne typy wiec bedzie Tuple od arrity
        }
    } else {
        // Przypadek, gdy żaden argument nie jest wyciągalny
        //return std::invoke(std::forward<Args>(args)...);
        return 888;
    }
}

#endif
