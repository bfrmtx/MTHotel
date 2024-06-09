#ifndef ATMM_H
#define ATMM_H

#include <filesystem>
#include <fstream>
#include <iostream>
#include <utility>
#include <vector>

/*!
 * \brief The atmm class represents a selection; sample start and end, like 0 and 1024 for example. to be used as for i = start; i < end; i++
 */

class atmm {
public:
  std::vector<std::pair<uint64_t, uint64_t>> atmm_data; //!< atmm data, start sample, end sample
  atmm() {
  }

  void read_data(const std::filesystem::path &atmmfile, const size_t n) {

    std::ifstream file;
    std::filesystem::path atmm_file(atmmfile);
    atmm_file.replace_extension(".atmm");
    try {
      if (std::filesystem::is_regular_file(atmm_file)) {
        file.open(atmm_file, std::ios::in | std::ios::binary);
      }
    } catch (std::filesystem::filesystem_error &e) {
      std::string err_str = __func__;
      std::cerr << err_str << " " << e.what() << std::endl;
      return;
    }
    this->atmm_data.clear();

    // get the file size in bytes from the OS
    auto file_size = std::filesystem::file_size(atmm_file);
    // calculate the number of pairs in the file
    auto num_pairs = file_size / (2 * sizeof(uint64_t));
    atmm_data.reserve(num_pairs);

    // read the pairs from the binary file into the atmm_data vector, for the complete file
    // while not EOF

    while (!file.eof()) {
      std::pair<uint64_t, uint64_t> atmm_pair;
      file.read(reinterpret_cast<char *>(&atmm_pair.first), sizeof(uint64_t));
      file.read(reinterpret_cast<char *>(&atmm_pair.second), sizeof(uint64_t));
      this->atmm_data.push_back(atmm_pair);
    }

    file.close();
  }

  void write_data(const std::vector<bool> &data, const std::filesystem::path &atmmfile) {
    std::ofstream file;
    std::filesystem::path atmm_file(atmmfile);
    atmm_file.replace_extension(".atmm");
    try {
      file.open(atmm_file, std::ios::out | std::ios::binary);
    } catch (std::filesystem::filesystem_error &e) {
      std::string err_str = __func__;
      std::cerr << err_str << " " << e.what() << std::endl;
      return;
    }

    // write the pairs to the binary file from the atmm_data vector
    for (size_t i = 0; i < this->atmm_data.size(); i++) {
      file.write(reinterpret_cast<const char *>(&this->atmm_data[i].first), sizeof(uint64_t));
      file.write(reinterpret_cast<const char *>(&this->atmm_data[i].second), sizeof(uint64_t));
    }

    file.close();
  }
  ~atmm() {}
};

#endif // ATMM_H
