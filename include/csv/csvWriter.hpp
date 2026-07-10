#pragma once

#include <cstdint>
#include <exception>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

#include "csv/csvDialectConstants.hpp"

struct Writer {
    std::ostream& stream;
    char separatorCharacter{DEFAULT_SEPARATOR};
    char quoteCharacter{DEFAULT_QUOTE_CHARACTER};
    char escapeCharacter{DEFAULT_ESCAPE_CHARACTER};  // prefixed before an embedded quote -> "" by default
    std::string lineEnd{DEFAULT_LINE_END_STR};
};

class CSVWriter {
 public:
    CSVWriter(std::ostream& stream, char separatorCharacter = DEFAULT_SEPARATOR,
              char quoteCharacter = DEFAULT_QUOTE_CHARACTER, char escapeCharacter = DEFAULT_ESCAPE_CHARACTER,
              std::string lineEnd = DEFAULT_LINE_END_STR);
    explicit CSVWriter(Writer writer);

    auto writeNext(const std::vector<std::string>& record, bool applyQuotesToAll = true) -> void;
    auto writeAll(const std::vector<std::vector<std::string>>& records, bool applyQuotesToAll = true) -> void;

    auto flush() -> void;
    auto close() -> void;
    auto checkError() -> bool;  // stream in a fail state?
    auto getException() const -> std::optional<std::string>;

    auto getRecordsWritten() const -> int64_t;
    auto getLinesWritten() const -> int64_t;

 private:
    Writer writer_;
    int64_t recordsWritten_ = 0;
    int64_t linesWritten_ = 0;
    std::optional<std::string> lastError_;
};

inline auto fieldNeedsQuoting(std::string_view field, char separator, char quoteChar, std::string_view lineEnd)
    -> bool {
    return field.find(separator) != std::string_view::npos || field.find(quoteChar) != std::string_view::npos ||
           field.find(lineEnd) != std::string_view::npos || field.find('\n') != std::string_view::npos ||
           field.find('\r') != std::string_view::npos;
}
inline auto writeField(std::ostream& stream, std::string_view field, char separator, char quoteChar, char escapeChar,
                       std::string_view lineEnd, bool applyQuotesToAll) -> void {
    auto quotingEnabled = quoteChar != '\0';
    auto quoted = quotingEnabled && (applyQuotesToAll || fieldNeedsQuoting(field, separator, quoteChar, lineEnd));
    if (!quoted) {
        stream << field;
        return;
    }
    stream << quoteChar;
    for (char c : field) {
        if (c == quoteChar) stream << escapeChar;  // "" doubles the quote by default
        stream << c;
    }
    stream << quoteChar;
}

inline CSVWriter::CSVWriter(std::ostream& stream, char separatorCharacter, char quoteCharacter, char escapeCharacter,
                            std::string lineEnd)
    : writer_{.stream = stream,
              .separatorCharacter = separatorCharacter,
              .quoteCharacter = quoteCharacter,
              .escapeCharacter = escapeCharacter,
              .lineEnd = lineEnd} {}

inline CSVWriter::CSVWriter(Writer writer) : writer_{writer} {}

inline auto CSVWriter::writeNext(const std::vector<std::string>& record, bool applyQuotesToAll) -> void {
    try {
        for (std::size_t i = 0; i < record.size(); ++i) {
            if (i != 0) writer_.stream << writer_.separatorCharacter;
            writeField(writer_.stream,
                       record[i],
                       writer_.separatorCharacter,
                       writer_.quoteCharacter,
                       writer_.escapeCharacter,
                       writer_.lineEnd,
                       applyQuotesToAll);
            // Any physical line break embedded in a quoted field adds an output line.
            for (char c : record[i]) {
                if (c == '\n') ++linesWritten_;
            }
        }
        writer_.stream << writer_.lineEnd;
        ++recordsWritten_;
        ++linesWritten_;
    } catch (const std::exception& e) {
        lastError_ = e.what();
    }
}

inline auto CSVWriter::writeAll(const std::vector<std::vector<std::string>>& records, bool applyQuotesToAll) -> void {
    for (const auto& record : records) writeNext(record, applyQuotesToAll);
}

inline auto CSVWriter::flush() -> void { writer_.stream.flush(); }

inline auto CSVWriter::close() -> void { writer_.stream.flush(); }

inline auto CSVWriter::checkError() -> bool { return !static_cast<bool>(writer_.stream); }

inline auto CSVWriter::getException() const -> std::optional<std::string> { return lastError_; }

inline auto CSVWriter::getRecordsWritten() const -> int64_t { return recordsWritten_; }

inline auto CSVWriter::getLinesWritten() const -> int64_t { return linesWritten_; }
