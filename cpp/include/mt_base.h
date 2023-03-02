
#ifndef MT_BASE_H
#define MT_BASE_H

#include <cstdint>
#include <climits>
#include <string>
#include <vector>
#include <memory>
#include <fstream>
#include <filesystem>
// #define _USE_MATH_DEFINES // for C++ and MSVC
#include <cmath>


/*!
 * @file mt_base.h contains all definitions used; all 64bit in order to be comparable with standard variables
 *
 * Basic JSON Formats
 * double: A double-precision 64-bit IEEE 754 floating point.
 * string: a string as UTF-8
 * string date: An RFC3339 date in the format YYYY-MM-DD.
 * boolean  A boolean value, either "true" or "false"
 * date An RFC3339 date in the format YYYY-MM-DD
 * date-time An RFC3339 timestamp in UTC time. This is formatted as YYYY-MM-DDThh:mm:ss.fffZ. The milliseconds portion (".fff") is optional. For higher sampling rates this value may be rounded in case
 *
 * INTEGER: int64_t +/- 9,223,372,036,854,775,808 (exclusive), uint64_t + 9,223,372,036,854,775,808 (inclusive)
 * are NOT part of JSON, max is 2^53 !! however we may not exceed 53 signed
 * int max 2^53  +/- 9,007,199,254,740,991
 * uint max 2^53 +   9,007,199,254,740,992
 * (compare 32bit)           4,294,967,296
 */

#define JSON_MAX_IVALUE 9007199254740991

// I use Capital CamelCase for enums and defines
// I use lowcase under_score seperated for structs and classes





//!< @enum CalibrationType contains various definitions of calibration FORMATS
enum class CalibrationType: int {
    no  = 0,                  //!< does not need or has a calibration
    mtx = 1,                  //!< mtx format: f [Hz], amplitude [mV/nT)],    phase [deg] (0...360)
    mtx_old = 2,              //!< mtx format: f [Hz], amplitude [V/(nT*Hz)], phase [deg] (0...360)
    nn  = 3,                  //!< f [Hz], amplitude [V/(nT)], phase [deg] (0...360)
    scalar = 4                //!< multiply by factor only

};

enum class ChopperStatus: int {
    off = 0,          //!< for all sensors as default, an for metronix HF mode
    on = 1,           //!< that may be true for metronix coils only, LF mode
};


//!< @enum tns provides access to the MT tensor elements
enum class tns : std::size_t {
    xx = 0,                   //!< zxx component
    xy = 1,                   //!< zxy component and coherency for the tensor row xx xy
    yx = 2,                   //!< zyx component and coherency for the tensor row yx yy
    yy = 3,                   //!< zyy component
    tns_size = 4,             //!< Z tensor without tipper (Hz)
    tx = 4,                   //!< tx component
    ty = 5,                   //!< ty component
    tns_tip_size = 6,         //!< Z tensor plus Tipper
    xxxy = 1,                 //!< coherency for the tensor row xx xy
    yxyy = 2,                 //!< coherency for the tensor row yx yy
    tns_scalar = 0            //!< presently scalar NOT supported
};



enum class ADU: int {

    adu08e_rf_1          = 1,            //!< ADU-08e RF-1 on
    adu08e_rf_2          = 2,            //!< ADU-08e RF-2 on
    adu07e_rf_1          = 1,            //!< ADU-07e RF-1 on
    adu07e_rf_2          = 2,            //!< ADU-07e RF-2 on
    adu07e_rf_3          = 3,            //!< ADU-07e RF-3 on
    adu07e_rf_4          = 4,            //!< ADU-07e RF-4 on
    adu08e_rf_off        = 0,            //!< ADU-08e RF off
    adu07e_rf_off        = 0,            //!< ADU-07e RF off
    adu08e_lp4hz_on      = 4,            //!< ADU-08e Low Pass 4Hz on
    adu07e_lp4hz_on      = 4,            //!< ADU-07e Low Pass 4Hz on
    adu08e_lp1hz_on      = 1,            //!< ADU-08e Low Pass 1Hz on
    adu08e_lp_off        = 0,            //!< ADU-08e Low Pass  off
    adu07e_lp_off        = 0,            //!< ADU-07e Low Pass  off
    adu08e_hp500hz_on    = 500,          //!< ADU-08e High Pass 500Hz on
    adu07e_hp500hz_on    = 500,          //!< ADU-08e High Pass 500Hz on
    adu08e_hp1hz_on      = 1,            //!< ADU-08e High Pass 1Hz on
    adu07e_hp1hz_on      = 1,            //!< ADU-07e High Pass 1Hz on
    adu08e_hp_off        = 0,            //!< ADU-08e High Pass off
    adu07e_hp_off        = 0,            //!< ADU-07e High Pass off
    div_1                = 1,            //!< input divider 1 (off direct)
    div_8                = 8,            //!< input divider 8 (on, divides by 8 for +/- 10V extended input, e.g. coils
    apply_hf_spc_cal_sample_f = 8000,   //!< sample frequency above we may want to apply spectral correction of the board
    apply_lf_spc_cal_sample_f = 800     //!< sample frequency above we may want to apply spectral correction of the board


};

std::vector<std::string> survey_dirs() {

    std::vector<std::string> dirs;
    dirs.emplace_back("config"); dirs.emplace_back("db");
    dirs.emplace_back("reports"); dirs.emplace_back("dump"); dirs.emplace_back("edi");
    // filters would also contain calibration functions of coils and boards
    // they are multiplied all together in order to get a final filter / calibration
    dirs.emplace_back("filters"); dirs.emplace_back("jle"); dirs.emplace_back("jobs");
    dirs.emplace_back("log"); dirs.emplace_back("processings");
    dirs.emplace_back("shell"); dirs.emplace_back("tmp"); dirs.emplace_back("stations");
    dirs.emplace_back("meta"); // meta information with log from the system etc, mirror of stations

    return dirs;
}

bool create_survey_dirs(const std::filesystem::path survey, std::vector<std::string> sub_dirs) {
    if (!sub_dirs.size()) {
        std::string err_str = __func__;
        err_str += ":: sub dirs provided! ->";
        throw err_str;
        return false;
    }
    try {
        std::filesystem::create_directory(survey);


        for (const auto str : sub_dirs) {
            auto svd = survey;
            std::filesystem::create_directory((svd /= str));
        }
    }
    catch (...) {
        std::string err_str = __func__;
        err_str += ":: error creating sub directories ->";
        throw err_str;
        return false;
    }

    return true;
}


#endif
