#pragma once

/*This is a reflection/Concept based Json Serializer it requires C++26 -freflection capabilities on GCC16*/
#include <meta>

#include <cmath>
#include <concepts>
#include <limits>
#include <map>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

#include "cpputils/error.hpp"
#include "json/json.hpp"

namespace json::reader {
struct Json {
    std::variant<std::nullptr_t, bool, double, std::string, std::vector<Json>, std::map<std::string, Json>> value;
};

namespace impl {

using namespace json::impl;

template <class T>
T readValue(const Json& node);

template <class T>
T readBool(const Json& node) {
    const bool* p = std::get_if<bool>(&node.value);
    cpputils::ensure(p, "A bool was expected");
    return static_cast<T>(*p);
}
// Arithmetic
template <std::integral T>
T checkedToInt(double d) {
    cpputils::ensure(std::isfinite(d), "number is NaN or infinte");
    cpputils::ensure(d == std::trunc(d), "number had a fractional part for an integer field");

    if constexpr (std::is_signed_v<T>) {
        constexpr double lo = -std::ldexp(1.0, std::numeric_limits<T>::digits);
        constexpr double hi = std::ldexp(1.0, std::numeric_limits<T>::digits);
        cpputils::ensure(d >= lo && d < hi, "number out of range for target integer");
    } else {
        constexpr double hi = std::ldexp(1.0, std::numeric_limits<T>::digits);
        cpputils::ensure(d >= 0.0 && d < hi, "number out of range for target integer");
    }
    return static_cast<T>(d);
}

template <class T>
T readNumber(const Json& node) {
    const double* p = std::get_if<double>(&node.value);
    cpputils::ensure(p, "A Number was expected");
    if constexpr (std::integral<T>) {
        return checkedToInt<T>(*p);
    } else {
        return static_cast<T>(*p);
    }
}

template <class T>
T readString(const Json& node) {
    const std::string* p = std::get_if<std::string>(&node.value);
    cpputils::ensure(p, "A String was expected");
    return static_cast<T>(*p);
}

template <class T>
T readNullable(const Json& node) {}

template <class T>
T readMap(const Json& node) {}

template <class T>
T readRange(const Json& node) {}

template <class T>
T readObject(const Json& node) {
    T out{};
    constexpr static auto members =
        std::define_static_array(std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::unchecked()));
    const auto* obj = std::get_if<std::map<std::string, Json>>(&node.value);
    cpputils::ensure(obj, "An Object was Expected");
    template for (constexpr auto m : members) {
        if constexpr (std::meta::has_identifier(m)) {
            using M = typename[:std::meta::type_of(m):];
            auto it = obj->find(std::string{std::meta::identifier_of(m)});
            cpputils::ensure(it != obj->end(), "readObject:missing key");
            out.[:m:] = readValue<M>(it->second);
        }
    }
    return out;
}

template <class T>
concept HasCustomReader = requires(const Json& json) {
    { readFromDom(json, std::type_identity<T>{}) } -> std::convertible_to<T>;
};

template <class T>
T reflectionReadFromNode(const Json& node) {
    if constexpr (std::is_same_v<T, bool>)
        return readBool<T>(node);
    else if constexpr (std::is_arithmetic_v<T>)
        return readNumber<T>(node);
    else if constexpr (StringLike<T>)
        return readString<T>(node);
    else if constexpr (Nullable<T>)
        return readNullable<T>(node);
    else if constexpr (MapLike<T>)
        return readMap<T>(node);
    else if constexpr (Range<T>)
        return readRange<T>(node);
    else if constexpr (std::is_class_v<T>)
        return readObject<T>(node);
    else
        static_assert(false, "Unsupported Type");
}

template <class T>
T readValue(const Json& node) {
    if constexpr (HasCustomReader<T>) {
        return readFromDom(node, std::type_identity<T>{});
    } else {
        return reflectionReadFromNode<T>(node);
    }
}
}  // namespace impl

template <class T>
T fromJson(const Json& root) {
    return impl::readValue<T>(root);
}
}  // namespace json::reader
