#pragma once
#include <string>
#include <unordered_map>
#include <vector>

#include "DiffBuilder.hpp"

struct IndexedRow {
    size_t hash;
    std::string rawData;
};

class CSVDiffBuilder : public DiffBuilder {
 public:
    std::vector<DiffEntry> build(const std::vector<std::string>& refFiles,
                                 const std::vector<std::string>& actFiles) override;
    std::string sanitizeCell(std::string cell);
    std::vector<std::string> splitCsvLine(const std::string& line, char delimiter);
    std::string extractIdFromFields(const std::vector<std::string>& fields,
                                    const std::unordered_map<std::string, int>& headerIndices);

 private:
    std::unordered_map<std::string, int> refHeaderIndices;

    void parseAndStore(const std::string& path, std::unordered_map<std::string, IndexedRow>& map);
    void streamAndCompare(const std::string& path, std::unordered_map<std::string, IndexedRow>& refMap,
                          std::vector<DiffEntry>& results);

    void mapHeaderToIndicesToProperty(const std::string& line);
    std::unordered_map<std::string, int> mapHeaderToIndices(const std::string& line);
    bool isIgnored(const std::string& colName);
    size_t hashRow(const std::vector<std::string>& fields, const std::vector<std::string>& headers);
};
