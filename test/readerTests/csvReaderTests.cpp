#include <gtest/gtest.h>

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "cpputils/exception.hpp"
#include "csv/csvReader.hpp"

// The kitchen_sink.csv path is injected by CMake so the test is independent of
// the working directory ctest happens to run it from.
#ifndef KITCHEN_SINK_CSV
#define KITCHEN_SINK_CSV "test/resources/kitchen_sink.csv"
#endif

namespace {

// Column order of kitchen_sink.csv, used across several assertions.
const std::vector<std::string> kHeader = {"id",
                                          "sku",
                                          "price",
                                          "currency",
                                          "quantity",
                                          "in_stock",
                                          "rating",
                                          "tags",
                                          "created_at",
                                          "release_date",
                                          "weight_kg",
                                          "dimensions",
                                          "discount_pct",
                                          "status",
                                          "description",
                                          "score",
                                          "category_id"};

// Fresh stream over the on-disk fixture. The stream must outlive the reader,
// so callers keep it as a local.
auto kitchenSink() -> std::ifstream {
    std::ifstream in{KITCHEN_SINK_CSV};
    EXPECT_TRUE(in.is_open()) << "Cannot open fixture: " << KITCHEN_SINK_CSV;
    return in;
}

}  // namespace

TEST(CSVReader, ReadsHeaderRowAsFirstRecord) {
    auto in = kitchenSink();
    cpputils::csv::CSVReader reader{in};

    auto header = reader.readNext();
    ASSERT_TRUE(header.has_value());
    EXPECT_EQ(*header, kHeader);
    EXPECT_EQ(header->size(), 17u);
}

TEST(CSVReader, ReadAllReturnsHeaderPlusEveryDataRow) {
    auto in = kitchenSink();
    cpputils::csv::CSVReader reader{in};

    auto rows = reader.readAll();
    // 1 header + 4 data rows. The multi-line quoted field in row 3 must NOT be
    // split into two records.
    ASSERT_EQ(rows.size(), 5u);
    for (const auto& row : rows) EXPECT_EQ(row.size(), 17u);
}

TEST(CSVReader, KeepsCommaInsideQuotedField) {
    auto in = kitchenSink();
    cpputils::csv::CSVReader reader{in};
    reader.skip(1);  // header

    auto row1 = reader.readNext();
    auto row2 = reader.readNext();
    ASSERT_TRUE(row2.has_value());
    // "XYZ, special edition" is one field despite the embedded comma.
    EXPECT_EQ((*row2)[1], "XYZ, special edition");
    EXPECT_EQ(row2->size(), 17u);
}

TEST(CSVReader, UnescapesDoubledQuotes) {
    auto in = kitchenSink();
    cpputils::csv::CSVReader reader{in};
    reader.skip(2);  // header + row 1

    auto row2 = reader.readNext();
    ASSERT_TRUE(row2.has_value());
    // "" inside a quoted field collapses to a single literal quote.
    EXPECT_EQ((*row2)[14], "He said \"the best ever\"");
}

TEST(CSVReader, KeepsEmbeddedNewlineInsideQuotes) {
    auto in = kitchenSink();
    cpputils::csv::CSVReader reader{in};
    reader.skip(3);  // header + rows 1,2

    auto row3 = reader.readNext();
    ASSERT_TRUE(row3.has_value());
    EXPECT_EQ((*row3)[14], "Multi-line\ndescription spanning rows");
}

TEST(CSVReader, TrimsWhitespaceFromUnquotedField) {
    auto in = kitchenSink();
    cpputils::csv::CSVReader reader{in};
    reader.skip(4);  // header + rows 1,2,3

    auto row4 = reader.readNext();
    ASSERT_TRUE(row4.has_value());
    // "   PAD-7   " is unquoted, so leading/trailing whitespace is stripped.
    EXPECT_EQ((*row4)[1], "PAD-7");
}

TEST(CSVReader, ReturnsNulloptPastEndOfInput) {
    auto in = kitchenSink();
    cpputils::csv::CSVReader reader{in};

    reader.skip(5);  // header + 4 data rows
    EXPECT_FALSE(reader.readNext().has_value());
    EXPECT_FALSE(reader.readNext().has_value());  // idempotent at EOF
}

TEST(CSVReader, RecordAndLineCountersAdvance) {
    auto in = kitchenSink();
    cpputils::csv::CSVReader reader{in};

    reader.readAll();
    EXPECT_EQ(reader.getRecordsRead(), 5);
    // Row 3 spans two physical lines, so lines read exceeds records read.
    EXPECT_GE(reader.getLinesRead(), reader.getRecordsRead());
}

TEST(CSVReader, PeekDoesNotConsume) {
    auto in = kitchenSink();
    cpputils::csv::CSVReader reader{in};

    auto peeked = reader.peek();
    ASSERT_TRUE(peeked.has_value());
    EXPECT_EQ((*peeked)[0], "id");  // header

    auto next = reader.readNext();
    ASSERT_TRUE(next.has_value());
    EXPECT_EQ(*next, *peeked);  // peek returned the same record readNext yields
}

TEST(CSVReader, ParsesInlineStreamWithoutTrailingNewline) {
    std::istringstream in{"a,b,c\n1,2,3"};  // no newline after last record
    cpputils::csv::CSVReader reader{in};

    auto header = reader.readNext();
    auto row = reader.readNext();
    ASSERT_TRUE(row.has_value());
    EXPECT_EQ(*row, (std::vector<std::string>{"1", "2", "3"}));
    EXPECT_FALSE(reader.readNext().has_value());
}

TEST(CSVReader, ThrowsOnUnterminatedQuotedField) {
    std::istringstream in{"a,b\n\"oops,still open"};
    cpputils::csv::CSVReader reader{in};
    reader.readNext();  // header

    EXPECT_THROW(reader.readNext(), cpputils::Exception);
}
