#include <cstdio>
#include <map>

#include "json/jsonSerialize.hpp"  // adjust to your path

enum Side { Buy, Sell };                          // unscoped: 0, 1
enum class Status { Pending, Cleared, Settled };  // scoped:   0, 1, 2

struct Trade {
    Side side;                                  // unscoped enum member
    Status status;                              // scoped enum member
    std::map<Status, double> notionalByStatus;  // enum KEY (non-string → entry array)
};

int main() {
    Trade t{
        .side = Sell,
        .status = Status::Cleared,
        .notionalByStatus = {{Status::Pending, 1.0}, {Status::Settled, 3.5}},
    };
    std::puts(json::serializer::to_json(t).c_str());

    Trade u{
        .side = Buy,
        .status = static_cast<Status>(99),  // out of range → <unknown>
        .notionalByStatus = {},
    };
    std::puts(json::serializer::to_json(u).c_str());
}
