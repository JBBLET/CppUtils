#include <cstddef>
#include <iostream>
#include <vector>

#include "cpputils/exception.hpp"
#include "cpputils/file.hpp"
#include "cpputils/logging.hpp"
#include "csv/csvReaderAware.hpp"

int main() {
    try {
        auto csv = File("test/resources/kitchen_sink.csv");
        logging::info("{} File found", csv);
        auto file = csv.stream();
        auto csvReader = CSVReaderHeaderAware{file};
        auto headers = csvReader.headers();
        auto row = csvReader.readNext(headers);
        while (row) {
            std::cout << " (" << row.value().size() << " fields)\n";
            for (size_t i = 0; i < row.value().size(); ++i) {
                std::cout << "  [" << i << "] = <" << row.value()[i] << ">\n";
            }
            row = csvReader.readNext(headers);
        }
        return 0;
    } catch (const Exception& e) {
        std::println("{}", e.what());
        return 1;
    }
}
