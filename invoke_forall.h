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
#include <iterator>

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
template <typename... Args>
concept not_empty = (sizeof...(Args) > 0);

// Concept: type behaves like a tuple (has std::tuple_size specialization).
template <typename T>
concept is_tuple_like =
    requires {
        typename std::tuple_size<bare_type_t<T>>;
        {std::tuple_size<bare_type_t<T>>::value} ->
            std::convertible_to<std::size_t>;
    };

// Concept: type is protected (has get_protected_value() method).
template <typename T>
concept is_protected = requires (T t) {
    { t.get_protected_value() };
};

// ============================================================================
// Tuple Element Accessibility Check
// ============================================================================

// Checks if std::get<Index>(T) is valid.
template <class T, std::size_t Index>
concept is_index_gettable =
    requires { std::get<Index>(std::declval<T>()); };

// Determines if all elements of a tuple-like type are accessible.
template <typename T>
constexpr bool is_gettable() {
    using U = bare_type_t<T>;

    if constexpr (!is_tuple_like<U>) {
        return false;
    }
    else {
        constexpr auto N = std::tuple_size<U>::value;
        return []<std::size_t... Indexes>(
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
    }
    else {
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
    std::remove_reference_t<T> value_;

public:
    template <typename U>
    constexpr explicit protected_arg(U&& v)
        : value_(std::forward<U>(v)) {}

    constexpr decltype(auto) get_protected_value() & {
        return (value_);
    }

    constexpr decltype(auto) get_protected_value() && {
        return std::move(value_);
    }

    constexpr decltype(auto) get_protected_value() const & {
        return (value_);
    }
};

// ============================================================================
// Reference Array - Container for L-value References
// ============================================================================

// Custom container that stores pointers to allow l-value reference semantics.
// Satisfies random_access_range concept.
template <typename T, std::size_t N>
class reference_array {
private:
    using value_t = std::remove_reference_t<T>;
    std::array<value_t*, N> ptrs_;

public:
    // Constructor taking references and storing their addresses
    template <typename... Refs>
    constexpr explicit reference_array(Refs&&... refs)
        : ptrs_{std::addressof(refs)...} {
        static_assert(sizeof...(Refs) == N,
            "Number of references must match array size");
    }

    // Element access operators
    constexpr T operator[](std::size_t i) const {
        return *ptrs_[i];
    }

    constexpr std::size_t size() const {
        return N;
    }

    // Random access iterator for range support
    class iterator {
    private:
        value_t **ptr_;

    public:
        using value_type = value_t;
        using reference = T;
        using pointer = value_t*;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::random_access_iterator_tag;

        constexpr iterator() : ptr_(nullptr) {}

        constexpr explicit iterator(value_t **p) : ptr_(p) {}

        constexpr reference operator*() const {
            return **ptr_;
        }

        constexpr pointer operator->() const {
            return *ptr_;
        }

        constexpr reference operator[](difference_type n) const {
            return *ptr_[n];
        }

        constexpr iterator& operator++() {
            ++ptr_;
            return *this;
        }

        constexpr iterator operator++(int) {
            auto tmp = *this;
            ++ptr_;
            return tmp;
        }

        constexpr iterator& operator--() {
            --ptr_;
            return *this;
        }

        constexpr iterator operator--(int) {
            auto tmp = *this;
            --ptr_;
            return tmp;
        }

        constexpr iterator& operator+=(difference_type n) {
            ptr_ += n;
            return *this;
        }

        constexpr iterator& operator-=(difference_type n) {
            ptr_ -= n;
            return *this;
        }

        constexpr iterator operator+(difference_type n) const {
            return iterator(ptr_ + n);
        }

        constexpr iterator operator-(difference_type n) const {
            return iterator(ptr_ - n);
        }

        constexpr difference_type operator-(const iterator& other) const {
            return ptr_ - other.ptr_;
        }

        constexpr bool operator==(const iterator& other) const = default;

        constexpr auto operator<=>(const iterator& other) const = default;

        friend constexpr iterator operator+(
            difference_type n, const iterator& it) {
            return it + n;
        }
    };

    constexpr iterator begin() const {
        return iterator(const_cast<value_t**>(ptrs_.data()));
    }

    constexpr iterator end() const {
        return iterator(const_cast<value_t**>(ptrs_.data() + N));
    }
};

// get function for reference_array (ADL support)
template <std::size_t I, typename T, std::size_t N>
constexpr T get(const reference_array<T, N>& arr) {
    static_assert(I < N, "Index out of bounds");
    return arr[I];
}

// ============================================================================
// Element Extraction for Invocation
// ============================================================================

// Extracts the I-th element from an argument for invocation:
// - Protected: returns the wrapped value directly (highest priority)
// - Gettable: returns std::get<I>(arg)
// - Regular: returns arg
template <std::size_t I, typename T>
constexpr decltype(auto) extract_for_invoke(T&& arg) {
    using bare_t = bare_type_t<T>;

    if constexpr (is_protected<bare_t>) {
        return std::forward<T>(arg).get_protected_value();
    }
    else if constexpr (is_gettable<T>()) {
        return std::get<I>(std::forward<T>(arg));
    }
    else {
        return std::forward<T>(arg);
    }
}

// ============================================================================
// Invocation Result Type Detection
// ============================================================================

// Helper to compute result type
template <std::size_t I, typename... Args>
struct invoke_result_at_i_impl {
    using type = decltype(
        std::invoke(extract_for_invoke<I>(std::declval<Args>())...)
    );
};

// Result type of invoking with I-th extracted arguments
template <std::size_t I, typename... Args>
using invoke_result_at_i = typename invoke_result_at_i_impl<I, Args...>::type;

// Helper struct for compile-time type comparison
template <std::size_t Arity, typename... Args>
struct all_results_same_type_impl {
    template <std::size_t... Indexes>
    static constexpr bool check(std::index_sequence<Indexes...>) {
        using first_result = invoke_result_at_i<0, Args...>;
        return (... && std::is_same_v<
            first_result,
            invoke_result_at_i<Indexes, Args...>
        >);
    }

    static constexpr bool value = check(std::make_index_sequence<Arity>());
};

// Checks if all invocation results have the same type.
// Returns: std::nullopt if void, true if all same, false if different
template <std::size_t Arity, typename... Args>
constexpr std::optional<bool> all_results_same_type() {
    if constexpr (Arity == 0) {
        return true;
    }

    using first_result = invoke_result_at_i<0, Args...>;

    if constexpr (std::is_void_v<first_result>) {
        return std::nullopt;
    }
    else {
        return all_results_same_type_impl<Arity, Args...>::value;
    }
}

// ============================================================================
// Invocation Implementations
// ============================================================================

// Creates std::tuple with results from invoking across all indices
template <std::size_t Arity, typename... Args>
constexpr auto make_tuple_of_results(Args&&... args) {
    return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
        auto invoke_at = [&](auto index_ic) {
            constexpr std::size_t I = decltype(index_ic)::value;
            return std::invoke(
                extract_for_invoke<I>(std::forward<Args>(args))...
            );
        };
        return std::make_tuple(
            invoke_at(std::integral_constant<std::size_t, Is>{})...
        );
    }(std::make_index_sequence<Arity>{});
}

// Creates container with results from invoking across all indices.
// Uses reference_array for l-value references, std::array otherwise.
template <std::size_t Arity, typename... Args>
constexpr auto make_array_of_results(Args&&... args) {
    return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
        auto invoke_at = [&](auto index_ic) {
            constexpr std::size_t I = decltype(index_ic)::value;
            return std::invoke(
                extract_for_invoke<I>(std::forward<Args>(args))...
            );
        };

        using first_result = decltype(
            invoke_at(std::integral_constant<std::size_t, 0>{})
        );

        if constexpr (std::is_lvalue_reference_v<first_result>) {
            return reference_array<first_result, Arity>(
                invoke_at(std::integral_constant<std::size_t, Is>{})...
            );
        }
        else {
            return std::array<first_result, Arity>{
                invoke_at(std::integral_constant<std::size_t, Is>{})...
            };
        }
    }(std::make_index_sequence<Arity>{});
}

// Invokes across all indices with void return type.
// Discards individual results.
template <std::size_t Arity, typename... Args>
constexpr void execute_void_invokes(Args&&... args) {
    [&]<std::size_t... Is>(std::index_sequence<Is...>) {
        auto invoke_at = [&](auto index_ic) {
            constexpr std::size_t I = decltype(index_ic)::value;
            return std::invoke(
                extract_for_invoke<I>(std::forward<Args>(args))...
            );
        };
        (invoke_at(std::integral_constant<std::size_t, Is>{}), ...);
    }(std::make_index_sequence<Arity>{});
}

} // namespace detail

// ============================================================================
// Specializations in std namespace (must be outside detail namespace)
// ============================================================================

namespace std {

// Specialization for tuple_size (enables structured bindings)
template <typename T, std::size_t N>
struct tuple_size<detail::reference_array<T, N>>
    : integral_constant<std::size_t, N> {};

// Specialization for tuple_element
template <std::size_t I, typename T, std::size_t N>
struct tuple_element<I, detail::reference_array<T, N>> {
    using type = T;
};

} // namespace std

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
// - reference_array<T&, N> if all invocations return l-value reference T&
// - std::array<T, N> if all invocations return same value type T
// - std::tuple<T1, T2, ...> if invocations return different types
// - std::tuple<> if invocations return void
//
template <detail::not_empty... Args>
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
        }
        else if constexpr (
            detail::all_results_same_type<arity, Args...>() == true) {
            // All invocations return same type -> return array or ref_array
            return detail::make_array_of_results<arity>(
                std::forward<Args>(args)...);
        }
        else {
            // Invocations return different types -> return tuple
            return detail::make_tuple_of_results<arity>(
                std::forward<Args>(args)...);
        }
    }
    else {
        // No tuple-like arguments: standard invocation
        return std::invoke(detail::extract_for_invoke<0>(
            std::forward<Args>(args))...);
    }
}

#endif // INVOKE_FORALL_H
