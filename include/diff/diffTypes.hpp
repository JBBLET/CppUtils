#pragma once
#include <map>
#include <string>
#include <vector>

enum class DiffType {
    ENTRY_ADDITION,
    ENTRY_DELETION,
    FIELD_ADDITION,
    FIELD_DELETION,
    VALUE_CHANGE,
    VALUE_MATCH,
};

struct DiffEntry {
    DiffType type;
    std::string field;
    std::string oldValue;
    std::string newValue;
    std::string uniqueIdentifier;
};
