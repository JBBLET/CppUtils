#pragma once

#include <filesystem>
#include <format>
#include <fstream>
#include <string>
#include <string_view>

#include "cpputils/error.hpp"

struct File {
    std::filesystem::path path;

    auto exists() const -> bool;
    explicit File(std::string_view pathString);
    auto stream() -> std::ifstream;
};

template <>
struct std::formatter<File> : std::formatter<std::string> {
    auto format(const File& f, std::format_context& ctx) const {
        return std::formatter<std::string>::format(f.path.parent_path(), ctx);
    }
};

inline File::File(std::string_view pathString) : path{pathString} {
    ensure(exists(), "File at {} could not be found ", *this);
}
inline auto File::stream() -> std::ifstream {
    auto file = std::ifstream{path};
    ensure(file.is_open(), "Could not open file at {}", *this);
    return file;
};

inline auto File::exists() const -> bool { return std::filesystem::exists(path); }
