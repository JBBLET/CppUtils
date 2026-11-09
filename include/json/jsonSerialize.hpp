#pragma once

#if !defined(__cpp_impl_reflection)
#error "json/jsonSerialize.hpp requires C++26 reflection (-freflection on GCC 16+)"
#endif

#include <meta>

#include <charconv>
#include <ranges>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "utils/string.hpp"

namespace json::serializer {

template <class T>
void writeArithmetic(std::string& out, const T& v) {
    if constexpr (std::is_same_v<T, bool>) {
        out += v ? "true" : "false";
    } else if constexpr (std::is_arithmetic_v<T>) {
        char buf[32];
        auto [ptr, ec] = std::to_chars(buf, buf + sizeof buf, v);
        out.append(buf, ptr);
    } else {
        static_assert(false, "writeArithmetic: unsupported type");
    }
}
template <class T>
void writeValue(std::string& out, const T& v);

// Enumerators
template <class E>
void writeEnum(std::string& out, const E& obj) {
    constexpr static auto values = std::define_static_array(std::meta::enumerators_of(^^E));
    template for (constexpr auto v : values) {
        if (obj == [:v:]) {
            writeEscapedString(out, std::meta::identifier_of(v));
            return;
        }
    }
    writeValue(out, std::to_underlying(obj));
}

// Objects
template <class T>
void writeObject(std::string& out, const T& obj) {
    bool first = true;
    out.push_back('{');
    constexpr static auto members =
        std::define_static_array(std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::unchecked()));
    template for (constexpr auto m : members) {
        if constexpr (std::meta::has_identifier(m)) {
            out += first ? "" : ",";
            first = false;
            writeEscapedString(out, std::meta::identifier_of(m));
            out.push_back(':');
            writeValue(out, obj.[:m:]);
        }
    }
    out.push_back('}');
}

// Containers
template <class T>
concept Range = std::ranges::range<T>;

template <class T>
concept MapLike = Range<T> && requires {
    typename T::key_type;
    typename T::mapped_type;
} && (std::is_same_v<typename T::value_type, std::pair<const typename T::key_type, typename T::mapped_type>>);
// TODO(Include flatMaps)

template <class K>
concept StringKey = std::constructible_from<std::string, K>;

template <Range T>
void writeRange(std::string& out, const T& obj) {
    bool first = true;
    out.push_back('[');
    for (const auto& m : obj) {
        out += first ? "" : ",";
        first = false;
        writeValue(out, m);
    }
    out.push_back(']');
}

template <class T>
    requires MapLike<T> && StringKey<typename T::key_type>
void writeRange(std::string& out, const T& obj) {
    bool first = true;
    out.push_back('{');
    for (const auto& p : obj) {
        out += first ? "" : ",";
        first = false;
        if constexpr (std::convertible_to<const typename T::key_type&, std::string_view>) {
            writeEscapedString(out, std::string_view{p.first});  // if key is already a string save an allocation
        } else {
            writeEscapedString(out, std::string{p.first});
        }
        out.push_back(':');
        writeValue(out, p.second);
    }
    out.push_back('}');
}

template <class T>
    requires MapLike<T>
void writeRange(std::string& out, const T& obj) {
    bool first = true;
    out.push_back('[');
    for (const auto& p : obj) {
        out += first ? "" : ",";
        first = false;
        out += "{\"key\":";
        writeValue(out, p.first);
        out += ",\"value\":";
        writeValue(out, p.second);
        out.push_back('}');
    }
    out.push_back(']');
}
// Nullable
template <class T>
concept Nullable = requires(const T& t) {
    static_cast<bool>(t);  // has a value (is non null)
    *t;                    // dereferenceable
};

template <Nullable T>
void writeNullable(std::string& out, const T& obj) {
    if (obj)
        writeValue(out, *obj);
    else
        out += "null";
}

// Dispatcher
template <class T>
void writeValue(std::string& out, const T& v) {
    if constexpr (std::is_arithmetic_v<T>) {
        writeArithmetic(out, v);
    } else if constexpr (std::is_enum_v<T>) {
        writeEnum(out, v);
    } else if constexpr (std::is_same_v<T, std::string> || std::is_same_v<T, std::string_view> ||
                         std::is_same_v<T, const char*>) {
        writeEscapedString(out, v);
    } else if constexpr (Nullable<T>) {
        writeNullable(out, v);
    } else if constexpr (std::is_scalar_v<T>) {
        static_assert(false, "pointer not handled for now");
        // TODO(handle pointer)
    } else if constexpr (std::ranges::range<T>) {
        writeRange(out, v);
    } else if constexpr (std::is_class_v<T>) {
        writeObject(out, v);
    } else {
        static_assert(false, "unsupported value type");
    }
}

template <class T>
std::string to_json(const T& obj) {
    std::string out;
    writeObject(out, obj);
    return out;
}

}  // namespace json::serializer
