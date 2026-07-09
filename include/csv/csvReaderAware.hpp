#pragma once

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "csv/csvReader.hpp"

class CSVReaderHeaderAware {
 public:
    explicit CSVReaderHeaderAware(CSVReader reader);

    auto readMap() -> std::optional<std::unordered_map<std::string, std::string>>;
    auto readNext(const std::vector<std::string>& columns) -> std::optional<std::vector<std::string>>;

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

inline auto CSVReaderHeaderAware::readMap() -> std::optional<std::unordered_map<std::string, std::string>> {
    auto row = reader_.readNext();
    if (!row) return std::nullopt;
    ensure(row->size() == headers_.size(), "Row has {} fields but header has {}", row->size(), headers_.size());
    std::unordered_map<std::string, std::string> out;
    for (std::size_t i = 0; i < headers_.size(); ++i) out.emplace(headers_[i], std::move((*row)[i]));
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

inline auto CSVReaderHeaderAware::headers() const -> const std::vector<std::string>& { return headers_; }
