#pragma once
#include <string>
#include <utility>
#include <vector>

#include "DiffTypes.hpp"

class DiffBuilder {
 protected:
    struct Config {
        bool ignoreMissing = false;
        bool ignoreOrder = true;
        bool showMatches = false;
        std::vector<std::string> ignoreFieldSet;
        std::vector<std::string> idFields;
        std::string logLevel;
    } config;

 public:
    virtual ~DiffBuilder() = default;
    DiffBuilder& setIgnoreMissing(bool val) {
        config.ignoreMissing = val;
        return *this;
    }
    DiffBuilder& setIgnoreOrder(bool val) {
        config.ignoreOrder = val;
        return *this;
    }
    DiffBuilder& setShowMatches(bool val) {
        config.showMatches = val;
        return *this;
    }
    DiffBuilder& setLogLevel(std::string l) {
        config.logLevel = std::move(l);
        return *this;
    }

    DiffBuilder& setIgnoreFields(std::vector<std::string> fields) {
        config.ignoreFieldSet = std::move(fields);
        return *this;
    }

    DiffBuilder& setIdFields(std::vector<std::string> ids) {
        config.idFields = std::move(ids);
        return *this;
    }

    DiffBuilder& setIdField(std::string id) {
        config.idFields = std::move(std::vector<std::string>{id});
        return *this;
    }

    virtual std::vector<DiffEntry> build(const std::vector<std::string>& referenceFiles,
                                         const std::vector<std::string>& actualFiles) = 0;
};
