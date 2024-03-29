#ifndef READ_CAL_H
#define READ_CAL_H

#include <algorithm>
#include <cfloat>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "sqlite_handler.h"

namespace fs = std::filesystem;

#include "cal_base.h"
#include "mt_base.h"

class read_cal {
public:
  read_cal();
  ~read_cal();
  bool is_init() const {
    return this->dbloaded;
  }

  /*!
   * @brief reads a standard calibration file from metronix (V/(nT * Hz)) and returns a calibration object (mV/nT)
   * @param filename
   * @param chopper
   * @return
   */
  std::shared_ptr<calibration> read_std_mtx_txt(const fs::path &filename, const ChopperStatus &chopper);
  /*!
   * \brief read_std_xml reads calibration from a measdoc xml file and returns a vector of calibration objects in new format (mV/nT)
   * \param filename e.g. a measdoc.xml with multiple channels
   * \return
   */
  std::vector<std::shared_ptr<calibration>> read_std_xml(const fs::path &filename, std::string &messages);

  /*!
   * \brief read_std_xml_single same as above - but for a XML calibration contain no channels - but calibration only and returns a vector of calibration objects in new format (mV/nT)
   * \param filename
   * \return
   */
  std::vector<std::shared_ptr<calibration>> read_std_xml_single(const std::filesystem::path &filename);

  void clear();

  std::string get_sensor_name(const std::string name) const;

  std::shared_ptr<tm> cal_date;

private:
  std::string get_units_mtx_old() const;

  ChopperStatus chopper = ChopperStatus::off;
  std::unique_ptr<sqlite_handler> sqldb;

  // use for a guess
  std::string magdate;
  std::string magtime;

  // from file name
  std::string fmagser;
  std::string fmagtype;

  void guess_date();

  bool dbloaded = false;
  std::vector<std::pair<std::string, std::string>> sensor_aliases;

  std::string date_hint;

  std::vector<double> f;
  std::vector<double> a;
  std::vector<double> p;
};

void remove_cal_duplicates(std::vector<std::shared_ptr<calibration>> &v) {
  auto end = v.end();
  for (auto it = v.begin(); it != end; ++it) {
    end = std::remove(it + 1, end, *it);
  }

  v.erase(end, v.end());
}

#endif // READ_CAL_H
