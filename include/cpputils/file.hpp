#pragma once

#include <filesystem>
#include <format>
#include <fstream>
#include <string>
#include <utility>

#include "cpputils/error.hpp"

namespace cpputils::files {

// A path plus the open/exists checks that always accompany it. Construction never
// throws — a File may name something not yet created; existence is enforced on read().
struct File {
    std::filesystem::path path;

    File() = default;
    explicit File(std::filesystem::path filePath);

    auto exists() const -> bool;
    auto remove() const -> bool;

    // Throws if the file is missing or cannot be opened.
    auto read() const -> std::ifstream;

    // Creates any missing parent directories. Truncates unless append is set.
    auto write(bool append = false) const -> std::ofstream;
};

}  // namespace cpputils::files

template <>
struct std::formatter<cpputils::files::File> : std::formatter<std::string> {
    auto format(const cpputils::files::File& f, std::format_context& ctx) const {
        return std::formatter<std::string>::format(f.path.string(), ctx);
    }
};

namespace cpputils::files {

inline File::File(std::filesystem::path filePath) : path{std::move(filePath)} {}

inline auto File::exists() const -> bool { return std::filesystem::exists(path); }

inline auto File::remove() const -> bool { return std::filesystem::remove(path); }

inline auto File::read() const -> std::ifstream {
    ensure(exists(), "File not found: {}", *this);
    auto file = std::ifstream{path};
    ensure(file.is_open(), "Could not open file for reading: {}", *this);
    return file;
}

inline auto File::write(bool append) const -> std::ofstream {
    if (path.has_parent_path()) std::filesystem::create_directories(path.parent_path());
    auto file = std::ofstream{path, append ? std::ios::app : std::ios::trunc};
    ensure(file.is_open(), "Could not open file for writing: {}", *this);
    return file;
}

}  // namespace cpputils::files
