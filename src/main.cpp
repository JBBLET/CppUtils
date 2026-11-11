#include <array>
#include <cstdio>
#include <string>
#include <tuple>
#include <utility>
#include <variant>

#include "json/jsonSerialize.hpp"  // adjust to your path

struct Point {
    int x;
    int y;
};

struct Bag {
    std::tuple<int, std::string, double> tup;   // heterogeneous → array
    std::pair<int, std::string> pr;             // standalone pair → 2-array (NOT key/value)
    std::array<int, 3> arr;                     // range path → array
    std::variant<int, std::string, Point> var;  // discriminated
};

int main() {
    Bag b{
        .tup = {1, "two", 3.5},
        .pr = {7, "seven"},
        .arr = {9, 8, 7},
        .var = std::string{"hello"},  // alternative index 1
    };
    std::puts(json::serializer::toJson(b).c_str());

    b.var = 42;  // index 0 (int)
    std::puts(json::serializer::toJson(b).c_str());

    b.var = Point{1, 2};  // index 2 (struct → recurse)
    std::puts(json::serializer::toJson(b).c_str());
}
