#ifndef ABOUT_SYSTEM_H
#define ABOUT_SYSTEM_H

#include <chrono>
#include <climits>
#include <cstdint>
#include <filesystem>
#include <map>
#include <string>
#include <vector>
#include "strings_etc.h"

std::tm *file_datetime(const std::string &filename) {
    auto ftime = std::filesystem::last_write_time(filename);
    auto cftime = std::chrono::system_clock::to_time_t(std::chrono::file_clock::to_sys(ftime));
    return std::localtime(&cftime);
}

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

/*!
 * \brief working_dir get dirs and files of procmt, size will be 0 if not found!
 * \param append_dir append a subdirectory without slash
 * \param filename append a file in case
 * \return either the current working dir of procmt, with subfolder and file in case, if file check existance
 */
std::filesystem::path working_dir(const std::string subdir, const std::string filename) {
    auto cup = std::filesystem::current_path();
    auto upp = cup.parent_path();
    std::filesystem::path exec_dir;
    // procmt we have procmt/bin procmt/data and so on - want maybe data info.sql3 or cal MFS.txt
    if (mstr::ends_with(cup.string(), "bin")) {
        exec_dir = cup;
    }
    else {
        exec_dir = upp;
    }
    if (!std::filesystem::is_directory(exec_dir)) {
        exec_dir.clear();
    }

    if (subdir.size()) {
        exec_dir /= subdir;
        if (!std::filesystem::is_directory(exec_dir)) {
            exec_dir.clear();

        }
    }

    if (filename.size()) {
        exec_dir /= filename;
        if (!std::filesystem::exists(exec_dir)) {
            exec_dir.clear();
        }
    }

    return exec_dir;
}

/*!
 * \brief sort_xml_and_files sort the path part - wich is concludent to the channel C00 ... C99 part of the filename
 * \param xmls_and_files
 */

void sort_xml_and_files(std::multimap<std::string, std::filesystem::path> &xmls_and_files) {

    std::vector<std::string> xml_files;

    // get all unique keys
    for(const auto &x: xmls_and_files) {
        // std::cout<< x.first <<":"<< x.second << std::endl;
        if (!xml_files.size())  xml_files.push_back(x.first);  // init
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

#endif
