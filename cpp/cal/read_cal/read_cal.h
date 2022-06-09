#ifndef READ_CAL_H
#define READ_CAL_H

#include <string>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <filesystem>
#include <utility>
#include <exception>
#include <memory>
#include <cfloat>

namespace fs = std::filesystem;


#include "../../include/mt_base.h"
#include "../../include/cal_base.h"
#include "../../include/base_xml.h"
#include "../../include/strings_etc.h"
#include "../../include/about_system.h"
#include "../../sqlite_handler/sqlite_handler.h"
#include "../../xml/tinyxmlwriter/tinyxmlwriter.h"
#include "../../xml/tinyxml2/tinyxml2.h"

class read_cal
{
public:
    read_cal();
    ~read_cal();
    bool is_init() const {
        return this->dbloaded;
    }
    std::shared_ptr<calibration> read_std_mtx_txt(const fs::path &filename, const ChopperStatus &chopper);
    /*!
     * \brief read_std_xml reads calibration from a measdoc
     * \param filename e.g. a measdoc.xml with multiple channels
     * \return
     */
    std::vector<std::shared_ptr<calibration>> read_std_xml(const fs::path &filename);

    std::vector<std::shared_ptr<calibration>> read_std_xml_single(const fs::path &filename);


    void clear();

    std::shared_ptr<tm> cal_date;

private:

    std::string get_units_mtx_old() const;

    ChopperStatus chopper = ChopperStatus::off;
    sqlite_handler sqldb;

    // use for a guess
    std::string magdate;
    std::string magtime;

    // from file name
    std::string fmagser;
    std::string fmagtype;


    void guess_date(std::string &datestr);

    bool dbloaded = false;
    std::vector<std::pair<std::string, std::string>> sensor_aliases;

    std::string date_hint;

    std::vector<double> f;
    std::vector<double> a;
    std::vector<double> p;


};

void remove_cal_duplicates(std::vector<std::shared_ptr<calibration>> &v)
{
    auto end = v.end();
    for (auto it = v.begin(); it != end; ++it) {
        end = std::remove(it + 1, end, *it);
    }

    v.erase(end, v.end());
}

#endif // READ_CAL_H
