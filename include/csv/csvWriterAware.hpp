#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "csv/csvWriter.hpp"

using Row = std::unordered_map<std::string, std::string>;  // same alias as csvReaderAware.hpp

class CSVWriterHeaderAware {
 public:
    // Writes the header row immediately on construction.
    CSVWriterHeaderAware(CSVWriter writer, std::vector<std::string> headers);

    auto writeMap(const Row& row, bool applyQuotesToAll = true) -> void;  // ordered by headers_
    auto writeAllMaps(const std::vector<Row>& rows) -> void;

    template <class T>
    auto writeObject(const T& obj, const std::function<Row(const T&)>& transformFunc) -> void;
    template <class T>
    auto writeAllObjects(const std::vector<T>& objs, const std::function<Row(const T&)>& transformFunc) -> void;

    auto headers() const -> const std::vector<std::string>&;

 private:
    CSVWriter writer_;
    std::vector<std::string> headers_;
};

inline CSVWriterHeaderAware::CSVWriterHeaderAware(CSVWriter writer, std::vector<std::string> headers)
    : writer_{writer}, headers_{std::move(headers)} {}

inline auto CSVWriterHeaderAware::writeMap(const Row& row, bool applyQuotesToAll) -> void {
    auto selected = std::vector<std::string>{};
    selected.reserve(headers_.size());
    for (const auto& column : headers_) {
        selected.push_back(row.at(column));
    }
    writer_.writeNext(selected, applyQuotesToAll);
}

inline auto CSVWriterHeaderAware::writeAllMaps(const std::vector<Row>& rows) -> void {
    for (const Row& row : rows) {
        writeMap(row);
    }
}

template <class T>
inline auto CSVWriterHeaderAware::writeObject(const T& obj, const std::function<Row(const T&)>& transformFunc) -> void {
    writeMap(*transformFunc(obj));
}

template <class T>
inline auto CSVWriterHeaderAware::writeAllObjects(const std::vector<T>& objs,
                                                  const std::function<Row(const T&)>& transformFunc) -> void {
    for (const T& obj : objs) {
        writeMap(*transformFunc(obj));
    }
}

inline auto CSVWriterHeaderAware::headers() const -> const std::vector<std::string>& { return headers_; }
