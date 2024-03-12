
#ifndef MT_BASE_H
#define MT_BASE_H

#include <algorithm>
#include <climits>
#include <cstdint>
#include <filesystem>
#include <sstream>
#include <string>
#include <vector>
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
#define JSON_MIN_IVALUE -9007199254740991

/*!
 * @brief two calibration vectors shall overlap at least for 3 frequencies
 */
const static uint64_t overlapping_cal = 3;

/*!
 * @brief minimum number of frequencies for calibration; 3 already may be used for overlapping with theoretical or master calibration
 */
const static uint64_t min_cal_size = 6;

// I use Capital CamelCase for enums and defines
// I use lowcase under_score seperated for structs and classes

//!< @enum CalibrationType contains various definitions of calibration FORMATS
enum class CalibrationType : int {
  no = 0,      //!< does not need or has a calibration
  mtx = 1,     //!< mtx format: f [Hz], amplitude [mV/nT)],    phase [deg] (0...360)
  mtx_old = 2, //!< mtx format: f [Hz], amplitude [V/(nT*Hz)], phase [deg] (0...360)
  nn = 3,      //!< f [Hz], amplitude [V/(nT)], phase [deg] (0...360)
  scalar = 4   //!< multiply by factor only
};

enum class plot_types : int {
  nothing = 0,         //!< nothing
  amplitude = 1,       //!< amplitude
  phase = 2,           //!< phase
  coherency = 3,       //!< coherency
  impedance = 4,       //!< impedance
  calibration = 5,     //!< calibration
  master_cal = 6,      //!< master calibration
  interpolate_cal = 7, //!< interpolated calibration
  theo_cal = 8         //!< theoretical calibration
};

enum class ChopperStatus : int {
  off = 0, //!< for all sensors as default, an for metronix HF mode
  on = 1,  //!< that may be true for metronix coils only, LF mode
};

//!< @enum tns provides access to the MT tensor elements
enum class tns : std::size_t {

  xx = 0,           //!< zxx component
  xy = 1,           //!< zxy component and coherency for the tensor row xx xy
  yx = 2,           //!< zyx component and coherency for the tensor row yx yy
  yy = 3,           //!< zyy component
  tns_size = 4,     //!< Z tensor without tipper (Hz)
  tx = 4,           //!< tx component
  ty = 5,           //!< ty component
  tns_tip_size = 6, //!< Z tensor plus Tipper
  xxxy = 1,         //!< coherency for the tensor row xx xy
  yxyy = 2,         //!< coherency for the tensor row yx yy
  tns_scalar = 0    //!< presently scalar NOT supported
};

enum class ADU : std::uint8_t {
  LF_RF_1 = 1,    //! 0x01 ADU07/8 LF-RF-1 filter on LF board with capacitor 22pF
  LF_RF_2 = 2,    //! 0x02 ADU07/8 LF-RF-2 filter on LF board with capacitor 122pF
  LF_RF_3 = 4,    //! 0x04 ADU07   LF-RF-3 filter on LF board with capacitor 242pF
  LF_RF_4 = 8,    //! 0x08 ADU07   LF-RF-4 filter on LF board with capacitor 342pF
  LF_LP_4Hz = 16, //! 0x10 ADU07/8 LF-LP-4Hz filter on LF board with 4 Hz Lowpass characteristic

  MF_RF_1 = 32, //! 0x40 ADU07   MF-RF-1 filter on MF board with capacitor 470nF
  MF_RF_2 = 64, //! 0x20 ADU07   MF-RF-2 filter on MF board with capacitor 4.7nF

  // HF Path
  // 1 Hz has been dropped for 08
  HF_HP_1Hz = 1, //! 0x01 ADU07   HF-HP-1Hz 1Hz filter enable for HF board
  // 500Hz is the HP for 08
  HF_HP_500Hz = 2, //! 0x02 ADU08   HF-HP-500Hz 500Hz filter enable for HF board

  div_1 = 1, //! default for E
  div_8 = 8, //! default for H and +/- 10V

  off = 0

};

static std::vector<std::string> survey_dirs_old() {

  return std::vector<std::string>({"cal", "config", "db", "dump", "edi", "filters", "jle", "jobs", "log", "processings", "shell", "tmp", "ts"});
}

static std::vector<std::string> survey_dirs() {
  // filters would also contain calibration functions of coils and boards
  // they are multiplied all together in order to get a final filter / calibration
  // meta information with log from the system etc, mirror of stations; espicially when data was converted from old files
  return std::vector<std::string>({"config", "db", "reports", "dump", "edi", "filters",
                                   "jle", "jobs", "log", "processings", "shell", "tmp", "stations", "meta"});
}

/*!
 * \brief create_survey_dirs
 * \param survey a direcory like /survey/nm or deeper like /survey/nm/test
 * \param sub_dirs
 * \param stations
 * \return
 */
static bool create_survey_dirs(const std::filesystem::path survey, const std::vector<std::string> sub_dirs, const std::vector<std::string> stations = {}) {
  if (!sub_dirs.size()) {
    std::ostringstream err_str(__func__, std::ios_base::ate);
    err_str << ":: sub dirs provided! ->";
    throw std::runtime_error(err_str.str());
  }
  try {
    std::filesystem::create_directories(survey);

    for (const auto &str : sub_dirs) {
      auto svd = survey;
      std::filesystem::create_directory((svd /= str));
    }
    if (sub_dirs.size() && stations.size()) {
      auto svd = survey;
      svd /= "ts";
      if (std::find(sub_dirs.begin(), sub_dirs.end(), "ts") != sub_dirs.end()) {
        for (const auto &str : stations) {
          auto svds = svd;
          std::filesystem::create_directory((svds /= str));
        }
      } else if (std::find(sub_dirs.begin(), sub_dirs.end(), "stations") != sub_dirs.end()) {
        for (const auto &str : stations) {
          auto svds = svd;
          std::filesystem::create_directory((svds /= str));
        }
      }
    }
  } catch (...) {
    std::ostringstream err_str(__func__, std::ios_base::ate);
    err_str << ":: error creating sub directories ->";
    throw std::runtime_error(err_str.str());
  }

  return true;
}

#endif
