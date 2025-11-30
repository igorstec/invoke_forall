/* ============================================================================
 * invoke_forall.h
 *
 * This module provides a metaprogramming utility for invoking callable objects
 * across multiple indices extracted from tuple-like structures.
 *
 * ============================================================================
 */

#ifndef INVOKE_FORALL_H
#define INVOKE_FORALL_H

#include <array>
#include <cstddef>
#include <optional>
#include <tuple>
#include <functional>
#include <type_traits>
#include <utility>

namespace detail {

// ============================================================================
// Type Traits and Concepts for Argument Analysis
// ============================================================================

// Extracts bare type (removes const, volatile, references).
template <typename T>
struct bare_type {
    using type = std::remove_cvref_t<T>;
};

template <typename T>
using bare_type_t = typename bare_type<T>::type;

// Concept: argument pack must be non-empty.
template<typename... Args>
concept not_empty = (sizeof...(Args) > 0);

// Concept: type behaves like a tuple (has std::tuple_size specialization).
template<typename T>
concept is_tuple_like =
    requires {
        typename std::tuple_size<bare_type_t<T>>;
        {std::tuple_size<bare_type_t<T>>::value} ->
            std::convertible_to<std::size_t>;
    };

// Concept: type is protected (has get_protected_value() method).
template<typename T>
concept is_protected = requires (T t) {
    t.get_protected_value();
};

// ============================================================================
// Tuple Element Accessibility Check
// ============================================================================

// Checks if std::get<Index>(T) is valid.
template<class T, std::size_t Index>
concept is_index_gettable =
    requires { std::get<Index>(std::declval<T>()); };

// Determines if all elements of a tuple-like type are accessible.
template<typename T>
constexpr bool is_gettable() {
    using U = bare_type_t<T>;

    if constexpr (!is_tuple_like<U>) {
        return false;
    } else {
        constexpr auto N = std::tuple_size<U>::value;
        return [] <std::size_t... Indexes>(
            std::integer_sequence<std::size_t, Indexes...>) constexpr -> bool {
            return (... && is_index_gettable<U, Indexes>);
        }(std::make_index_sequence<N>());
    }
}

// ============================================================================
// Arity Computation
// ============================================================================

// Gets arity (tuple size) from a gettable type.
template <typename T>
constexpr std::size_t get_gettable_arity() {
    if constexpr (is_gettable<T>()) {
        return std::tuple_size_v<bare_type_t<T>>;
    } else {
        return 0;
    }
}

// Computes unified arity from all arguments.
template <typename... Args>
constexpr std::size_t compute_arity() {
    std::size_t result = 0;
    ((result = result != 0 ? result : get_gettable_arity<Args>()), ...);
    return result;
}

// ============================================================================
// Protected Argument Wrapper
// ============================================================================

// Wraps a tuple-like object to prevent unpacking by std::invoke.
// Allows passing tuple-like types as single arguments.
template <typename T>
class protected_arg {
private:
    T&& value_;

public:
    constexpr explicit protected_arg(T&& value)
        : value_(std::forward<T>(value)) {}

    constexpr T&& get_protected_value() const {
        return std::forward<T>(value_);
    }
};

// ============================================================================
// Element Extraction for Invocation
// ============================================================================

// Extracts the I-th element from an argument for invocation:
// - Gettable: returns std::get<I>(arg)
// - Protected: returns the wrapped value directly
// - Regular: returns arg
template <std::size_t I, typename T>
constexpr decltype(auto) extract_for_invoke(T&& arg) {
    if constexpr (is_gettable<T>()) {
        return std::get<I>(std::forward<T>(arg));
    } else if constexpr (is_protected<T>) {
        return arg.get_protected_value();
    } else {
        return std::forward<T>(arg);
    }
}

// ============================================================================
// Invocation Result Type Detection
// ============================================================================

// Result type of invoking with I-th extracted arguments
template <std::size_t I, typename Func, typename... Args>
using invoke_result_at_i =
    std::invoke_result_t<
        Func,
        decltype(extract_for_invoke<I>(std::declval<Args>()))...
    >;

// Checks if all invocation results have the same type.
// Returns: std::nullopt if void, true if all same, false if different
template <std::size_t Arity, typename Func, typename... Args>
constexpr std::optional<bool> all_results_same_type() {
    if constexpr (Arity == 0) {
        return true;
    } else if constexpr (is_tuple_like<Func>) {
        // Tuple of callables
        return false;
    } else {
        using first_result = invoke_result_at_i<0, Func, Args...>;
        if constexpr (std::is_void_v<first_result>) {
            return std::nullopt;
        } else {
            return [] <std::size_t... Indexes>(
                std::integer_sequence<std::size_t, Indexes...>
            ) constexpr -> bool {
                return (... &&
                    std::is_same_v<
                        first_result,
                        invoke_result_at_i<Indexes, Func, Args...>
                    >);
            }(std::make_index_sequence<Arity>());
        }
    }
}

// ============================================================================
// Invocation Implementations
// ============================================================================

// Creates std::tuple with results from invoking across all indices
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

// Creates std::array with results from invoking across all indices.
// All results must have the same type.
template <std::size_t Arity, typename... Args>
constexpr auto make_array_of_results(Args&&... args) {
    return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
        auto call_one = [&](auto ic) {
            constexpr std::size_t I = decltype(ic)::value;
            return std::invoke(
                extract_for_invoke<I>(std::forward<Args>(args))...
            );
        };
        return std::array{
            call_one(std::integral_constant<std::size_t, Is>{})...
        };
    }(std::make_index_sequence<Arity>{});
}

// Invokes across all indices with void return type.
// Discards individual results.
template <std::size_t Arity, typename... Args>
constexpr void execute_void_invokes(Args&&... args) {
    [&]<std::size_t... Is>(std::index_sequence<Is...>) {
        auto call_one = [&](auto ic) {
            constexpr std::size_t I = decltype(ic)::value;
            return std::invoke(
                extract_for_invoke<I>(std::forward<Args>(args))...
            );
        };
        (call_one(std::integral_constant<std::size_t, Is>{}), ...);
    }(std::make_index_sequence<Arity>{});
}

} // namespace detail

// ============================================================================
// Public API
// ============================================================================

// Creates a protected wrapper around an argument to prevent unpacking.
// Use this to pass tuple-like objects as single arguments.
template <typename T>
constexpr auto protect_arg(T&& arg) {
    return detail::protected_arg<T>(std::forward<T>(arg));
}

// Invokes callable objects across multiple indices extracted from arguments.
//
// Behavior:
// - If no arguments are tuple-like, invokes normally with all arguments.
// - If some arguments are tuple-like, invokes N times (N = arity),
//   extracting the I-th element from each tuple-like argument.
//
// Return type:
// - Array<T, N> if all invocations return type T
// - Tuple<T1, T2, ...> if invocations return different types
// - void (returns 1) if invocations return void
//
template<detail::not_empty... Args>
constexpr decltype(auto) invoke_forall(Args&&... args) {
    if constexpr ((... || detail::is_gettable<Args>())) {
        constexpr std::size_t arity = detail::compute_arity<Args...>();

        static_assert(arity > 0,
            "Computed arity must be greater than 0 for tuple-like arguments");

        if constexpr (detail::all_results_same_type<arity, Args...>() ==
                      std::nullopt) {
            // All invocations return void
            detail::execute_void_invokes<arity>(std::forward<Args>(args)...);
            return std::tuple<>{};
        } else if constexpr (
            detail::all_results_same_type<arity, Args...>() == true) {
            // All invocations return same type -> return array
            return detail::make_array_of_results<arity>(
                std::forward<Args>(args)...);
        } else {
            // Invocations return different types -> return tuple
            return detail::make_tuple_of_results<arity>(
                std::forward<Args>(args)...);
        }
    } else {
        // No tuple-like arguments: standard invocation
        return std::invoke(detail::extract_for_invoke<0>(
            std::forward<Args>(args))...);
    }
}

#endif // INVOKE_FORALL_H
