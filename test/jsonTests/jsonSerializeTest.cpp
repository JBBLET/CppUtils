#include <gtest/gtest.h>

#include <array>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

#include "json/jsonSerialize.hpp"  // adjust include path to your layout

using json::serializer::toJson;

// ---- shared fixtures -------------------------------------------------------
enum Color { Red, Green, Blue };                  // unscoped: 0,1,2
enum class Status { Pending, Cleared, Settled };  // scoped:   0,1,2

struct Point {
    int x;
    int y;
};
struct Line {
    Point a;
    Point b;
};

// ============================================================================
// 1. Scalars & bool
// ============================================================================
TEST(ScalarTest, Integers) {
    EXPECT_EQ(toJson(42), "42");
    EXPECT_EQ(toJson(-7), "-7");
    EXPECT_EQ(toJson(0), "0");
}
TEST(ScalarTest, Bool) {
    EXPECT_EQ(toJson(true), "true");
    EXPECT_EQ(toJson(false), "false");
}
TEST(ScalarTest, Doubles) {
    EXPECT_EQ(toJson(3.5), "3.5");
    EXPECT_EQ(toJson(0.5), "0.5");
    EXPECT_EQ(toJson(-0.25), "-0.25");
    EXPECT_EQ(toJson(1.0), "1");  // shortest round-trip drops the .0
}
TEST(ScalarTest, DoubleScientific) {
    EXPECT_EQ(toJson(1000000.0), "1e+06");  // documents to_chars shortest form
}

// ============================================================================
// 2. Strings, escaping, const char*
//    NOTE: a string LITERAL is const char[N], not const char*, so
//    toJson("lit") does NOT hit the CString path — bind to const char* first.
// ============================================================================
TEST(StringTest, Plain) {
    EXPECT_EQ(toJson(std::string{"hi"}), R"("hi")");
    EXPECT_EQ(toJson(std::string_view{"view"}), R"("view")");
    EXPECT_EQ(toJson(std::string{}), R"("")");  // empty string != null
}
TEST(StringTest, Escaping) {
    // input bytes:  a " b \ c <newline> <tab>
    EXPECT_EQ(toJson(std::string{"a\"b\\c\n\t"}), R"("a\"b\\c\n\t")");
}
TEST(StringTest, CString) {
    const char* s = "lit";
    EXPECT_EQ(toJson(s), R"("lit")");
}
TEST(StringTest, CStringNull) {
    const char* s = nullptr;
    EXPECT_EQ(toJson(s), "null");  // null char* -> null (the segfault case, guarded)
}

// ============================================================================
// 3. Enums  (name when known, underlying int when not)
// ============================================================================
TEST(EnumTest, Unscoped) { EXPECT_EQ(toJson(Green), R"("Green")"); }
TEST(EnumTest, Scoped) { EXPECT_EQ(toJson(Status::Cleared), R"("Cleared")"); }
TEST(EnumTest, Unknown) { EXPECT_EQ(toJson(static_cast<Status>(99)), "99"); }

// ============================================================================
// 4. Objects (flat + nested)
// ============================================================================
TEST(ObjectTest, Flat) { EXPECT_EQ(toJson(Point{1, 2}), R"({"x":1,"y":2})"); }
TEST(ObjectTest, Nested) { EXPECT_EQ(toJson(Line{{1, 2}, {3, 4}}), R"({"a":{"x":1,"y":2},"b":{"x":3,"y":4}})"); }

// ============================================================================
// 5. Sequences (vector, set) -> JSON array
// ============================================================================
TEST(RangeTest, VectorInt) { EXPECT_EQ(toJson(std::vector<int>{1, 2, 3}), "[1,2,3]"); }
TEST(RangeTest, VectorObj) {
    EXPECT_EQ(toJson(std::vector<Point>{{1, 2}, {3, 4}}), R"([{"x":1,"y":2},{"x":3,"y":4}])");
}
TEST(RangeTest, Set) { EXPECT_EQ(toJson(std::set<int>{3, 1, 2}), "[1,2,3]"); }  // sorted
TEST(RangeTest, Empty) { EXPECT_EQ(toJson(std::vector<int>{}), "[]"); }

// ============================================================================
// 6. Maps
//    string key    -> JSON object
//    non-string key -> array of {"key":..,"value":..} (lossless round-trip)
// ============================================================================
TEST(MapTest, StringKeyObject) {
    EXPECT_EQ(toJson(std::map<std::string, int>{{"a", 1}, {"b", 2}}), R"({"a":1,"b":2})");
}
TEST(MapTest, IntKeyEntryArray) {
    EXPECT_EQ(toJson(std::map<int, int>{{1, 10}, {2, 20}}), R"([{"key":1,"value":10},{"key":2,"value":20}])");
}
TEST(MapTest, EmptyStringKey) {
    EXPECT_EQ(toJson(std::map<std::string, int>{}), "{}");  // object shape
}
TEST(MapTest, EmptyIntKey) {
    EXPECT_EQ(toJson(std::map<int, int>{}), "[]");  // array shape
}
TEST(MapTest, EnumKeyEntryArray) {
    // enum key is NOT StringKey -> entry array, key serialized as its name
    EXPECT_EQ(toJson(std::map<Status, double>{{Status::Pending, 1.0}}), R"([{"key":"Pending","value":1}])");
}

// ============================================================================
// 7. Nullable (optional, smart pointers, raw pointer)
// ============================================================================
TEST(NullableTest, OptionalEngaged) { EXPECT_EQ(toJson(std::optional<int>{5}), "5"); }
TEST(NullableTest, OptionalEmpty) { EXPECT_EQ(toJson(std::optional<int>{}), "null"); }
TEST(NullableTest, OptionalObject) { EXPECT_EQ(toJson(std::optional<Point>{Point{1, 2}}), R"({"x":1,"y":2})"); }
TEST(NullableTest, UniquePtr) { EXPECT_EQ(toJson(std::make_unique<int>(7)), "7"); }
TEST(NullableTest, UniquePtrNull) {
    std::unique_ptr<int> p;
    EXPECT_EQ(toJson(p), "null");
}
TEST(NullableTest, SharedPtrObject) { EXPECT_EQ(toJson(std::make_shared<Point>(Point{1, 2})), R"({"x":1,"y":2})"); }
TEST(NullableTest, RawPointer) {
    int n = 3;
    EXPECT_EQ(toJson(&n), "3");
}

// ============================================================================
// 8. Tuple / pair / array -> JSON array
//    (standalone pair is a 2-array, NOT a {"key","value"} entry)
// ============================================================================
TEST(TupleTest, Mixed) { EXPECT_EQ(toJson(std::tuple<int, std::string, double>{1, "two", 3.5}), R"([1,"two",3.5])"); }
TEST(TupleTest, Pair) { EXPECT_EQ(toJson(std::pair<int, std::string>{7, "seven"}), R"([7,"seven"])"); }
TEST(TupleTest, Array) { EXPECT_EQ(toJson(std::array<int, 3>{9, 8, 7}), "[9,8,7]"); }

// ============================================================================
// 9. Variant (index-tagged)  --- ASSUMES the `,"value":` colon fix ---
// ============================================================================
using Var = std::variant<int, std::string, Point>;
TEST(VariantTest, IntAlternative) { EXPECT_EQ(toJson(Var{42}), R"({"index":0,"value":42})"); }
TEST(VariantTest, StringAlternative) { EXPECT_EQ(toJson(Var{std::string{"hi"}}), R"({"index":1,"value":"hi"})"); }
TEST(VariantTest, StructAlternative) { EXPECT_EQ(toJson(Var{Point{1, 2}}), R"({"index":2,"value":{"x":1,"y":2}})"); }

struct HasChar {
    char c;
};
TEST(CharTest, CharAsString) {
    EXPECT_EQ(toJson(HasChar{'A'}), R"({"c":"A"})");  // if you make char a string
}

// ============================================================================
// 11. Integration — a realistic composite exercising many paths at once
// ============================================================================
struct Trade {
    std::string id;
    Status status;
    std::optional<double> price;  // empty -> null
    std::vector<int> legs;
    std::map<std::string, double> weights;  // string key -> object
};
TEST(IntegrationTest, CompositeStruct) {
    Trade t{
        .id = "T1",
        .status = Status::Cleared,
        .price = std::nullopt,
        .legs = {1, 2, 3},
        .weights = {{"a", 0.5}, {"b", 0.25}},
    };
    EXPECT_EQ(toJson(t), R"({"id":"T1","status":"Cleared","price":null,"legs":[1,2,3],"weights":{"a":0.5,"b":0.25}})");
}
