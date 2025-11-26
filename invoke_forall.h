#ifndef INVOKE_FORALL_H
#define INVOKE_FORALL_H

#include <array>
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
    concept not_empty = (sizeof...(Args) > 0);

    template<typename T>
    concept isTupleLikeConcept =
     requires {
        // musi istnieć specjalizacja std::tuple_size dla bare_type_t<T>
        typename std::tuple_size<bare_type_t<T>>;
        // a jej składowa value musi być używalna jak size_t
        { std::tuple_size<bare_type_t<T>>::value } -> std::convertible_to<std::size_t>;
     };
    // template<typename T>
    // constexpr bool is_tuple_like_v = is_tuple_like<bare_type_t<T>>::value;

    template<typename T>
    concept Protected = requires (T t){
        t.get_protected_value();
    };

    // =============================================================================
    // Sprawdzenie Gettable i Arity
    // =============================================================================

    template<class T, std::size_t Index>
    concept is_index_gettable = requires { std::get<Index>(std::declval<T>()); };

    template<typename T>
    constexpr bool isGettable() {

        using U = bare_type_t<T>;

        if constexpr (!isTupleLikeConcept<U>) {
            return false;
        } else {
            /*
            *  Jeśli wszyskie indexy są gettable, zwraca true,
            *  w przeciwnym wypadku zwraca false.
            */
            constexpr auto N = std::tuple_size<U>::value;
            return [] <std::size_t... Indexes>(std::integer_sequence<std::size_t, Indexes...>) constexpr -> bool {
                return (... && is_index_gettable<U, Indexes>);
            }(std::make_index_sequence<N>());
        }
    };



    // Weż arity ze wszystkich gettable argumentów - muszą mieć tę samą arity
    template <typename T>
    constexpr std::size_t get_gettable_arity() {
        if constexpr (isGettable<T>()) {
            return std::tuple_size_v<bare_type_t<T>>;
        }else {
            return 0;
        }
    }

    template <typename... Args>
    constexpr std::size_t compute_arity() {
        std::size_t result = 0;
        ((result = result != 0 ? result : get_gettable_arity<Args>()), ...);
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
        if constexpr (isGettable<T>()) {
            // Gettable argument: extract element at index I
            return std::get<I>(std::forward<T>(arg));
        }else if constexpr (Protected<T>) {
            // Protected argument: use the wrapped value directly, not indexed
            return arg.get_protected_value();
        } else {
            // Regular argument: use as-is
            return std::forward<T>(arg);
        }
    }

    template <std::size_t Arity, typename... Args>
    constexpr auto make_tuple_of_results(Args&&... args) {
        return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            auto call_one = [&](auto ic) {
                constexpr std::size_t I = decltype(ic)::value;
                return std::invoke(
                    extract_for_invoke<I>(std::forward<Args>(args))...
                );
            };
            return std::make_tuple(
                call_one(std::integral_constant<std::size_t, Is>{})...
            );
        }(std::make_index_sequence<Arity>{});
    }

    // template <std::size_t I, typename... Args>
    // using invoke_result_at_i = std::invoke_result_t<
    //     decltype(extract_for_invoke<I>(std::declval<Args>()))...
    // >;



    // template <std::size_t Arity, typename... Args>
    // constexpr bool all_results_same_type() {
    //     if constexpr (Arity == 0) {
    //         return true;
    //     }else {
    //         using first_result = invoke_result_at_i<0, Args...>;
    //
    //         // Compares all with first_result.
    //         return [] <std::size_t... Indexes>(std::integer_sequence<std::size_t, Indexes...>) constexpr -> bool {
    //             return (... && std::is_same_v<first_result, invoke_result_at_i<Indexes, Args...>>);
    //         }(std::make_index_sequence<Arity>());
    //     }
    // }

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

template<detail::not_empty... Args>
constexpr decltype(auto) invoke_forall(Args&&... args) {
    if constexpr ((... || detail::isGettable<Args>())) {

        // Tutaj implementacja dla przypadku, gdy przynajmniej jeden argument jest wyciągalny
        constexpr std::size_t arity = detail::compute_arity<Args...>();

        static_assert(arity > 0);
         // if constexpr (arity==0){
         //     // Brak wyciągalnych argumentów
         //     // To chyba nie moze sie stać:
         //     return std::invoke(std::forward<Args>(args)...);
         // }


         // if constexpr (detail::all_results_same_type< arity, Args...>()) {
         //     // Wszystkie wyniki mają ten sam typ wiec bedzie Array od arrity oraz first_result
         // }
         // else {
         //     // Wyniki mają różne typy wiec bedzie Tuple od arrity
         // }
       return detail::make_tuple_of_results<arity>(std::forward<Args>(args)...);
    } else {
        // Przypadek, gdy żaden argument nie jest wyciągalny
        return std::invoke(std::forward<Args>(args)...);
    }
}

#endif
