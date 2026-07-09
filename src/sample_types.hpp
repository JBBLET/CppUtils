#pragma once

// A deliberately maximal target type for `test/resources/kitchen_sink.csv`.
//
// The point of this file is NOT to be a realistic domain model -- it is to
// enumerate every *category of conversion* your generic parser must let a user
// plug in. For each member below, the comment marks which conversion category
// it belongs to. Your library's job is to make the [BUILTIN] ones effortless
// and the [CUSTOM] ones possible via a user-supplied transform function.

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace csvutils::sample {

// [ENUM<-string]  User maps token -> enumerator; also consider case-insensitivity.
enum class Currency : std::uint8_t { USD, EUR, GBP };
enum class Status : std::uint8_t { Active, Discontinued, Pending };

// [CUSTOM struct]  One cell "10x5x3" must fan out into a small aggregate.
// This is the flagship case for "user provides a transform function".
struct Vec3 {
    double x{}, y{}, z{};
};

struct Product {
    std::int64_t id{};    // [BUILTIN int]   from_chars
    std::string sku{};    // [BUILTIN string] + whitespace-trim policy ("   PAD-7   ")
    double price{};       // [BUILTIN float]  (money: watch precision/locale)
    Currency currency{};  // [ENUM<-string]

    std::uint32_t quantity{};        // [BUILTIN uint]  (unsigned; reject negatives)
    bool inStock{};                  // [BUILTIN bool]  BUT tokens vary: true/false/1/0/yes/no
    std::optional<double> rating{};  // [OPTIONAL]      "" -> nullopt, else parse inner

    std::vector<std::string> tags{};            // [CONTAINER]     split one cell on '|' -> N values
    std::chrono::sys_seconds createdAt{};       // [CUSTOM/date]   ISO-8601 "2026-07-09T14:30:00Z"
    std::chrono::year_month_day releaseDate{};  // [CUSTOM/date]   "2026/07/09" (different format, same file)

    std::optional<double> weightKg{};  // [OPTIONAL float]
    Vec3 dimensions{};                 // [CUSTOM struct] "10x5x3"
    double discountPct{};              // [CUSTOM number] "12.5%" -> strip '%', maybe /100

    Status status{};                           // [ENUM<-string]
    std::string description{};                 // [BUILTIN string] the "hard" column: quoted commas,
                                               //                  escaped "" quotes, embedded newlines
    float score{};                             // [BUILTIN float]  narrow width; "1" must parse as 1.0f
    std::optional<std::int32_t> categoryId{};  // [OPTIONAL int]   trailing empty field
};

}  // namespace csvutils::sample
