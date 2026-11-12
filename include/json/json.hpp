#pragma once

#include <concepts>
#include <ranges>
#include <string>
#include <tuple>
#include <utility>
#include <variant>
namespace json::impl {
// Containers
template <class T>
concept Range = std::ranges::range<T>;

template <class T>
concept MapLike = Range<T> && requires {
    typename T::key_type;
    typename T::mapped_type;
} && (std::is_same_v<typename T::value_type, std::pair<const typename T::key_type, typename T::mapped_type>>);

// Is the key construtible into a string for serialization
template <class K>
concept StringKey = std::constructible_from<std::string, K>;
// Nullable

template <class T>
concept Nullable = requires(const T& t) {
    static_cast<bool>(t);  // has a value (is non null)
    *t;                    // dereferenceable
};

// String Like
template <class T>
concept StringLike = std::is_same_v<T, std::string> || std::is_same_v<T, std::string_view>;

// CStringLike
template <class T>
concept CStringLike = std::is_same_v<T, const char*> || std::is_same_v<T, char*>;

// Variant
template <class T>
concept VariantLike = requires { std::variant_size<T>::value; };

// Tuple,Pair
template <class T>
concept TupleLike = requires { std::tuple_size<T>::value; };
};  // namespace json::impl
