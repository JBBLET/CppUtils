#pragma once

#include <algorithm>
#include <format>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "csv/csvReader.hpp"

using Row = std::unordered_map<std::string, std::string>;

class CSVReaderHeaderAware {
 public:
    explicit CSVReaderHeaderAware(CSVReader reader);

    auto readMap() -> std::optional<Row>;
    auto readAllMaps() -> std::vector<Row>;

    auto readNext(const std::vector<std::string>& columns) -> std::optional<std::vector<std::string>>;

    template <class T>
    auto readNextObject(const std::function<T(Row)>& transformFunc) -> std::optional<T>;
    template <class T>
    auto readAllObjects(const std::function<T(Row)>& transformFunc) -> std::vector<T>;
    auto headers() const -> const std::vector<std::string>&;

 private:
    CSVReader reader_;
    std::vector<std::string> headers_;
};

inline CSVReaderHeaderAware::CSVReaderHeaderAware(CSVReader reader) : reader_{std::move(reader)} {
    auto first = reader_.readNext();
    ensure(first.has_value(), "CSV has no header row");
    headers_ = std::move(*first);
}

inline auto CSVReaderHeaderAware::readMap() -> std::optional<Row> {
    auto row = reader_.readNext();
    if (!row) return std::nullopt;
    ensure(row->size() == headers_.size(), "Row has {} fields but header has {}", row->size(), headers_.size());
    std::unordered_map<std::string, std::string> out;
    for (std::size_t i = 0; i < headers_.size(); ++i) out.emplace(headers_[i], std::move((*row)[i]));
    return out;
}

inline auto CSVReaderHeaderAware::readAllMaps() -> std::vector<Row> {
    std::vector<Row> out;
    while (auto row = readMap()) out.push_back(std::move(*row));
    return out;
}
inline auto CSVReaderHeaderAware::readNext(const std::vector<std::string>& columns)
    -> std::optional<std::vector<std::string>> {
    auto row = reader_.readNext();
    if (!row) return std::nullopt;
    std::vector<std::string> out;
    out.reserve(columns.size());
    for (const auto& name : columns) {
        auto it = std::ranges::find(headers_, name);
        ensure(it != headers_.end(), "Unknown column '{}'", name);
        out.push_back((*row)[std::distance(headers_.begin(), it)]);
    }
    return out;
}

template <class T>
inline auto CSVReaderHeaderAware::readNextObject(const std::function<T(Row)>& transformFunc) -> std::optional<T> {
    auto m = readMap();
    if (!m) return std::nullopt;
    return transformFunc(*m);
}

template <class T>
inline auto CSVReaderHeaderAware::readAllObjects(const std::function<T(Row)>& transformFunc) -> std::vector<T> {
    std::vector<T> output;
    while (auto obj = readNextObject(transformFunc)) output.push_back(std::move(*obj));
    return output;
}
inline auto CSVReaderHeaderAware::headers() const -> const std::vector<std::string>& { return headers_; }
