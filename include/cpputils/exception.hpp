#pragma once

#include <format>
#include <stacktrace>
#include <stdexcept>
#include <string>
#include <utility>

namespace cpputils {

// Skip is how many leading stacktrace frames to drop so frame 0 is the throw site.
// Derived classes add one frame via their inherited constructor, hence Skip = 2 below.
template <class Base, std::size_t Skip = 1>
class TracedError : public Base {
 public:
    template <class... Args>
    constexpr TracedError(std::format_string<Args...> msg, Args&&... args)
        : Base{std::format(msg, std::forward<Args>(args)...)},
          what_{std::format("{}\n{}", Base::what(), std::stacktrace::current(Skip))} {}

    constexpr auto to_string(this auto&& self) -> std::string { return self.what_; }

    constexpr auto what() const noexcept -> const char* override { return what_.c_str(); }

 private:
    std::string what_;
};

class Exception : public TracedError<std::runtime_error, 2> {
 public:
    using TracedError::TracedError;
};

class InvalidArgument : public TracedError<std::invalid_argument, 2> {
 public:
    using TracedError::TracedError;
};

}  // namespace cpputils
