#include <array>
#include <cstdio>
#include <map>
#include <string>

#include "json/jsonReader.hpp"  // adjust to your path

using json::reader::Json;

struct User {
    std::string name;
    int id;
};
struct TestDom {
    double leg;
    bool received;
    int id;
    std::string receiver;
    User user;
};

void printObject(const TestDom& obj) {
    std::printf("Leg:%.2f, type:%s \n", obj.leg, typeid(obj.leg).name());
    std::printf("Received:%d, type:%s \n", obj.received, typeid(obj.received).name());
    std::printf("ID:%d, type:%s \n", obj.id, typeid(obj.id).name());
    std::printf("Receiver:%s, type:%s \n", obj.receiver.c_str(), typeid(obj.receiver).name());
    std::printf("user:%s, type:%s \n", obj.user.name.c_str(), typeid(obj.user.name).name());
    std::printf("user Id:%d, type:%s \n", obj.user.id, typeid(obj.user.id).name());
}
int main() {
    Json jsonDOMTest{std::map<std::string, Json>{
        {"leg", Json{3.14}},
        {"received", Json{true}},
        {"id", Json{double{1}}},
        {"receiver", Json{"group"}},
        {"user", Json{std::map<std::string, Json>{{"name", Json{"test_name"}}, {"id", Json{double{1}}}}}}
    }};
    TestDom test = json::reader::fromJson<TestDom>(jsonDOMTest);
    printObject(test);
}
