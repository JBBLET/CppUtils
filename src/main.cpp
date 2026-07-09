#include "cpputils/error.hpp"
#include "cpputils/exception.hpp"
#include "cpputils/file.hpp"
#include "cpputils/logging.hpp"
#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

int main() {
  try {
    auto csv = File("test/resources/kitchen_sink.csv");
    logging::info("{} File found", csv);
    auto file = csv.stream();
    auto record = std::vector<std::string>{};
    auto rowNumber = 0;
    while (readRecord(file, record)) {
      std::cout << "row " << rowNumber++ << " (" << record.size()
                << " fields)\n";
      for (size_t i = 0; i < record.size(); ++i) {
        std::cout << "  [" << i << "] = <" << record[i] << ">\n";
      }
    }
    return 0;
  } catch (const Exception &e) {
    std::println("{}", e.what());
    return 1;
  }
}
