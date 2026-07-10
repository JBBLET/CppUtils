#include <gtest/gtest.h>

#include <chrono>
#include <fstream>
#include <functional>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "cpputils/exception.hpp"
#include "csv/csvReaderAware.hpp"
#include "product_serde.hpp"
#include "sample_types.hpp"

#ifndef KITCHEN_SINK_CSV
#define KITCHEN_SINK_CSV "test/resources/kitchen_sink.csv"
#endif

using csvutils::sample::Currency;
using csvutils::sample::Product;
using csvutils::sample::productFromRow;
using csvutils::sample::Status;

namespace {

auto kitchenSink() -> std::ifstream {
    std::ifstream in{KITCHEN_SINK_CSV};
    EXPECT_TRUE(in.is_open()) << "Cannot open fixture: " << KITCHEN_SINK_CSV;
    return in;
}

}  // namespace

TEST(CSVReaderHeaderAware, ConstructorConsumesHeaderRow) {
    auto in = kitchenSink();
    CSVReaderHeaderAware reader{CSVReader{in}};

    ASSERT_EQ(reader.headers().size(), 17u);
    EXPECT_EQ(reader.headers().front(), "id");
    EXPECT_EQ(reader.headers()[1], "sku");
    EXPECT_EQ(reader.headers().back(), "category_id");
}

TEST(CSVReaderHeaderAware, ThrowsWhenStreamHasNoHeader) {
    std::istringstream empty{""};
    EXPECT_THROW(CSVReaderHeaderAware{CSVReader{empty}}, Exception);
}

TEST(CSVReaderHeaderAware, ReadMapKeysCellsByHeader) {
    auto in = kitchenSink();
    CSVReaderHeaderAware reader{CSVReader{in}};

    auto row = reader.readMap();
    ASSERT_TRUE(row.has_value());
    EXPECT_EQ(row->size(), 17u);
    EXPECT_EQ(row->at("id"), "1");
    EXPECT_EQ(row->at("sku"), "ABC-123");
    EXPECT_EQ(row->at("description"), "A great product");
    EXPECT_EQ(row->at("category_id"), "42");
}

TEST(CSVReaderHeaderAware, ReadMapReturnsNulloptAtEnd) {
    auto in = kitchenSink();
    CSVReaderHeaderAware reader{CSVReader{in}};

    for (int i = 0; i < 4; ++i) EXPECT_TRUE(reader.readMap().has_value());
    EXPECT_FALSE(reader.readMap().has_value());
}

TEST(CSVReaderHeaderAware, ReadAllMapsReturnsEveryDataRow) {
    auto in = kitchenSink();
    CSVReaderHeaderAware reader{CSVReader{in}};

    auto rows = reader.readAllMaps();
    ASSERT_EQ(rows.size(), 4u);
    EXPECT_EQ(rows[1].at("sku"), "XYZ, special edition");  // quoted comma survives
    EXPECT_EQ(rows[3].at("sku"), "PAD-7");                 // whitespace trimmed
}

TEST(CSVReaderHeaderAware, ThrowsWhenRowFieldCountMismatchesHeader) {
    std::istringstream in{"a,b,c\n1,2\n"};  // data row has 2 fields, header has 3
    CSVReaderHeaderAware reader{CSVReader{in}};

    EXPECT_THROW(reader.readMap(), Exception);
}

TEST(CSVReaderHeaderAware, ReadNextSelectsColumnsByName) {
    auto in = kitchenSink();
    CSVReaderHeaderAware reader{CSVReader{in}};

    auto picked = reader.readNext({"sku", "status", "quantity"});
    ASSERT_TRUE(picked.has_value());
    EXPECT_EQ(*picked, (std::vector<std::string>{"ABC-123", "Active", "5"}));
}

TEST(CSVReaderHeaderAware, ReadNextThrowsOnUnknownColumn) {
    auto in = kitchenSink();
    CSVReaderHeaderAware reader{CSVReader{in}};

    EXPECT_THROW(reader.readNext({"does_not_exist"}), Exception);
}

TEST(CSVReaderHeaderAware, ReadNextObjectTransformsSingleRow) {
    auto in = kitchenSink();
    CSVReaderHeaderAware reader{CSVReader{in}};

    auto product = reader.readNextObject<Product>(productFromRow);
    ASSERT_TRUE(product.has_value());
    EXPECT_EQ(product->id, 1);
    EXPECT_EQ(product->sku, "ABC-123");
    EXPECT_DOUBLE_EQ(product->price, 19.99);
    EXPECT_EQ(product->currency, Currency::USD);
    EXPECT_EQ(product->quantity, 5u);
    EXPECT_TRUE(product->inStock);
    ASSERT_TRUE(product->rating.has_value());
    EXPECT_DOUBLE_EQ(*product->rating, 4.5);
    EXPECT_EQ(product->tags, (std::vector<std::string>{"electronics", "gadgets"}));
    EXPECT_DOUBLE_EQ(product->dimensions.x, 10.0);
    EXPECT_DOUBLE_EQ(product->dimensions.z, 3.0);
    EXPECT_DOUBLE_EQ(product->discountPct, 12.5);
    EXPECT_EQ(product->status, Status::Active);
    ASSERT_TRUE(product->categoryId.has_value());
    EXPECT_EQ(*product->categoryId, 42);
}

TEST(CSVReaderHeaderAware, ReadNextObjectReturnsNulloptAtEnd) {
    auto in = kitchenSink();
    CSVReaderHeaderAware reader{CSVReader{in}};

    for (int i = 0; i < 4; ++i) EXPECT_TRUE(reader.readNextObject<Product>(productFromRow).has_value());
    EXPECT_FALSE(reader.readNextObject<Product>(productFromRow).has_value());
}

TEST(CSVReaderHeaderAware, ReadAllObjectsParsesWholeFixture) {
    auto in = kitchenSink();
    CSVReaderHeaderAware reader{CSVReader{in}};

    auto products = reader.readAllObjects<Product>(productFromRow);
    ASSERT_EQ(products.size(), 4u);

    // Row 2: empty rating / weight / category_id map to nullopt; quoted comma sku.
    EXPECT_EQ(products[1].sku, "XYZ, special edition");
    EXPECT_FALSE(products[1].rating.has_value());
    EXPECT_FALSE(products[1].weightKg.has_value());
    EXPECT_FALSE(products[1].categoryId.has_value());
    EXPECT_EQ(products[1].currency, Currency::EUR);
    EXPECT_EQ(products[1].status, Status::Pending);
    EXPECT_EQ(products[1].description, "He said \"the best ever\"");

    // Row 3: "1" parses as inStock=true, embedded newline in description.
    EXPECT_TRUE(products[2].inStock);
    EXPECT_EQ(products[2].status, Status::Discontinued);
    EXPECT_EQ(products[2].description, "Multi-line\ndescription spanning rows");
    EXPECT_TRUE(products[2].tags == std::vector<std::string>{"tools"});

    // Row 4: "yes" -> true, trimmed sku, empty tags -> {}.
    EXPECT_EQ(products[3].sku, "PAD-7");
    EXPECT_TRUE(products[3].inStock);
    EXPECT_EQ(products[3].quantity, 999999u);
    EXPECT_TRUE(products[3].tags.empty());
    EXPECT_DOUBLE_EQ(products[3].discountPct, 100.0);
}

TEST(CSVReaderHeaderAware, ReadAllObjectsAcceptsInlineLambdaTransform) {
    auto in = kitchenSink();
    CSVReaderHeaderAware reader{CSVReader{in}};

    // Exercise the template with a caller-supplied transform other than productFromRow.
    std::function<std::string(Row)> skuOf = [](const Row& row) { return row.at("sku"); };
    auto skus = reader.readAllObjects<std::string>(skuOf);
    ASSERT_EQ(skus.size(), 4u);
    EXPECT_EQ(skus.front(), "ABC-123");
    EXPECT_EQ(skus.back(), "PAD-7");
}
