#include "about_system.h"
#include "sqlite_handler.h"
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

int main() {
  std::filesystem::path sqlfile;
  sqlfile = working_dir_data("filter.sql3");
  auto sql_db = std::make_unique<sqlite_handler>(sqlfile);
  std::string sql_query("SELECT * FROM mtx8;");

  auto result = sql_db->sqlite_vector_double(sql_query);
  double sum = 0;
  for (const auto &v : result) {
    sum += v;
  }
  std::cout << "sum: " << sum << std::endl;

  return 0;
}