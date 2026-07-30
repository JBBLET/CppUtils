#pragma once

#include <cstdint>
#include <istream>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "cpputils/error.hpp"
#include "csv/csvDialectConstants.hpp"

namespace cpputils::csv {

struct Reader {
    std::istream& stream;
    char separatorCharacter{DEFAULT_SEPARATOR};
    char quoteCharacter{DEFAULT_QUOTE_CHARACTER};
    char endOfLineCharacter{DEFAULT_LINE_END};
    char escapeCharacter{DEFAULT_ESCAPE_CHARACTER};
    char noQuoteCharacter{NO_QUOTE_CHARACTER};
    char noEscapeCharacter{NO_ESCAPE_CHARACTER};
};

class CSVReader {
 public:
    CSVReader(std::istream& stream, char separatorCharacter = DEFAULT_SEPARATOR,
              char quoteCharacter = DEFAULT_QUOTE_CHARACTER, char endOfLineCharacter = DEFAULT_LINE_END,
              char escapeCharacter = DEFAULT_ESCAPE_CHARACTER, char noQuoteCharacter = NO_QUOTE_CHARACTER,
              char noEscapeCharacter = NO_ESCAPE_CHARACTER);
    explicit CSVReader(Reader reader);

    auto readNext() -> std::optional<std::vector<std::string>>;
    auto readAll() -> std::vector<std::vector<std::string>>;
    auto peek() -> std::optional<std::vector<std::string>>;
    auto skip(int n) -> void;
    auto close() -> void;
    auto getLinesRead() -> int64_t;
    auto getRecordsRead() -> int64_t;

 private:
    Reader reader_;
    std::vector<std::unordered_map<std::string, std::string>> rows_;
    std::vector<std::string> headers_;
    std::optional<std::vector<std::string>> peeked_;
    int64_t linesRead_ = 0;
    int64_t recordsRead_ = 0;
};

// Trim leading/trailing whitespace. Only applied to *unquoted* fields; a quoted
// field keeps its whitespace because "  x  " means the spaces are intentional.
static auto sanitizeCell(std::string cell) -> std::string {
    const auto* whiteSpace = " \t\v\r\f\n";
    auto start = cell.find_first_not_of(whiteSpace);
    if (std::string::npos == start) {
        return std::string{""};
    }
    auto end = cell.find_last_not_of(whiteSpace);
    return cell.substr(start, (end - start + 1));
}

static auto readRecord(std::istream& file, std::vector<std::string>& record, char separatorCharacter,
                       char quoteCharacter, char endOfLineCharacter, [[maybe_unused]] char escapeCharacter,
                       int64_t& linesRead) -> bool {
    record.clear();
    auto cell = std::string{};
    auto inQuotes = false;
    auto fieldWasQuoted = false;
    auto sawInput = false;
    auto c = char{};

    auto pushField = [&] {
        record.push_back(fieldWasQuoted ? cell : sanitizeCell(cell));
        cell.clear();
        fieldWasQuoted = false;
    };

    while (file.get(c)) {
        sawInput = true;
        if (inQuotes) {
            if (c == quoteCharacter) {
                if (file.peek() == quoteCharacter) {  // "" -> literal quote
                    cell += quoteCharacter;
                    file.get();
                } else {
                    inQuotes = false;  // closing quote
                }
            } else {
                cell += c;  // any byte, including embedded , and \n, is data here
            }
        } else {
            if (c == quoteCharacter) {
                inQuotes = true;
                fieldWasQuoted = true;
            } else if (c == separatorCharacter) {
                pushField();
            } else if (c == '\r') {
                // swallow CR outside quotes so \r\n and \r behave like \n
            } else if (c == endOfLineCharacter) {
                ++linesRead;
                pushField();
                return true;  // end of record
            } else {
                cell += c;
            }
        }
    }
    cpputils::ensure(!inQuotes, "Unterminated quoted field at end of file");
    ++linesRead;
    if (!sawInput) {
        return false;  // clean EOF: no more records
    }
    pushField();  // final record with no trailing newline
    return true;
}

inline CSVReader::CSVReader(std::istream& stream, char separatorCharacter, char quoteCharacter, char endOfLineCharacter,
                            char escapeCharacter, char noQuoteCharacter, char noEscapeCharacter)
    : reader_{Reader{.stream = stream,
                     .separatorCharacter = separatorCharacter,
                     .quoteCharacter = quoteCharacter,
                     .endOfLineCharacter = endOfLineCharacter,
                     .escapeCharacter = escapeCharacter,
                     .noQuoteCharacter = noQuoteCharacter,
                     .noEscapeCharacter = noEscapeCharacter}} {}

inline CSVReader::CSVReader(Reader reader) : reader_{reader} {}

inline auto CSVReader::readNext() -> std::optional<std::vector<std::string>> {
    if (peeked_) {
        auto out = std::exchange(peeked_, std::nullopt);
        ++recordsRead_;
        return out;
    }
    std::vector<std::string> line;
    if (readRecord(reader_.stream,
                   line,
                   reader_.separatorCharacter,
                   reader_.quoteCharacter,
                   reader_.endOfLineCharacter,
                   reader_.escapeCharacter,
                   linesRead_)) {
        ++recordsRead_;
        return line;
    }
    return std::nullopt;
}

inline auto CSVReader::readAll() -> std::vector<std::vector<std::string>> {
    std::vector<std::vector<std::string>> out;
    while (auto row = readNext()) out.push_back(std::move(*row));
    return out;
}

inline auto CSVReader::peek() -> std::optional<std::vector<std::string>> {
    peeked_ = readNext();
    return peeked_;
}

inline auto CSVReader::skip(int n) -> void {
    for (auto i = 0; i < n; i++) {
        readNext();
    }
}

inline auto CSVReader::close() -> void { reader_.stream.clear(); }

inline auto CSVReader::getLinesRead() -> int64_t { return linesRead_; }

inline auto CSVReader::getRecordsRead() -> int64_t { return recordsRead_; }
}  // namespace cpputils::csv
