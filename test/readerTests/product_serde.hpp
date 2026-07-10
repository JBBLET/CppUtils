#pragma once

// User-side deserialization for the kitchen-sink `Product`.
//
// This file is deliberately *not* part of the library. It shows how a user
// wires a domain type to CSV: primitive fields go through csv::convert, and
// every [CUSTOM] column (enums, dates, Vec3, percent, tags) gets a hand-written
// transform function. The library never needs to know these types exist.

#include <chrono>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "cpputils/error.hpp"
#include "csv/convert.hpp"
#include "sample_types.hpp"

namespace csvutils::sample {

namespace detail {

using Row = std::unordered_map<std::string, std::string>;

// Look up a required column; a missing header is a hard error, not an empty cell.
inline auto field(const Row& row, std::string_view key) -> const std::string& {
    auto it = row.find(std::string{key});
    ensure(it != row.end(), "Missing column '{}'", key);
    return it->second;
}

// [OPTIONAL] Empty cell -> nullopt, otherwise run the inner transform.
template <class F>
auto optional(std::string_view s, F&& inner) -> std::optional<std::invoke_result_t<F, std::string_view>> {
    if (s.empty()) return std::nullopt;
    return std::forward<F>(inner)(s);
}

// [CONTAINER] Split one cell into N pieces on a delimiter; empty cell -> {}.
inline auto split(std::string_view s, char delim) -> std::vector<std::string> {
    std::vector<std::string> out;
    if (s.empty()) return out;
    std::size_t start = 0;
    while (true) {
        auto pos = s.find(delim, start);
        out.emplace_back(s.substr(start, pos - start));
        if (pos == std::string_view::npos) break;
        start = pos + 1;
    }
    return out;
}

// [ENUM<-string]
inline auto currency(std::string_view s) -> Currency {
    if (s == "USD") return Currency::USD;
    if (s == "EUR") return Currency::EUR;
    if (s == "GBP") return Currency::GBP;
    ensure(false, "Unknown currency '{}'", s);
    std::unreachable();
}

inline auto status(std::string_view s) -> Status {
    if (s == "Active") return Status::Active;
    if (s == "Discontinued") return Status::Discontinued;
    if (s == "Pending") return Status::Pending;
    ensure(false, "Unknown status '{}'", s);
    std::unreachable();
}

// [CUSTOM struct] "10x5x3" -> Vec3.
inline auto vec3(std::string_view s) -> Vec3 {
    auto parts = split(s, 'x');
    ensure(parts.size() == 3, "Expected 'AxBxC' for Vec3, got '{}'", s);
    return Vec3{csv::convert::parseFloat<double>(parts[0]),
                csv::convert::parseFloat<double>(parts[1]),
                csv::convert::parseFloat<double>(parts[2])};
}

// [CUSTOM number] "12.5%" -> 12.5 (kept as a human percent, not divided by 100).
inline auto percent(std::string_view s) -> double {
    ensure(!s.empty() && s.back() == '%', "Expected trailing '%' in '{}'", s);
    s.remove_suffix(1);
    return csv::convert::parseFloat<double>(s);
}

// [CUSTOM/date] ISO-8601 instant "2026-07-09T14:30:00Z".
inline auto sysSeconds(std::string_view s) -> std::chrono::sys_seconds {
    std::istringstream ss{std::string{s}};
    std::chrono::sys_seconds tp{};
    ss >> std::chrono::parse("%Y-%m-%dT%H:%M:%SZ", tp);
    ensure(!ss.fail(), "Cannot parse ISO-8601 datetime from '{}'", s);
    return tp;
}

// [CUSTOM/date] Slash calendar date "2026/07/09".
inline auto ymd(std::string_view s) -> std::chrono::year_month_day {
    std::istringstream ss{std::string{s}};
    std::chrono::year_month_day out{};
    ss >> std::chrono::parse("%Y/%m/%d", out);
    ensure(!ss.fail(), "Cannot parse date from '{}'", s);
    return out;
}

}  // namespace detail

// Map one header-keyed row to a Product.
inline auto productFromRow(const std::unordered_map<std::string, std::string>& row) -> Product {
    using namespace csv::convert;
    namespace d = detail;

    Product p;
    p.id = parseInt<std::int64_t>(d::field(row, "id"));
    p.sku = parseString(d::field(row, "sku"));
    p.price = parseFloat<double>(d::field(row, "price"));
    p.currency = d::currency(d::field(row, "currency"));
    p.quantity = parseInt<std::uint32_t>(d::field(row, "quantity"));
    p.inStock = parseBool(d::field(row, "in_stock"));
    p.rating = d::optional(d::field(row, "rating"), [](std::string_view s) { return parseFloat<double>(s); });
    p.tags = d::split(d::field(row, "tags"), '|');
    p.createdAt = d::sysSeconds(d::field(row, "created_at"));
    p.releaseDate = d::ymd(d::field(row, "release_date"));
    p.weightKg = d::optional(d::field(row, "weight_kg"), [](std::string_view s) { return parseFloat<double>(s); });
    p.dimensions = d::vec3(d::field(row, "dimensions"));
    p.discountPct = d::percent(d::field(row, "discount_pct"));
    p.status = d::status(d::field(row, "status"));
    p.description = parseString(d::field(row, "description"));
    p.score = parseFloat<float>(d::field(row, "score"));
    p.categoryId =
        d::optional(d::field(row, "category_id"), [](std::string_view s) { return parseInt<std::int32_t>(s); });
    return p;
}

}  // namespace csvutils::sample
