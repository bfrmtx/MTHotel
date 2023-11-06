#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "sqlite_handler.h"
using namespace std;

int main() {

  // auto sql_info = std::unique_ptr<sqlite_handler>();

  std::filesystem::path sqlfile("/home/bfr/number_test.sql3");
  auto sql_info = std::make_unique<sqlite_handler>(sqlfile);

  // std::filesystem::path sqlfile("/usr/local/procmt/bin/info.sql3");
  std::vector<std::vector<std::string>> table, table2;
  std::vector<double> f;
  std::vector<uint64_t> vui;

  std::cout << table.size() << std::endl;
  // table2 = table;

  string sql_query("SELECT * FROM uints;");
  try {
    table = sql_info->sqlite_select_strs(sql_query);
  } catch (const std::runtime_error &error) {
    std::cerr << error.what() << std::endl;
    sql_info.reset();
  }
  if (table.size()) {
    sql_info->show_table(table);
  }

  sql_query = "SELECT value FROM uints";
  try {
    vui = sql_info->sqlite_vector_uint64_t(sql_query);
  } catch (const std::runtime_error &error) {
    std::cerr << error.what() << std::endl;
    sql_info.reset();
  }

  for (const auto &v : vui) {
    std::cout << v << std::endl;
  }
  std::cout << std::endl;

  sqlfile = "/tmp/number_test.sql3";
  sql_query = "CREATE TABLE IF NOT EXISTS `uuints` ( `key` TEXT, `value` UNSIGNED INTEGER)";

  try {
    sql_info->create_table(sql_query);
  } catch (const std::runtime_error &error) {
    std::cerr << error.what() << std::endl
              << std::endl;
    sql_info.reset();
  }

  /*

      string sql_query("SELECT * FROM sensortypes;");

      try {
         table = sql_info->sqlite_select_strs(sqlfile, sql_query);
      }
catch (const std::runtime_error &error) {
    std::cerr << error.what() << std::endl;
          sql_info.reset();
      }

      if (table.size()) {
          sql_info->show_table(table);
      }

      sql_query = "PRAGMA table_info('sensortypes')" ;

      try {
          table2 = sql_info->sqlite_select_strs(sqlfile, sql_query);
      }
catch (const std::runtime_error &error) {
    std::cerr << error.what() << std::endl;
          sql_info.reset();
      }

      if (table2.size()) {
          sql_info->show_stable(table2);
      }

      sql_query = "SELECT * FROM default_mt_frequencies";
      try {
          f = sql_info->sqlite_vector_double(sqlfile, sql_query);
      }
catch (const std::runtime_error &error) {
    std::cerr << error.what() << std::endl;
          sql_info.reset();
      }

      if (f.size()) {
          for (const auto v : f) std::cout << std::setw(12) << v << "  " << 1.0/v << std::endl;
          std::cout << std::endl;
      }
  */
  return (0);
}
