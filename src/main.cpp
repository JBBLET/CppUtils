#include <print>
#include <vector>

#include "cpputils/exception.hpp"
#include "cpputils/file.hpp"
#include "cpputils/logging.hpp"
#include "csv/csvReaderAware.hpp"
#include "product_serde.hpp"
#include "sample_types.hpp"

using csvutils::sample::Product;
using csvutils::sample::productFromRow;

int main() {
    try {
        auto csv = File("test/resources/kitchen_sink.csv");
        logging::info("{} File found", csv);
        auto file = csv.stream();
        auto reader = CSVReaderHeaderAware{file};

        auto products = reader.readAllObjects<Product>(productFromRow);

        std::println("Parsed {} products", products.size());
        for (const auto& p : products) {
            std::println("  #{} sku='{}' price={} qty={} inStock={} tags={} dims=({},{},{}) discount={}%",
                         p.id,
                         p.sku,
                         p.price,
                         p.quantity,
                         p.inStock,
                         p.tags.size(),
                         p.dimensions.x,
                         p.dimensions.y,
                         p.dimensions.z,
                         p.discountPct);
        }
        return 0;
    } catch (const Exception& e) {
        std::println("{}", e.what());
        return 1;
    }
}
