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

    // T nie wspiera operacji tuple_size_v.
    template<typename T, typename = void>
    struct is_tuple_like : std::false_type {};

    template<typename T>
    struct is_tuple_like<T, std::void_t<decltype(std::tuple_size<T>::value)>> : std::true_type {};

    template<typename T>
    constexpr bool is_tuple_like_v = is_tuple_like<T>::value;

    template<typename T>
    concept Protected = requires (T t){
        t.get_protected_value();
    };

    // =============================================================================
    // Sprawdzenie Gettable i Arity
    // =============================================================================

    template<class T, std::size_t Index>
    constexpr bool is_index_gettable = requires { std::get<Index>(std::declval<T>()); };

    template<typename T>
    constexpr bool is_gettable() {
        using Bare = bare_type_t<T>; // T bez cv i referencji
        if constexpr (is_tuple_like_v<Bare>) {
            /*
            *  Jeśli wszyskie indexy są gettable, zwraca true,
            *  w przeciwnym wypadku zwraca false.
            */
            constexpr std::size_t N = std::tuple_size_v<Bare>;
            return [] <std::size_t... Indexes>(std::integer_sequence<std::size_t, Indexes...>) constexpr -> bool {
                return (... && is_index_gettable<Bare, Indexes>);
            }(std::make_index_sequence<N>());
        }

        return false;
    };

    // Weż arity ze wszystkich gettable argumentów - muszą mieć tę samą arity
    template <typename T>
    constexpr std::size_t get_gettable_arity() {
        if constexpr (is_gettable<T>()) {
            return std::tuple_size_v<bare_type_t<T>>;
        }
        return 0;
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
    /* zwraca dla nongettable pp typ arg a
     dla gettable pp std::get<I>(arg)
     a dla protected get_protected value*/
    constexpr decltype(auto) extract_for_invoke(T && arg) {
        if constexpr (is_gettable<T>()) {
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

    template <std::size_t I, typename... Args>
    using invoke_result_at_i = std::invoke_result_t<
        decltype(extract_for_invoke<I>(std::declval<Args>()))...
    >;

    template <std::size_t Arity, typename... Args>
    constexpr bool all_results_same_type() {
        if constexpr (Arity == 0) {
            return true;
        }
        using first_result = invoke_result_at_i<0, Args...>;

        // Compares all with first_result.
        return [] <std::size_t... Indexes>(std::integer_sequence<std::size_t, Indexes...>) constexpr -> bool {
            return (... && std::is_same_v<first_result, invoke_result_at_i<Indexes, Args...>>);
        }(std::make_index_sequence<Arity>());
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

template<detail::not_empty... Args>
constexpr decltype(auto) invoke_forall(Args&&... args) {
    if constexpr ((... || detail::is_gettable<Args>())) {

        // Tutaj implementacja dla przypadku, gdy przynajmniej jeden argument jest wyciągalny
        constexpr std::size_t arity = detail::compute_arity<Args...>();
        static_assert(arity > 0);
        // if constexpr (arity==0){
        //     // Brak wyciągalnych argumentów
        //     // To chyba nie moze sie stać:
        //     return std::invoke(std::forward<Args>(args)...);
        // }

        if constexpr (detail::all_results_same_type< arity, Args...>()) {
            // Wszystkie wyniki mają ten sam typ wiec bedzie Array od arrity oraz first_result
        }
        else {
            // Wyniki mają różne typy wiec bedzie Tuple od arrity
        }
    } else {
        // Przypadek, gdy żaden argument nie jest wyciągalny
        return std::invoke(std::forward<Args>(args)...);
    }
}

#endif
