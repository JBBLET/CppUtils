#pragma once

#include <format>
#include <stacktrace>
#include <utility>

#include "cpputils/exception.hpp"
#include "cpputils/logging.hpp"

namespace cpputils {
template <class... Args>
constexpr auto expect(bool predicate, std::format_string<Args...> msg, Args&&... args) -> void;

template <class... Args>
constexpr auto ensure(bool predicate, std::format_string<Args...> msg, Args&&... args) -> void;

template <class... Args>
constexpr auto expect(bool predicate, std::format_string<Args...> msg, Args&&... args) -> void {
    if (!predicate) {
        logging::error("{}", std::format(msg, std::forward<Args>(args)...));
        logging::error("{}", std::stacktrace::current(1));
        std::terminate();
        std::unreachable();
    }
}

template <class... Args>
constexpr auto ensure(bool predicate, std::format_string<Args...> msg, Args&&... args) -> void {
    if (!predicate) {
        throw Exception(msg, std::forward<Args>(args)...);
    }
}
}  // namespace cpputils
