#pragma once
#include <algorithm>
#include <fstream>
#include <functional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "csv/csvReader.hpp"
#include "diff/diffBuilder.hpp"
#include "diff/diffTypes.hpp"

namespace cpputils::csv {

using cpputils::diff::DiffBuilder;
using cpputils::diff::DiffEntry;
using cpputils::diff::DiffType;

struct IndexedRow {
    size_t hash;
    std::vector<std::string> fields;  // parsed once by CSVReader; no raw line to re-split
};

class CSVDiffBuilder : public DiffBuilder {
 public:
    std::vector<DiffEntry> build(const std::vector<std::string>& refFiles,
                                 const std::vector<std::string>& actFiles) override;
    std::string extractIdFromFields(const std::vector<std::string>& fields,
                                    const std::unordered_map<std::string, int>& headerIndices);

 private:
    std::unordered_map<std::string, int> refHeaderIndices;

    void parseAndStore(const std::string& path, std::unordered_map<std::string, IndexedRow>& map);
    void streamAndCompare(const std::string& path, std::unordered_map<std::string, IndexedRow>& refMap,
                          std::vector<DiffEntry>& results);

    void mapHeaderToIndicesToProperty(const std::vector<std::string>& headers);
    std::unordered_map<std::string, int> mapHeaderToIndices(const std::vector<std::string>& headers);
    bool isIgnored(const std::string& colName);
    size_t hashRow(const std::vector<std::string>& fields, const std::vector<std::string>& headers);
};

inline std::string joinCsv(const std::vector<std::string>& fields) {
    std::string out;
    for (std::size_t i = 0; i < fields.size(); ++i) {
        if (i != 0) out += ',';
        out += fields[i];
    }
    return out;
}

inline bool isBlankRecord(const std::vector<std::string>& fields) {
    return fields.empty() || (fields.size() == 1 && fields[0].empty());
}

inline std::vector<DiffEntry> CSVDiffBuilder::build(const std::vector<std::string>& refFiles,
                                                    const std::vector<std::string>& actFiles) {
    std::vector<DiffEntry> results;
    results.reserve(50000);
    std::unordered_map<std::string, IndexedRow> refLookup;

    for (const auto& file : refFiles) {
        parseAndStore(file, refLookup);
    }

    // 2. Compare Actual
    for (const auto& file : actFiles) {
        streamAndCompare(file, refLookup, results);
    }

    // 3. Handle Deletions
    if (!config.ignoreMissing) {
        for (auto const& [id, content] : refLookup) {
            results.push_back({DiffType::ENTRY_DELETION, "", joinCsv(content.fields), "", id});
        }
    }
    return results;
}

inline void CSVDiffBuilder::parseAndStore(const std::string& path,
                                          std::unordered_map<std::string, IndexedRow>& refLookup) {
    std::ifstream file(path);
    if (!file.is_open()) return;
    CSVReader reader{file};

    auto header = reader.readNext();
    if (!header) return;
    mapHeaderToIndicesToProperty(*header);
    const auto& headers = *header;

    while (auto row = reader.readNext()) {
        auto& fields = *row;
        if (isBlankRecord(fields)) continue;

        std::string rowId = extractIdFromFields(fields, refHeaderIndices);
        std::size_t rowHash = hashRow(fields, headers);

        refLookup[rowId] = IndexedRow{rowHash, std::move(fields)};
    }
}

inline void CSVDiffBuilder::streamAndCompare(const std::string& path,
                                             std::unordered_map<std::string, IndexedRow>& refLookup,
                                             std::vector<DiffEntry>& results) {
    std::ifstream file(path);
    if (!file.is_open()) return;
    CSVReader reader{file};

    auto header = reader.readNext();
    if (!header) return;
    auto actHeaderIndices = mapHeaderToIndices(*header);

    struct HashPlan {
        std::string name;
        int actIdx;
    };
    std::vector<HashPlan> commonForHashing;

    struct CommonField {
        std::string name;
        int refIdx;
        int actIdx;
    };
    std::vector<CommonField> commonFields;

    for (const auto& [name, actIdx] : actHeaderIndices) {
        auto it = refHeaderIndices.find(name);
        if (it == refHeaderIndices.end()) {
            results.push_back({DiffType::FIELD_ADDITION, name, "", "", ""});
        } else {
            commonFields.push_back({name, it->second, actIdx});
            commonForHashing.push_back({name, actIdx});
        }
    }

    for (const auto& [name, refIdx] : refHeaderIndices) {
        if (actHeaderIndices.find(name) == actHeaderIndices.end() && !isIgnored(name)) {
            results.push_back({DiffType::FIELD_DELETION, name, "", "", ""});
        }
    }

    while (auto row = reader.readNext()) {
        auto& actFields = *row;
        if (isBlankRecord(actFields)) continue;

        std::string rowId = extractIdFromFields(actFields, actHeaderIndices);

        size_t actHash = 0;
        for (const auto& plan : commonForHashing) {
            std::string combined = plan.name + "=" + actFields[plan.actIdx];
            actHash ^= std::hash<std::string>{}(combined) + 0x9e3779b9 + (actHash << 6) + (actHash >> 2);
        }

        auto it = refLookup.find(rowId);
        if (it == refLookup.end()) {
            if (!config.ignoreMissing) {
                results.push_back({DiffType::ENTRY_ADDITION, "", "", joinCsv(actFields), rowId});
            }
        } else {
            if (it->second.hash != actHash) {
                const auto& refFields = it->second.fields;  // already parsed, no re-split
                for (const auto& field : commonFields) {
                    if (refFields[field.refIdx] != actFields[field.actIdx]) {
                        results.push_back({DiffType::VALUE_CHANGE,
                                           field.name,
                                           refFields[field.refIdx],
                                           actFields[field.actIdx],
                                           rowId});
                    }
                }
            }
            refLookup.erase(it);
        }
    }
}

inline bool CSVDiffBuilder::isIgnored(const std::string& colName) {
    return std::find(config.ignoreFieldSet.begin(), config.ignoreFieldSet.end(), colName) !=
           config.ignoreFieldSet.end();
}

inline void CSVDiffBuilder::mapHeaderToIndicesToProperty(const std::vector<std::string>& headers) {
    std::unordered_map<std::string, int> headerIndices;
    for (int i = 0; i < static_cast<int>(headers.size()); ++i) {
        if (!isIgnored(headers[i])) {
            headerIndices[headers[i]] = i;
        }
    }
    refHeaderIndices = std::move(headerIndices);
}

inline std::unordered_map<std::string, int> CSVDiffBuilder::mapHeaderToIndices(
    const std::vector<std::string>& headers) {
    std::unordered_map<std::string, int> headerIndices;
    for (int i = 0; i < static_cast<int>(headers.size()); ++i) {
        if (!isIgnored(headers[i])) {
            headerIndices[headers[i]] = i;
        }
    }
    return headerIndices;
}

inline size_t CSVDiffBuilder::hashRow(const std::vector<std::string>& fields, const std::vector<std::string>& headers) {
    size_t h = 0;
    for (size_t i = 0; i < fields.size(); ++i) {
        if (isIgnored(headers[i])) continue;
        std::string combined = headers[i] + "=" + fields[i];
        h ^= std::hash<std::string>{}(combined) + 0x9e3779b9 + (h << 6) + (h >> 2);
    }
    return h;
}

inline std::string CSVDiffBuilder::extractIdFromFields(const std::vector<std::string>& fields,
                                                       const std::unordered_map<std::string, int>& headerIndices) {
    std::string combinedId;
    for (const auto& idName : config.idFields) {
        auto it = headerIndices.find(idName);
        if (it != headerIndices.end()) {
            combinedId += fields[it->second];
            combinedId += "|";
        }
    }
    return combinedId;
}
}  // namespace cpputils::csv
