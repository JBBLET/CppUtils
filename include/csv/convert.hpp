#pragma once

// The [BUILTIN] toolkit: primitive-cell converters only.
//
// The library deliberately knows nothing about enums, dates, or user structs.
// It turns a cell into a primitive scalar; every non-primitive conversion is a
// user-supplied transform function that composes these.

#include <charconv>
#include <concepts>
#include <string>
#include <string_view>
#include <utility>

#include "cpputils/error.hpp"

namespace csv::convert {

// [BUILTIN int/uint] Whole cell must be consumed, else it's a parse error.
template <std::integral T>
auto parseInt(std::string_view s) -> T {
    T value{};
    auto* end = s.data() + s.size();
    auto [ptr, ec] = std::from_chars(s.data(), end, value);
    ensure(ec == std::errc{} && ptr == end, "Cannot parse integer from '{}'", s);
    return value;
}

// [BUILTIN float] Same whole-cell rule; from_chars handles "5" -> 5.0 too.
template <std::floating_point T>
auto parseFloat(std::string_view s) -> T {
    T value{};
    auto* end = s.data() + s.size();
    auto [ptr, ec] = std::from_chars(s.data(), end, value);
    ensure(ec == std::errc{} && ptr == end, "Cannot parse float from '{}'", s);
    return value;
}

// [BUILTIN bool] Tokens vary across files: true/false, 1/0, yes/no (any case).
inline auto parseBool(std::string_view s) -> bool {
    std::string lower;
    lower.reserve(s.size());
    for (char c : s) lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    if (lower == "true" || lower == "1" || lower == "yes") return true;
    if (lower == "false" || lower == "0" || lower == "no") return false;
    ensure(false, "Cannot parse bool from '{}'", s);
    std::unreachable();
}

// [BUILTIN string] Identity: the cell is already the value (reader trims it).
inline auto parseString(std::string_view s) -> std::string { return std::string{s}; }

}  // namespace csv::convert
