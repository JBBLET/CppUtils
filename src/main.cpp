#include <cstdio>
#include <memory>
#include <optional>
#include <string>

#include "json/jsonSerialize.hpp"  // adjust to your path

struct Inner {
    int a;
    std::string b;
};

struct Wrap {
    std::optional<int> maybeInt;
    std::optional<std::string> maybeStr;
    std::optional<Inner> maybeObj;  // optional of a struct → recurse
    std::unique_ptr<Inner> owned;   // owning ptr → recurse
    std::shared_ptr<int> shared;    // owning ptr → scalar
    const char* cstr;               // Trap 2: must be a string, not a char
};

int main() {
    Wrap present{
        .maybeInt = 42,
        .maybeStr = "hi",
        .maybeObj = Inner{1, "x"},
        .owned = std::make_unique<Inner>(Inner{2, "y"}),
        .shared = std::make_shared<int>(7),
        .cstr = nullptr,
    };
    std::puts(json::serializer::toJson(present).c_str());
    /*
        Wrap empty{
            .maybeInt = std::nullopt,
            .maybeStr = std::nullopt,
            .maybeObj = std::nullopt,
            .owned = nullptr,
            .shared = nullptr,
            .cstr = nullptr,
        };
        std::puts(json::serializer::to_json(empty).c_str());*/
}
