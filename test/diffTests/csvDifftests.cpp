#include <gtest/gtest.h>

#include "CSVDiffBuilder.hpp"

class CSVDiffTest : public ::testing::Test {
 protected:
    CSVDiffBuilder builder;
};

// Test the CSV splitting logic
TEST_F(CSVDiffTest, HandlesQuotedCells) {
    std::string line = "ID1,\"Name, With Comma\",Value1";
    auto fields = builder.splitCsvLine(line, ',');
    ASSERT_EQ(fields.size(), 3);
    EXPECT_EQ(fields[1], "Name, With Comma");
}

// Test basic ID extraction
TEST_F(CSVDiffTest, ExtractsCorrectId) {
    std::vector<std::string> fields = {"ID_123", "John", "Doe"};
    std::unordered_map<std::string, int> headerMap = {{"ID", 0}, {"FirstName", 1}};

    // Simulating config setup
    builder.setIdFields({"ID"});
    std::string id = builder.extractIdFromFields(fields, headerMap);
    EXPECT_EQ(id, "ID_123|");
}

// Test build with small data
TEST_F(CSVDiffTest, DetectsAddition) {
    // You can write small temp files here to test the full build() logic
}
