#pragma once

/*This is a reflection/Concept based Json Serializer it requires C++26 -freflection capabilities on GCC16*/
#include <meta>

#include <charconv>
#include <concepts>
#include <cstddef>
#include <ranges>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

#include "utils/string.hpp"

namespace json::serializer {

namespace impl {

using utils::string::writeEscapedString;

void writeSeparator(std::string& out, bool& sentinel) {
    if (!sentinel)
        out.push_back(',');
    else
        sentinel = false;
}

template <class T>
void writeArithmetic(std::string& out, const T& value) {
    if constexpr (std::is_same_v<T, bool>) {
        out += value ? "true" : "false";
    } else if constexpr (std::is_same_v<T, char>) {
        out.push_back('"');
        out.push_back(value);
        out.push_back('"');
    } else if constexpr (std::is_arithmetic_v<T>) {
        char buf[64];
        auto [ptr, ec] = std::to_chars(buf, buf + sizeof buf, value);
        out.append(buf, ptr);
    } else {
        static_assert(false, "writeArithmetic: unsupported type");
    }
}

template <class T>
void writeValue(std::string& out, const T& value);

// Enumerators
template <class E>
void writeEnum(std::string& out, const E& value) {
    constexpr static auto enums = std::define_static_array(std::meta::enumerators_of(^^E));
    template for (constexpr auto e : enums) {
        if (value == [:e:]) {
            writeEscapedString(out, std::meta::identifier_of(e));
            return;
        }
    }
    writeValue(out, std::to_underlying(value));
}

// Objects
template <class T>
void writeObject(std::string& out, const T& value) {
    bool first = true;
    out.push_back('{');
    constexpr static auto members =
        std::define_static_array(std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::unchecked()));
    template for (constexpr auto m : members) {
        if constexpr (std::meta::has_identifier(m)) {
            writeSeparator(out, first);
            writeEscapedString(out, std::meta::identifier_of(m));
            out.push_back(':');
            writeValue(out, value.[:m:]);
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
void writeRange(std::string& out, const T& value) {
    bool first = true;
    out.push_back('[');
    for (const auto& v : value) {
        writeSeparator(out, first);
        writeValue(out, v);
    }
    out.push_back(']');
}

template <class T>
    requires MapLike<T> && StringKey<typename T::key_type>
void writeRange(std::string& out, const T& value) {
    bool first = true;
    out.push_back('{');
    for (const auto& p : value) {
        writeSeparator(out, first);
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
void writeRange(std::string& out, const T& value) {
    bool first = true;
    out.push_back('[');
    for (const auto& p : value) {
        writeSeparator(out, first);
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
void writeNullable(std::string& out, const T& value) {
    if (value)
        writeValue(out, *value);
    else
        out += "null";
}

// String Like
template <class T>
concept StringLike = std::is_same_v<T, std::string> || std::is_same_v<T, std::string_view>;

// CStringLike
template <class T>
concept CStringLike = std::is_same_v<T, const char*> || std::is_same_v<T, char*>;

template <CStringLike T>
void writeCString(std::string& out, const T& value) {
    if (value)
        writeEscapedString(out, value);
    else
        out += "null";
}

// Tuple,Pair
template <class T>
concept TupleLike = requires { std::tuple_size<T>::value; };

template <class T>
    requires TupleLike<T> && (!std::ranges::range<T>)
void writeTuple(std::string& out, const T& value) {
    out += '[';
    std::apply(
        [&](const auto&... v) {
            bool first = true;
            ((writeSeparator(out, first), writeValue(out, v)), ...);
        },
        value);
    out += ']';
}

// Variant
template <class T>
concept VariantLike = requires { std::variant_size<T>::value; };

template <VariantLike T>
void writeVariant(std::string& out, const T& value) {
    out += "{\"index\":";
    std::size_t index = value.index();
    if (index == std::variant_npos) {
        out += "null,\"value\":null}";
        return;
    } else {
        out += std::to_string(index);
        out += ",\"value\":";
        std::visit([&](const auto& v) { writeValue(out, v); }, value);
        out += '}';
    }
}
// Dispatcher
template <class T>
void writeValue(std::string& out, const T& value) {
    /*The order in the if chain is significant because they are checks are not mutually exclusive
    For example char* is both CStringLike and Nullable it will be assigned in the first*/
    if constexpr (std::is_arithmetic_v<T>) {
        writeArithmetic(out, value);
    } else if constexpr (std::is_enum_v<T>) {
        writeEnum(out, value);
    } else if constexpr (StringLike<T>) {
        writeEscapedString(out, value);
    } else if constexpr (CStringLike<T>) {
        writeCString(out, value);
    } else if constexpr (Nullable<T>) {
        writeNullable(out, value);
    } else if constexpr (std::is_scalar_v<T>) {
        static_assert(false, "pointer not handled for now");
        // TODO(handle non smart pointer)
    } else if constexpr (std::ranges::range<T>) {
        writeRange(out, value);
    } else if constexpr (VariantLike<T>) {
        writeVariant(out, value);
    } else if constexpr (TupleLike<T>) {
        writeTuple(out, value);
    } else if constexpr (std::is_class_v<T>) {
        writeObject(out, value);
    } else {
        static_assert(false, "unsupported value type");
    }
}
}  // namespace impl

template <class T>
std::string toJson(const T& value) {
    std::string out;
    impl::writeValue(out, value);
    return out;
}

}  // namespace json::serializer
