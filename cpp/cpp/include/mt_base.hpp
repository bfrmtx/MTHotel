#ifndef MT_BASE_HPP
#define MT_BASE_HPP

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

/*!
 * @brief Maximum number of channels; each channel is at a fixed position! As it comes from the data logger
 *
 * ## Details

 * ### Channel
 *
 * A channel is a configuration of a sensor connected to a specific slot (that is the channel number, unique) in the data logger.<br>
 * Each channel has a type (e.g., Ex, Ey, Hx, Hy, Hz, etc.) and is associated with a unique channel number corresponding to its slot.<br>
 * The channel shall contain the calibration data and other metadata specific to the SENSOR connected to that slot.<br>
 * The sensor does NOT appear as a separate entity in the system; it is always part of a channel.<br>
 * We can have Hx[2] and Hx[5] for example; both are different channels connectedOn land, Hx shall be North direction, offshore and airborne it may differ, even vary.<br>
 * ### Run
 *
 * A run represents a recording started at a specific UTC time for a defined duration and defined sample rate [Hz]. atss file size divided by sizeof(double) == samples and divided by sample rate) returns the duration. The duration is NOT stored in the json header.<br>
 *
 * ### Station
 *
 * A station is a physical location where multiple runs are recorded using various channels.<br>
 * Each station can have multiple runs, and each run can contain data from multiple channels.<br>
 * For airborne surveys, a station may represent a specific flight path or area of interest.<br>
 *
 * ### Survey
 *
 * A survey is a collection of stations.<br>
 */
inline constexpr std::size_t max_survey_channels = 16;
// maximum number of runs; change both if needed
inline constexpr std::size_t max_runs = 999; // maximum number of runs per station
inline constexpr std::size_t run_digits = 3; // number of digits for run formatting like run_001
//
#define JSON_MAX_IVALUE 9007199254740991
#define JSON_MIN_IVALUE -9007199254740991
const double mue0 = 4.0 * M_PI * 1.0E-7;
const double min_fft_wl = 64;
const double treat_as_null = 1E-32;
/**
 * @brief Threshold value used to identify out-of-range measurements in EDI files
 *
 * This constant defines the numerical threshold above which data values are
 * considered invalid or out-of-range when processing EDI (Electrical Data Interchange)
 * files. Values greater than or equal to this threshold should be treated as
 * missing or invalid data points.
 *
 * @note This value follows the EDI file format specification for marking
 *       invalid or missing measurements
 */
const double treat_as_out_of_range = 1E+32; //!< EDI file limit for out-of-range values
// vector of strings for channel types Ex, Ey, Hx, Hy, Hz, REx, REy, RHx, RHy, RHz, emap: EEx, EEy; Ez may be needed for sub-marine
// J TX ampere, ... jaw Uy (away from flight direction forward), pitch Up (up & down nose), roll Ur (over the wings "left/right" up & down))
// T for temperature, t for time
static const std::vector<std::string> available_channel_types = {"Ex", "Ey", "Hx", "Hy", "Hz", "REx", "REy", "RHx", "RHy", "RHz", "EEx", "EEy", "Ez", "REz", "EEz", "Jx", "Jy", "Jz", "x", "y", "z", "T", "t", "Uy", "Up", "Ur"};
static const std::vector<std::string> available_ac_spectra_types = {"ExEx", "ExEy", "HxHx", "HxHy", "HxHz", "HyHy", "HyHz", "HzHz"};
static bool is_E(const std::string &channel_type) {
  // check if channel type is E, contains Ex, Ey or Ez as substring
  return channel_type.find("Ex") != std::string::npos || channel_type.find("Ey") != std::string::npos || channel_type.find("Ez") != std::string::npos;
}
static bool is_H(const std::string &channel_type) {
  // check if channel type is H, contains Hx, Hy or Hz as substring
  return channel_type.find("Hx") != std::string::npos || channel_type.find("Hy") != std::string::npos || channel_type.find("Hz") != std::string::npos;
}

/*!
 * @brief two calibration vectors shall overlap at least for 3 frequencies
 */
const inline uint64_t overlapping_cal = 3;

/*!
 * @brief minimum number of frequencies for calibration; 3 already may be used for overlapping with theoretical or master calibration
 */
const inline uint64_t min_cal_size = 6;

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

enum class spc_type : int {
  // single spectra like Ex, Hz ... e.g. the raw input data
  null = -1, //!< null
  Ex = 0,    //!< Ex
  Ey = 1,    //!< Ey
  Ez = 2,    //!< Ez
  Hx = 3,    //!< Hx
  Hy = 4,    //!< Hy
  Hz = 5,    //!< Hz
  REx = 6,   //!< REx // remote spectra
  REy = 7,   //!< REy
  REz = 8,   //!< REz
  RHx = 9,   //!< RHx
  RHy = 10,  //!< RHy
  RHz = 11,  //!< RHz
  EEx = 12,  //!< EEx // EMAP spectra
  EEy = 13,  //!< EEy
  EEz = 14,  //!< EEz
  // now the auto spectra like ExEx, EyEy, HzHz and so on
  ExEx = 15,   //!< ExEx
  EyEy = 16,   //!< EyEy
  EzEz = 17,   //!< EzEz
  HxHx = 18,   //!< HxHx
  HyHy = 19,   //!< HyHy
  HzHz = 20,   //!< HzHz
  RExREx = 21, //!< RExREx // remote auto spectra
  REyREy = 22, //!< REyREy
  REzREz = 23, //!< REzREz
  RHxRHx = 24, //!< RHxRHx
  RHyRHy = 25, //!< RHyRHy
  RHzRHz = 26, //!< RHzRHz
  EExEEx = 27, //!< EExEEx // EMAP auto spectra
  EEyEEy = 28, //!< EEyEEy
  EEzEEz = 29, //!< EEzEEz
  // now the cross spectra like ExEy, ExEz, EyEz and so on
  ExEy = 30,   //!< ExEy
  ExEz = 31,   //!< ExEz
  EyEz = 32,   //!< EyEz
  HxHy = 33,   //!< HxHy
  HxHz = 34,   //!< HxHz
  HyHz = 35,   //!< HyHz
  RExREy = 36, //!< RExREy // remote cross spectra
  RExREz = 37, //!< RExREz
  REyREz = 38, //!< REyREz
  RHxRHy = 39, //!< RHxRHy
  RHxRHz = 40, //!< RHxRHz
  RHyRHz = 41, //!< RHyRHz
  EExEEy = 42, //!< EExEEy // EMAP cross spectra (just for completeness)
  EExEEz = 43, //!< EExEEz
  EEyEEz = 44, //!< EEyEEz
  // now the auto spectra between local and remote like ExREx ... HxRHx, HzRHz
  ExREx = 45, //!< ExREx
  EyREy = 46, //!< EyREy
  EzREz = 47, //!< EzREz
  HxRHx = 48, //!< HxRHx
  HyRHy = 49, //!< HyRHy
  HzRHz = 50, //!< HzRHz
  // now the cross spectra between local and remote like ExREy, ExREz, EyREz
  ExREy = 51, //!< ExREy
  ExREz = 52, //!< ExREz
  EyREz = 53, //!< EyREz
  HxRHy = 54, //!< HxRHy
  HxRHz = 55, //!< HxRHz
  HyRHz = 56, //!< HyRHz
  // now the auto spectra between local and emap like ExEEx ... HxHHx, HzEEz
  ExEEx = 57, //!< ExEEx
  EyEEy = 58, //!< EyEEy
  EzEEz = 59, //!< EzEEz
  unknown = 60
};

// how a function to generate the name of the spectra from the enum as std::pair<std::string, std::string> would look like
inline std::pair<std::string, std::string> get_name_from_spc_enum(const spc_type type) {
  switch (type) {
  case spc_type::Ex:
    return {"Ex", ""};
  case spc_type::Ey:
    return {"Ey", ""};
  case spc_type::Ez:
    return {"Ez", ""};
  case spc_type::Hx:
    return {"Hx", ""};
  case spc_type::Hy:
    return {"Hy", ""};
  case spc_type::Hz:
    return {"Hz", ""};
  case spc_type::REx:
    return {"REx", ""};
  case spc_type::REy:
    return {"REy", ""};
  case spc_type::REz:
    return {"REz", ""};
  case spc_type::RHx:
    return {"RHx", ""};
  case spc_type::RHy:
    return {"RHy", ""};
  case spc_type::RHz:
    return {"RHz", ""};
  case spc_type::EEx:
    return {"EEx", ""};
  case spc_type::EEy:
    return {"EEy", ""};
  case spc_type::EEz:
    return {"EEz", ""};
  case spc_type::ExEx:
    return {"ExEx", "ExEx"};
  case spc_type::EyEy:
    return {"EyEy", "EyEy"};
  case spc_type::EzEz:
    return {"EzEz", "EzEz"};
  case spc_type::HxHx:
    return {"HxHx", "HxHx"};
  case spc_type::HyHy:
    return {"HyHy", "HyHy"};
  case spc_type::HzHz:
    return {"HzHz", "HzHz"};
  case spc_type::RExREx:
    return {"RExREx", "RExREx"};
  case spc_type::REyREy:
    return {"REyREy", "REyREy"};
  case spc_type::REzREz:
    return {"REzREz", "REzREz"};
  case spc_type::RHxRHx:
    return {"RHxRHx", "RHxRHx"};
  case spc_type::RHyRHy:
    return {"RHyRHy", "RHyRHy"};
  case spc_type::RHzRHz:
    return {"RHzRHz", "RHzRHz"};
  case spc_type::EExEEx:
    return {"EExEEx", "EExEEx"};
  case spc_type::EEyEEy:
    return {"EEyEEy", "EEyEEy"};
  case spc_type::EEzEEz:
    return {"EEzEEz", "EEzEEz"};
  case spc_type::ExEy:
    return {"ExEy", "ExEy"};
  case spc_type::ExEz:
    return {"ExEz", "ExEz"};
  case spc_type::EyEz:
    return {"EyEz", "EyEz"};
  case spc_type::HxHy:
    return {"HxHy", "HxHy"};
  case spc_type::HxHz:
    return {"HxHz", "HxHz"};
  case spc_type::HyHz:
    return {"HyHz", "HyHz"};
  case spc_type::RExREy:
    return {"RExREy", "RExREy"};
  case spc_type::RExREz:
    return {"RExREz", "RExREz"};
  case spc_type::REyREz:
    return {"REyREz", "REyREz"};
  case spc_type::RHxRHy:
    return {"RHxRHy", "RHxRHy"};
  case spc_type::RHxRHz:
    return {"RHxRHz", "RHxRHz"};
  case spc_type::RHyRHz:
    return {"RHyRHz", "RHyRHz"};
  case spc_type::EExEEy:
    return {"EExEEy", "EExEEy"};
  case spc_type::EExEEz:
    return {"EExEEz", "EExEEz"};
  case spc_type::EEyEEz:
    return {"EEyEEz", "EEyEEz"};
  case spc_type::ExREx:
    return {"ExREx", "ExREx"};
  case spc_type::EyREy:
    return {"EyREy", "EyREy"};
  case spc_type::EzREz:
    return {"EzREz", "EzREz"};
  case spc_type::HxRHx:
    return {"HxRHx", "HxRHx"};
  case spc_type::HyRHy:
    return {"HyRHy", "HyRHy"};
  case spc_type::HzRHz:
    return {"HzRHz", "HzRHz"};
  case spc_type::ExREy:
    return {"ExREy", "ExREy"};
  case spc_type::ExREz:
    return {"ExREz", "ExREz"};
  case spc_type::EyREz:
    return {"EyREz", "EyREz"};
  case spc_type::HxRHy:
    return {"HxRHy", "HxRHy"};
  case spc_type::HxRHz:
    return {"HxRHz", "HxRHz"};
  case spc_type::HyRHz:
    return {"HyRHz", "HyRHz"};
  case spc_type::ExEEx:
    return {"ExEEx", "ExEEx"};
  case spc_type::EyEEy:
    return {"EyEEy", "EyEEy"};
  case spc_type::EzEEz:
    return {"EzEEz", "EzEEz"};
  default:
    return {"Unknown", "Unknown"};
  }
}

inline bool is_available_spectra_type(const std::pair<std::string, std::string> &name) {
  spc_type type_start = spc_type::Ex;
  spc_type type_end = spc_type::unknown;
  for (int i = static_cast<int>(type_start); i < static_cast<int>(type_end); ++i) {
    if (get_name_from_spc_enum(static_cast<spc_type>(i)) == name) {
      return true;
    }
  }
  return false;
}

inline enum spc_type get_spc_type_from_name(const std::pair<std::string, std::string> &name) {
  spc_type type_start = spc_type::Ex;
  spc_type type_end = spc_type::unknown;
  for (int i = static_cast<int>(type_start); i < static_cast<int>(type_end); ++i) {
    if (get_name_from_spc_enum(static_cast<spc_type>(i)) == name) {
      return static_cast<spc_type>(i);
    }
  }
  return spc_type::null;
}

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
  LF_LP_off = 0,

  MF_RF_1 = 32, //! 0x40 ADU07   MF-RF-1 filter on MF board with capacitor 470nF
  MF_RF_2 = 64, //! 0x20 ADU07   MF-RF-2 filter on MF board with capacitor 4.7nF

  // HF Path
  // 1 Hz has been dropped for 08, default for 07
  HF_HP_1Hz = 1, //! 0x01 ADU07   HF-HP-1Hz 1Hz filter enable for HF board
  // 500Hz is the HP for 08 default, you can switch off
  HF_HP_500Hz = 2, //! 0x02 ADU08   HF-HP-500Hz 500Hz filter enable for HF board
  HH_HP_off = 0,

  div_1 = 1, //! default for E
  div_8 = 8, //! default for H and +/- 10V

  off = 0,

  gain_1_1 = 1,
  gain_1_4 = 4,
  gain_1_8 = 8,
  gain_1_16 = 16,
  gain_1_32 = 32,
  gain_1_64 = 64,

  gain_2_1 = 1,
  gain_2_4 = 4,
  gain_2_8 = 8,
  gain_2_16 = 16,
  gain_2_32 = 32,
  gain_2_64 = 64,

  HF = 1, //!< HF board
  LF = 2, //!< LF board
  MF = 3, //!< MF board

  direct_mode_on = 1, //!< direct mode
  direct_mode_off = 0 //!< direct mode

};

inline void xset_filter(const std::string &system, const std::string &board_and_filters, ADU &radio_filter, ADU &lp_filter, ADU &hp_filter, ADU &input_div, ADU &gain_1, ADU &gain_2) {
  // boards and filters is a comma separated string, split
  std::vector<std::string> filters;
  std::stringstream ss(board_and_filters);
  std::string item;
  while (std::getline(ss, item, ',')) {
    filters.push_back(item);
  }
  if (system == "ADU-08e") {
    for (const auto &filter : filters) {
      if (filter == "LF-RF-1")
        radio_filter = ADU::LF_RF_1;
      else if (filter == "LF-RF-2")
        radio_filter = ADU::LF_RF_2;
      else if (filter == "LF-RF-3")
        radio_filter = ADU::LF_RF_3;
      else if (filter == "LF-RF-4")
        radio_filter = ADU::LF_RF_4;
      else if (filter == "LF-LP-4Hz")
        lp_filter = ADU::LF_LP_4Hz;
      else if (filter == "MF-RF-1")
        radio_filter = ADU::MF_RF_1;
      else if (filter == "MF-RF-2")
        radio_filter = ADU::MF_RF_2;
      else if (filter == "HF-HP-1Hz")
        hp_filter = ADU::HF_HP_1Hz;
      else if (filter == "HF-HP-500Hz")
        hp_filter = ADU::HF_HP_500Hz;
      else if (filter == "div-1")
        input_div = ADU::div_1;
      else if (filter == "div-8")
        input_div = ADU::div_8;
    }
  } else if (system == "ADU-07") {
    for (const auto &filter : filters) {
      if (filter == "LF-RF-1")
        radio_filter = ADU::LF_RF_1;
      else if (filter == "LF-RF-2")
        radio_filter = ADU::LF_RF_2;
      else if (filter == "LF-RF-3")
        radio_filter = ADU::LF_RF_3;
      else if (filter == "LF-RF-4")
        radio_filter = ADU::LF_RF_4;
      else if (filter == "LF-LP-4Hz")
        lp_filter = ADU::LF_LP_4Hz;
      else if (filter == "MF-RF-1")
        radio_filter = ADU::MF_RF_1;
      else if (filter == "MF-RF-2")
        radio_filter = ADU::MF_RF_2;
      else if (filter == "HF-HP-1Hz")
        hp_filter = ADU::HF_HP_1Hz;
      else if (filter == "div-1")
        input_div = ADU::div_1;
      else if (filter == "div-8")
        input_div = ADU::div_8;
    }
  }
}

inline std::vector<std::string> survey_dirs_old() {

  return std::vector<std::string>({"cal", "config", "db", "dump", "edi", "filters", "jle", "jobs", "log", "processings", "shell", "tmp", "ts"});
}

inline std::vector<std::string> survey_sites_stations(const std::filesystem::path survey_top_dir, const std::string ts_or_stations) {
  std::vector<std::string> names;
  if (!std::filesystem::exists(survey_top_dir))
    return names;
  auto svd = survey_top_dir;
  svd /= "ts";
  if (!std::filesystem::exists(svd)) {
    svd = survey_top_dir;
    svd /= "stations";
  }
  if (!std::filesystem::exists(svd)) {
    return names;
  }
  // cd
  // find all site / stations
  for (const auto &entry : std::filesystem::directory_iterator(svd)) {
    // std::cout << entry.path() << std::endl;
    if (std::filesystem::is_directory(entry)) {
      names.emplace_back(entry.path().stem());
    }
  }

  return names;
}

inline std::vector<std::string>
survey_dirs() {
  // filters would also contain calibration functions of coils and boards
  // they are multiplied all together in order to get a final filter / calibration
  // meta information with log from the system etc, mirror of stations; espicially when data was converted from old files
  return std::vector<std::string>({"config", "db", "reports", "dump", "edi", "filters",
                                   "jle", "jobs", "log", "processings", "shell", "tmp", "stations", "meta"});
}

/*!
 * \brief create_survey_dirs
 * \param survey a directory like /survey/nm or deeper like /survey/nm/test
 * \param sub_dirs vector of subdirs to be created; if contains "ts" old survey is assumed (sites), otherwise new survey (stations)
 * \param stations
 * \return
 */
inline bool create_survey_dirs(const std::filesystem::path survey, const std::vector<std::string> sub_dirs, const std::vector<std::string> stations = {}) {
  if (!sub_dirs.size()) {
    std::ostringstream err_str(__func__, std::ios_base::ate);
    err_str << ":: no sub dirs (stations) provided! ->";
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
    err_str << ":: error creating sub directories or main directory -> " << survey << std::endl;
    err_str << "check path and permissions!";
    throw std::runtime_error(err_str.str());
  }

  return true;
}

#endif
