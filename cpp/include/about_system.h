#ifndef ABOUT_SYSTEM_H
#define ABOUT_SYSTEM_H

#include "strings_etc.h"
#include "whereami.h"
#include <chrono>
#include <climits>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <list>
#include <map>
#include <string>
#include <vector>

// std::tm *file_datetime(const std::string &filename) {
//     auto ftime = std::filesystem::last_write_time(filename);
//     auto cftime = std::chrono::system_clock::to_time_t(std::chrono::file_clock::to_sys(ftime));
//     return std::localtime(&cftime);
// }

/*!
 * \brief tm_to_num_date return simply the date numbers fron a std::tm struct
 * \param date
 * \param year
 * \param month
 * \param day
 */

void tm_to_num_date(const std::tm *date, int &year, int &month, int &day) {
  year = date->tm_year + 1900;
  month = date->tm_mon + 1;
  day = date->tm_mday;
}

/*!
 * \brief tm_to_num_date return simply the time numbers fron a std::tm struct
 * \param date
 * \param year
 * \param month
 * \param day
 */
void tm_to_num_time(const std::tm *date, int &hour, int &min, int &sec) {
  hour = date->tm_hour;
  min = date->tm_min;
  sec = date->tm_sec;
}

/*!
 * \brief tm_to_str_date simple conversion to something like 2021-05-19
 * \param date
 * \return
 */
std::string tm_to_str_date(const std::shared_ptr<tm> &date) {
  return std::to_string(date->tm_year + 1900) + "-" + mstr::zero_fill_field(date->tm_mon + 1, 2) + "-" + mstr::zero_fill_field(date->tm_mday, 2);
}

/*!
 * \brief tm_to_str_time simple conversion to something like 14:22:50
 * \param date
 * \return
 */
std::string tm_to_str_time(const std::tm *date) {
  return mstr::zero_fill_field(date->tm_hour, 2) + ":" + mstr::zero_fill_field(date->tm_min + 1, 2) + ":" + mstr::zero_fill_field(date->tm_sec, 2);
}

/*!
 * \brief time_from_ints create a std::tm from numbers
 * \param YYYY
 * \param MM
 * \param DD
 * \param hh
 * \param mm
 * \param ss
 * \return
 */
std::shared_ptr<tm> time_from_ints(const int YYYY = 0, const int MM = 0, const int DD = 0, const int hh = 0, const int mm = 0, const int ss = 0) {
  std::shared_ptr<tm> dt = std::make_shared<tm>();
  dt->tm_year = YYYY - 1900; // Years from 1900
  dt->tm_mon = MM - 1;       // 0-based
  dt->tm_mday = DD;          // 1 based

  dt->tm_hour = hh;
  dt->tm_min = mm;
  dt->tm_sec = ss;
  dt->tm_isdst = 0;
  std::mktime(dt.get());

  return dt;
}

std::filesystem::path get_data_dir() {
  // that is the path to the executable and should be inside the bin folder, on mac below the app bundle
  std::filesystem::path result(get_exec_dir());
  // cd up until we have one above the bin folder
  if (!std::filesystem::exists(result))
    return std::filesystem::path();
  // now loop cd up until we have the data folder
  for (;;) {
    if (!result.has_parent_path())
      return std::filesystem::path();
    else
      result = result.parent_path();
    if (std::filesystem::exists(result / "data")) {
      result /= "data";
      break;
    }
  }
  return std::filesystem::canonical(result);
}

/*!
 * \brief working_dir get dirs and files of procmt, size will be 0 if not found!
 * \param append_dir append a subdirectory without slash
 * \param filename append a file in case
 * \return either the current working dir of procmt, with subfolder and file in case, if file check existance
 */
std::filesystem::path working_dir_data(const std::string filename) {
  std::filesystem::path result(get_exec_dir());
  if (!std::filesystem::exists(result))
    return std::filesystem::path();

  // ..... /bin/atstools
  // ..... /bin

  if (!result.has_parent_path())
    return std::filesystem::path();
  else
    result = result.parent_path();
  // .....
  if (!result.has_parent_path())
    return std::filesystem::path();
  else
    result = result.parent_path();

  // ..... /data
  result /= "data";

  if (!std::filesystem::exists(result)) {
    if (const char *env_p = std::getenv("MTHotel_data")) {
      std::cout << "Your PATH is: " << env_p << '\n';
      result = std::string(env_p);
    }
  }

  // procmt we have procmt/bin procmt/data and so on - want maybe data info.sql3 or cal MFS.txt

  result /= filename;
  if (!std::filesystem::exists(result)) {
    return std::filesystem::path();
  }

  return std::filesystem::canonical(result);
}

/*!
 * \brief sort_xml_and_files sort the path part - wich is concludent to the channel C00 ... C99 part of the filename
 * \param xmls_and_files
 */

void sort_xml_and_files(std::multimap<std::string, std::filesystem::path> &xmls_and_files) {

  std::vector<std::string> xml_files;

  // get all unique keys
  for (const auto &x : xmls_and_files) {
    // std::cout<< x.first <<":"<< x.second << std::endl;
    if (!xml_files.size())
      xml_files.push_back(x.first); // init
    else if (xml_files.back() != x.first) {
      xml_files.push_back(x.first);
    }
  }

  std::multimap<std::string, std::filesystem::path> new_xmls_and_files;
  for (auto const &xmlfile : xml_files) {
    // get all values for a unique key
    auto set_itr = xmls_and_files.equal_range(xmlfile);
    std::vector<std::filesystem::path> vfsp;
    for (auto it = set_itr.first; it != set_itr.second; ++it) {
      vfsp.emplace_back(it->second);
    }
    // sort by name - which should be the channel for ats files
    std::sort(vfsp.begin(), vfsp.end());
    for (auto const &fsp : vfsp) {
      new_xmls_and_files.emplace(xmlfile, fsp);
    }
    vfsp.clear();
  }

  std::swap(xmls_and_files, new_xmls_and_files);
}

/*!
 * @brief
 * @param in_cmd sends a command to the shell and returns the output as a list of strings
 * @param timeout terminates the command after timeout seconds
 * @return list of strings
 */
std::list<std::string> GetStdoutFromCommand(const std::string &in_cmd, const std::chrono::seconds &timeout = std::chrono::seconds(5)) {
  std::string cmd(in_cmd);
  std::list<std::string> data;
  FILE *stream;
  const int max_buffer = 8192;
  char buffer[max_buffer];
  cmd.append(" 2>&1");
  stream = popen(cmd.c_str(), "r");
  // timeout watchdog
  // get the actual time
  auto start = std::chrono::steady_clock::now();
  if (stream) {
    while (!feof(stream)) {
      if (fgets(buffer, max_buffer, stream) != NULL)
        data.emplace_back(buffer);
      // compare the time with duration
      auto end = std::chrono::steady_clock::now();
      if (std::chrono::duration_cast<std::chrono::seconds>(end - start) > timeout) {
        std::cout << "timeout" << std::endl;
        break;
      }
    }
    pclose(stream);
  }
  return data;
}

/*!
 * @brief find an executable in a list of paths; if not found try which
 * @param paths list of paths
 * @param p program name
 * @return path to the executable or empty path
 */
std::filesystem::path check_executable(const std::list<std::filesystem::path> &paths, const std::string &p) {
  for (const auto &path : paths) {
    std::filesystem::path pp = path / p;
    if (std::filesystem::exists(pp) && std::filesystem::is_regular_file(pp)) {
      return pp;
    }
  }
  std::list<std::string> result = GetStdoutFromCommand("which " + p);
  if (result.size() == 1) {
    if (std::filesystem::exists(result.front()) && std::filesystem::is_regular_file(result.front())) {
      return std::filesystem::path(result.front());
    }
  }
  return std::filesystem::path(); // return empty path
}

#endif
