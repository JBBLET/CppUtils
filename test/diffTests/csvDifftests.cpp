#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "csv/csvReader.hpp"
#include "diff/csvDiffBuilder.hpp"

#ifndef DIFF_FIXTURE_DIR
#define DIFF_FIXTURE_DIR "test/diffTests"
#endif

using cpputils::diff::DiffBuilder;
using cpputils::diff::DiffEntry;
using cpputils::diff::DiffType;

namespace {

namespace fs = std::filesystem;

// Writes CSV content to a uniquely-named temp file and removes it on scope exit,
// so build() (which takes file paths) can be exercised hermetically.
struct TempCsv {
    fs::path path;
    explicit TempCsv(const std::string& content) {
        static std::atomic<int> counter{0};
        path = fs::temp_directory_path() / ("csvdiff_test_" + std::to_string(counter.fetch_add(1)) + ".csv");
        std::ofstream{path} << content;
    }
    TempCsv(const TempCsv&) = delete;
    TempCsv& operator=(const TempCsv&) = delete;
    ~TempCsv() {
        std::error_code ec;
        fs::remove(path, ec);
    }
    std::string str() const { return path.string(); }
};

auto countType(const std::vector<DiffEntry>& diffs, DiffType type) -> std::size_t {
    return std::count_if(diffs.begin(), diffs.end(), [&](const DiffEntry& d) { return d.type == type; });
}

// First entry of a given type, or a default-constructed sentinel if none.
auto firstOfType(const std::vector<DiffEntry>& diffs, DiffType type) -> DiffEntry {
    for (const auto& d : diffs)
        if (d.type == type) return d;
    return DiffEntry{};
}

}  // namespace

class CSVDiffTest : public ::testing::Test {
 protected:
    cpputils::csv::CSVDiffBuilder builder;
};

// The diff builder now delegates tokenization to CSVReader; this pins the
// quoted-comma behaviour it relies on.
TEST_F(CSVDiffTest, HandlesQuotedCells) {
    std::istringstream in{"ID1,\"Name, With Comma\",Value1"};
    cpputils::csv::CSVReader reader{in};

    auto fields = reader.readNext();
    ASSERT_TRUE(fields.has_value());
    ASSERT_EQ(fields->size(), 3u);
    EXPECT_EQ((*fields)[1], "Name, With Comma");
}

TEST_F(CSVDiffTest, ExtractsCorrectId) {
    std::vector<std::string> fields = {"ID_123", "John", "Doe"};
    std::unordered_map<std::string, int> headerMap = {{"ID", 0}, {"FirstName", 1}};

    builder.setIdFields({"ID"});
    std::string id = builder.extractIdFromFields(fields, headerMap);
    EXPECT_EQ(id, "ID_123|");
}

TEST_F(CSVDiffTest, DetectsValueChange) {
    TempCsv ref{"id,name,score\n1,alice,10\n2,bob,20\n"};
    TempCsv act{"id,name,score\n1,alice,10\n2,bob,99\n"};
    builder.setIdField("id");

    auto diffs = builder.build({ref.str()}, {act.str()});

    ASSERT_EQ(countType(diffs, DiffType::VALUE_CHANGE), 1u);
    auto change = firstOfType(diffs, DiffType::VALUE_CHANGE);
    EXPECT_EQ(change.field, "score");
    EXPECT_EQ(change.oldValue, "20");
    EXPECT_EQ(change.newValue, "99");
    EXPECT_EQ(change.uniqueIdentifier, "2|");
    EXPECT_EQ(countType(diffs, DiffType::ENTRY_ADDITION), 0u);
    EXPECT_EQ(countType(diffs, DiffType::ENTRY_DELETION), 0u);
}

TEST_F(CSVDiffTest, DetectsAddition) {
    TempCsv ref{"id,name\n1,alice\n"};
    TempCsv act{"id,name\n1,alice\n2,bob\n"};
    builder.setIdField("id");

    auto diffs = builder.build({ref.str()}, {act.str()});

    ASSERT_EQ(countType(diffs, DiffType::ENTRY_ADDITION), 1u);
    EXPECT_EQ(firstOfType(diffs, DiffType::ENTRY_ADDITION).uniqueIdentifier, "2|");
    EXPECT_EQ(countType(diffs, DiffType::ENTRY_DELETION), 0u);
}

TEST_F(CSVDiffTest, DetectsDeletion) {
    TempCsv ref{"id,name\n1,alice\n2,bob\n"};
    TempCsv act{"id,name\n1,alice\n"};
    builder.setIdField("id");

    auto diffs = builder.build({ref.str()}, {act.str()});

    ASSERT_EQ(countType(diffs, DiffType::ENTRY_DELETION), 1u);
    EXPECT_EQ(firstOfType(diffs, DiffType::ENTRY_DELETION).uniqueIdentifier, "2|");
    EXPECT_EQ(countType(diffs, DiffType::ENTRY_ADDITION), 0u);
}

TEST_F(CSVDiffTest, DetectsColumnAddAndDelete) {
    TempCsv ref{"id,name,old_col\n1,alice,x\n"};
    TempCsv act{"id,name,new_col\n1,alice,y\n"};
    builder.setIdField("id");

    auto diffs = builder.build({ref.str()}, {act.str()});

    ASSERT_EQ(countType(diffs, DiffType::FIELD_ADDITION), 1u);
    EXPECT_EQ(firstOfType(diffs, DiffType::FIELD_ADDITION).field, "new_col");
    ASSERT_EQ(countType(diffs, DiffType::FIELD_DELETION), 1u);
    EXPECT_EQ(firstOfType(diffs, DiffType::FIELD_DELETION).field, "old_col");
    // id + name are unchanged common columns -> no value changes.
    EXPECT_EQ(countType(diffs, DiffType::VALUE_CHANGE), 0u);
}

// The payoff of routing through CSVReader: a quoted field containing a newline
// stays a single field. The old getline() parser split it across records and
// would report spurious diffs / wrong field counts here.
TEST_F(CSVDiffTest, HandlesQuotedNewlineInField) {
    TempCsv ref{"id,note,score\n1,\"line1\nline2\",10\n"};
    TempCsv act{"id,note,score\n1,\"line1\nline2\",20\n"};
    builder.setIdField("id");

    auto diffs = builder.build({ref.str()}, {act.str()});

    // Only score differs; the multi-line note must compare equal.
    ASSERT_EQ(countType(diffs, DiffType::VALUE_CHANGE), 1u);
    EXPECT_EQ(firstOfType(diffs, DiffType::VALUE_CHANGE).field, "score");
    EXPECT_EQ(countType(diffs, DiffType::ENTRY_ADDITION), 0u);
    EXPECT_EQ(countType(diffs, DiffType::ENTRY_DELETION), 0u);
}

// End-to-end smoke test over the committed 100k-row fixtures.
TEST_F(CSVDiffTest, DiffsFullFixtures) {
    const std::string dir = DIFF_FIXTURE_DIR;
    builder.setIdField("UNIQUE_ID");

    auto diffs = builder.build({dir + "/reference.csv"}, {dir + "/actual.csv"});

    // actual.csv drops FIELD_10 and appends NEW_FIELD_X.
    ASSERT_EQ(countType(diffs, DiffType::FIELD_ADDITION), 1u);
    EXPECT_EQ(firstOfType(diffs, DiffType::FIELD_ADDITION).field, "NEW_FIELD_X");
    ASSERT_EQ(countType(diffs, DiffType::FIELD_DELETION), 1u);
    EXPECT_EQ(firstOfType(diffs, DiffType::FIELD_DELETION).field, "FIELD_10");

    // Sanity, not exact counts: generate_test_data.py can be re-run to change
    // how many rows are modified/added/deleted, so only require that each kind
    // of row-level diff is exercised.
    EXPECT_GT(countType(diffs, DiffType::VALUE_CHANGE), 0u);
    EXPECT_GT(countType(diffs, DiffType::ENTRY_ADDITION), 0u);
    EXPECT_GT(countType(diffs, DiffType::ENTRY_DELETION), 0u);
}
