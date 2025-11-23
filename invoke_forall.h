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

    template<class T, class = void>
    struct isTupleLike : std::false_type {};

    // T nie wspiera operacji tuple_size_v.
    template<class T>
    struct isTupleLike<T, std::void_t<decltype(std::tuple_size_v<T>)>> : std::true_type {};


    // =============================================================================
    // Sprawdzenie Gettable i Arity
    // =============================================================================

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
        using Bare = bare_type_t<T>; // T bez cv i referencji

        if (!isTupleLike<Bare>::value)
            return false;

        static constexpr std::size_t arity = std::tuple_size_v<Bare>;
        auto indexes = std::make_index_sequence<arity>();
        return allIndexesGettable(indexes);
    }

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
        ((result = result != 0 ? result : get_gettable_arity<Args>()), ...);
        return result;
    }

    // =============================================================================
    // Typy odpowiedzi i funckcje pomocnicze do ich okreslenia
    // =============================================================================

    //template <std::size_t I, typename T>
    //constexpr decltype(auto) extract_for_invoke(T && arg)
    // zwraca dla nongettable pp typ arg a
    // dla gettable pp std::get<I>(arg)

    template < std::size_t I, typename... Args>
    using invoke_result_at_i = std::invoke_result_t<
        decltype(extract_for_invoke<I>(std::declval<Args>()))...
    >;

    template < std::size_t Arity, typename... Args>
    constexpr bool all_results_same_type() {
        if constexpr (Arity == 0) {
            return true;
        }
        using first_result = invoke_result_at_i< 0, Args...>;
        bool same = true;
        //TODO
        return same;
    }

    // func: zrob tuple odpowiedz
    // func: zrob array odpowiedz
    // co jak wszytskie to void??? nwm



    // ==============================================================================
    // template <typename T> constexpr typ protect_arg(T&&) wykonuje konstruktor na tym
    // ==============================================================================

    // Opakowuje argument, aby zapobiec jego rozbiciu przez std::invoke.
    // Używane do przekazywania argumentów krotkowych jako pojedynczych argumentów.
    template <typename T>
    class ProtectedArg {
    private:
        T value_;
    public:
        //TODO konstruktory, get_protected_value
    };


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
    if constexpr ((... || detail::isGettable(args))) {

        // Tutaj implementacja dla przypadku, gdy przynajmniej jeden argument jest wyciągalny
        constexpr std::size_t arity = detail::compute_arity<Args...>();

        if constexpr (arity==0){
            // Brak wyciągalnych argumentów
            // To chyba nie moze sie stać:
            return std::invoke(std::forward<Args>(args)...);
        }

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
