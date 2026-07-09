#include <cstddef>
#include <iostream>
#include <vector>

#include "cpputils/exception.hpp"
#include "cpputils/file.hpp"
#include "cpputils/logging.hpp"
#include "csv/csvReader.hpp"

int main() {
    try {
        auto csv = File("test/resources/kitchen_sink.csv");
        logging::info("{} File found", csv);
        auto file = csv.stream();
        auto csvReader = CSVReader{file};
        auto row = csvReader.readNext();
        while (row) {
            std::cout << "row " << csvReader.getRecordsRead() << " (" << row.value().size() << " fields)\n";
            for (size_t i = 0; i < row.value().size(); ++i) {
                std::cout << "  [" << i << "] = <" << row.value()[i] << ">\n";
            }
        }
        return 0;
    } catch (const Exception& e) {
        std::println("{}", e.what());
        return 1;
    }
}
