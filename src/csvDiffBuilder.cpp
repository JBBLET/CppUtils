#include "CSVDiffBuilder.hpp"

#include <algorithm>
#include <fstream>
#include <unordered_set>

using std::find;
using std::getline;
using std::ifstream;
using std::string;
using std::unordered_map;
using std::unordered_set;
using std::vector;

vector<DiffEntry> CSVDiffBuilder::build(const vector<string>& refFiles, const vector<string>& actFiles) {
    vector<DiffEntry> results;
    results.reserve(50000);
    unordered_map<string, IndexedRow> refLookup;

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
            results.push_back({DiffType::ENTRY_DELETION, "", content.rawData, "", id});
        }
    }
    return results;
};

void CSVDiffBuilder::parseAndStore(const string& path, unordered_map<std::string, IndexedRow>& refLookup) {
    ifstream file(path);
    if (!file.is_open()) return;
    string line;

    if (!getline(file, line)) return;
    mapHeaderToIndicesToProperty(line);
    auto headers = splitCsvLine(line, ',');

    while (getline(file, line)) {
        if (line.empty()) continue;

        // Parse once
        auto fields = splitCsvLine(line, ',');
        string rowId = extractIdFromFields(fields, refHeaderIndices);

        // Hash the fields
        size_t rowHash = hashRow(fields, headers);

        // Use move to keep it fast
        refLookup[rowId] = IndexedRow{rowHash, std::move(line)};
    }
}
void CSVDiffBuilder::streamAndCompare(const string& path, unordered_map<string, IndexedRow>& refLookup,
                                      vector<DiffEntry>& results) {
    ifstream file(path);
    if (!file.is_open()) return;

    string headerLine;
    if (!getline(file, headerLine)) return;

    auto actHeaderIndices = mapHeaderToIndices(headerLine);
    auto actHeaders = splitCsvLine(headerLine, ',');

    struct HashPlan {
        string name;
        int actIdx;
    };
    vector<HashPlan> commonForHashing;

    struct CommonField {
        string name;
        int refIdx;
        int actIdx;
    };
    vector<CommonField> commonFields;

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

    string line;
    while (getline(file, line)) {
        if (line.empty()) continue;

        auto actFields = splitCsvLine(line, ',');
        string rowId = extractIdFromFields(actFields, actHeaderIndices);

        size_t actHash = 0;
        for (const auto& plan : commonForHashing) {
            string combined = plan.name + "=" + actFields[plan.actIdx];
            actHash ^= std::hash<string>{}(combined) + 0x9e3779b9 + (actHash << 6) + (actHash >> 2);
        }

        auto it = refLookup.find(rowId);
        if (it == refLookup.end()) {
            if (!config.ignoreMissing) {
                results.push_back({DiffType::ENTRY_ADDITION, "", "", line, rowId});
            }
        } else {
            if (it->second.hash != actHash) {
                auto refFields = splitCsvLine(it->second.rawData, ',');
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
bool CSVDiffBuilder::isIgnored(const std::string& colName) {
    return std::find(config.ignoreFieldSet.begin(), config.ignoreFieldSet.end(), colName) !=
           config.ignoreFieldSet.end();
}

void CSVDiffBuilder::mapHeaderToIndicesToProperty(const string& line) {
    std::unordered_map<std::string, int> headerIndices;
    char delimiter = ',';
    auto headers = splitCsvLine(line, delimiter);
    for (int i = 0; i < (int)headers.size(); ++i) {
        if (!isIgnored(headers[i])) {
            headerIndices[headers[i]] = i;
        }
    }
    refHeaderIndices = std::move(headerIndices);
}

std::unordered_map<std::string, int> CSVDiffBuilder::mapHeaderToIndices(const string& line) {
    std::unordered_map<std::string, int> headerIndices;
    char delimiter = ',';
    auto headers = splitCsvLine(line, delimiter);
    for (int i = 0; i < (int)headers.size(); ++i) {
        if (!isIgnored(headers[i])) {
            headerIndices[headers[i]] = i;
        }
    }
    return headerIndices;
}

size_t CSVDiffBuilder::hashRow(const std::vector<std::string>& fields, const std::vector<std::string>& headers) {
    size_t h = 0;
    for (size_t i = 0; i < fields.size(); ++i) {
        if (isIgnored(headers[i])) continue;
        std::string combined = headers[i] + "=" + fields[i];
        h ^= std::hash<std::string>{}(combined) + 0x9e3779b9 + (h << 6) + (h >> 2);
    }
    return h;
}

string CSVDiffBuilder::extractIdFromFields(const vector<string>& fields,
                                           const unordered_map<string, int>& headerIndices) {
    string combinedId;
    for (const auto& idName : config.idFields) {
        auto it = headerIndices.find(idName);
        if (it != headerIndices.end()) {
            combinedId += fields[it->second];
            combinedId += "|";
        }
    }
    return combinedId;
}

vector<string> CSVDiffBuilder::splitCsvLine(const string& line, char delimiter) {
    bool inQuotes = false;
    string processedCell = "";
    vector<string> processedLine;
    for (size_t i = 0; i < line.length(); i++) {
        char c = line[i];
        if (c == '"') {
            if (inQuotes && i + 1 < line.size() && line[i + 1] == '"') {
                processedCell += '"';
                ++i;
            } else {
                inQuotes = !inQuotes;
            }
        } else if (c == delimiter && !inQuotes) {
            processedLine.push_back(sanitizeCell(processedCell));
            processedCell = "";
        } else {
            processedCell += c;
        }
    }
    processedLine.push_back(sanitizeCell(processedCell));
    return processedLine;
}
string CSVDiffBuilder::sanitizeCell(string cell) {
    // remove leading and trailing ""
    if (!cell.empty() && cell.front() == '"' && cell.back() == '"') {
        cell = cell.substr(1, cell.size() - 2);
    }
    // escaped quotes
    size_t pos = 0;
    while ((pos = cell.find("\"\"", pos)) != string::npos) {
        cell.replace(pos, 2, "\"");
        ++pos;
    }
    // remove leading and trailing whitespaces
    const char* WhiteSpace = " \t\v\r\f\n";
    size_t start = cell.find_first_not_of(WhiteSpace);
    if (string::npos == start) {
        return "";
    }
    size_t end = cell.find_last_not_of(WhiteSpace);
    cell = cell.substr(start, (end - start + 1));
    return cell;
}
