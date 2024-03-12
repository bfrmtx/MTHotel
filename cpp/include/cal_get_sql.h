#ifndef CAL_GET_SQL_H
#define CAL_GET_SQL_H
#include "cal_base.h"
#include "sqlite_handler.h"
#include <algorithm>
#include <filesystem>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

class get_from_master_cal {
public:
  get_from_master_cal(const std::filesystem::path &db_path) {
    if (!std::filesystem::exists(db_path)) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << ":: db file does not exist -> " << db_path;
      throw std::runtime_error(err_str.str());
    }
    if (!std::filesystem::is_regular_file(db_path)) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << ":: db file is not a regular file -> " << db_path;
      throw std::runtime_error(err_str.str());
    }
    this->master_cal = std::make_unique<sqlite_handler>(db_path);
  }
  ~get_from_master_cal() = default;

  void get_master_cal(std::shared_ptr<calibration> &cal) {
    auto sensor = cal->sensor;
    auto chopper = cal->chopper;
    // get chopper status as integer from enum
    int chopper_status = static_cast<int>(chopper);
    std::vector<std::vector<double>> fap;
    try {
      this->sql_query = "SELECT f, a, p FROM '" + sensor + "' WHERE chopper = " + std::to_string(chopper_status) + ";";
      fap = this->master_cal->sqlite_vector_three_doubles_column(this->sql_query);
      if (fap.empty()) {
        std::ostringstream err_str(__func__, std::ios_base::ate);
        err_str << ":: no master cal found for sensor -> " << sensor;
      }
    } catch (const std::exception &e) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << ":: error getting master cal for sensor -> " << sensor << " -> " << e.what();
      throw std::runtime_error(err_str.str());
    }
    std::vector<double> f, a, p;
    master_cal->vec_vec_to_three_vec(fap, f, a, p);
    cal->set_master_cal(f, a, p);
    this->sql_query.clear();
  }

private:
  std::unique_ptr<sqlite_handler> master_cal;
  std::string sql_query;

  // std::vector<std::string> get_all_sensor_names() {
  //   std::vector<std::string> sensor_names;
  //   try {
  //     sqlite_handler db(db_path);
  //     auto res = db.get_all_sensor_names();
  //     for (const auto &row : res) {
  //       sensor_names.push_back(row);
  //     }
  //   } catch (const std::exception &e) {
  //     std::ostringstream err_str(__func__, std::ios_base::ate);
  //     err_str << ":: error getting all sensor names -> " << e.what();
  //     throw std::runtime_error(err_str.str());
  //   }
  //   return sensor_names;
  // }

  // std::vector<std::string> get_all_sensor_serials() {
  //   std::vector<std::string> sensor_serials;
  //   try {
  //     sqlite_handler db(db_path);
  //     auto res = db.get_all_sensor_serials();
  //     for (const auto &row : res) {
  //       sensor_serials.push_back(row);
  //     }
  //   } catch (const std::exception &e) {
  //     std::ostringstream err_str(__func__, std::ios_base::ate);
  //     err_str << ":: error getting all sensor serials ->  " << e.what();
  //     throw std::runtime_error(err_str.str());
  //   }
  //   return sensor_serials;
  // }
}; // end class get_from_master_cal

#endif // CAL_GET_SQL_H
