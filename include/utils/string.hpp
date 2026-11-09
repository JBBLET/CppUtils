#pragma once
#include <string>
#include <string_view>

namespace utils::string {

namespace impl {
inline std::string otherEscapeChar(char c) {
    const char hexDigits[] = "0123456789ABCDEF";
    unsigned char uc = static_cast<unsigned char>(c);

    if (uc < 0x20) {
        std::string out = "\\u0000";
        out[4] = hexDigits[(uc >> 4) & 0x0F];
        out[5] = hexDigits[uc & 0x0F];
        return out;
    }
    return std::string(1, c);
}

inline std::string escapeChar(char c) {
    switch (c) {
        case '"':
            return "\\\"";
        case '\\':
            return "\\\\";
        case '\n':
            return "\\n";
            break;
        case '\t':
            return "\\t";
        case '\r':
            return "\\r";
        case '\b':
            return "\\b";
        case '\f':
            return "\\f";
        default:
            return otherEscapeChar(
                c);  // Would move be better here ? but then need to include full utility for one move call
    }
}
}  // namespace impl
inline void writeEscapedString(std::string& out, std::string_view s) {
    out += '"';
    for (char c : s) {
        out += impl::escapeChar(c);
    }
    out += '"';
}
}  // namespace utils::string
