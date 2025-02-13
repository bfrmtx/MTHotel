#ifndef ATSHEADER_DEF
#define ATSHEADER_DEF

/**
 * @file atsheader_def.h
 * Contains the binary header of ats files AND ats_json
 *
 */

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <iterator>
#include <sstream>
#include <string>
#include <unordered_map>

#include "json.h"
#include "mt_base.h"
#include "strings_etc.h"

namespace fs = std::filesystem;

//
//
//
//         array of chars are not NULL terminated !
//         achChanType   [2] = "Ex" is not NULL terminated
//
//

#define C_ATS_CEA_NUM_HEADERS 1023 /*!< maximum number of 1023 headers of the ATS sliced file.*/

/*!
   \brief The ATSSliceHeader_s struct
   A sliced ats file ALWAYS contains a default header and C_ATS_CEA_NUM_HEADERS (1023) = 1024 total.<br>
   The FIRST and DEFAULT slice contains ALL information (total samples of all slices)<br>
   The 0 ... 1022 slices (=  ATS_CEA_NUM_HEADERS ) contain the individual descriptions<br>
   start tsdata = 400h = 1024th byte<br>
   start of CEA data is 400h + 3FFh * 20h = 83E0h<br>
                        1024 + 1023 * 23  = 33760<br>

assuming 3 slices of 4096 iSamples will be 12,288 samples in the header,
    followed by 3 slice headers with 4096 iSamples each
        and 1019 empty slice headers<br>

                        */

struct ATSSliceHeader_1080 {
  uint32_t uiSamples;         //!< 000h  number of samples for this slice
  uint32_t uiStartDateTime;   //!< 004h  startdate/time as UNIX timestamp (UTC)
  double dbDCOffsetCorrValue; //!< 008h  DC offset correction value in mV
  float rPreGain;             //!< 010h  originally used pre-gain (GainStage1) - LSB is the same for all slices
  float rPostGain;            //!< 014h  originally used post-gain (GainStage2) - LSB is the same for all slices
  std::int8_t DCOffsetCorrOn; //!< 018h  DC offset correction was on/off for this slice
  std::int8_t reserved[7];    //!< 020h  reserved bytes to get to word / dword boundary
};

/*! @todo TX info
  maybe UTM Zone number
  maybe UTM letter
  Tx times six double at least
  Tx base freq

*/

/*!
   \brief The ATSComments_80 struct for ats header 80,81 and 90 <br>
   Some software will automatically convert to higher version<br>
   Some fields are external - the user has to set then; examples:<br>
   northing, easting, UTM Zone : these can be used for high density grids<br>
 */
struct ATSComments_80 {
  char Client[16];     //!< 000h  UTF-8 storage, used in EDI
  char Contractor[16]; //!< 010h  UTF-8 storage, used in EDI
  char Area[16];       //!< 020h  UTF-8 storage, used in EDI
  char SurveyID[16];   //!< 030h  UTF-8 storage, used in EDI
  char Operator[16];   //!< 040h  UTF-8 storage, used in EDI
  char SiteName[112];  //!< 050h  UTF-8 storage, no BOM at the beginning!; WORST case = 28 Chinese/Japanese chars (multibyte chars)
  char XmlHeader[64];  //!< 0C0h  UTF-8 storage  points to the corresponding XML file, containing addtional information for this header; no PATH=XML file is in the same directory as the ats file
  // deleted September 2018 char          achComments   [512];          //!< 100h  UTF-8 storage  comment block starts (comment field can start with "weather:" but has no meaning
  // new September 2018
  char Comments[288];     //!< 100h  UTF-8 storage  comment block starts (comment field can start with "weather:" but has no meaning
  char SiteNameRR[112];   //!< 220h  UTF-8 storage, no BOM at the beginning!; WORST case = 28 Chinese/Japanese chars (multibyte chars)
  char SiteNameEMAP[112]; //!< 290h  UTF-8 storage, no BOM at the beginning!; WORST case = 28 Chinese/Japanese chars (multibyte chars); INDICATES a default EMAP center station, where we expect the H (magnetic)
};

/*!
   \brief The ATSHeader_80 struct<br>
   reference is lat and long; additional entries like northing and easting should be empty // not used<br>
   these entries can be overwritten by the user in case of high density grids <br>
   The site_name (site name, achSiteName, is in the comment field; the site_no, site_num, site_number, site number is nver stored; this is part of external numeration
 */
struct ATSHeader_80 {
  std::uint16_t header_length; //!< 000h  length of header in bytes (default 1024 and  33760 for CEA)
  std::int16_t header_version; //!< 002h  80 for ats, 81 for 64bit possible / metronix, 1080 for CEA / sliced header

  // This information can be found in the ChannelTS datastructure
  uint32_t samples; //!< 004h  amount of samples (each is a 32bit / 64bit int INTEL byte order little endian) in the file total of all slices; if uiSamples == UINT32_MAX, uiSamples64bit at address 0F0h will be used instead; uiSamples64bit replaces uiSamples in that case; do not add!

  float sample_rate;      //!< 008h  sampling frequency in Hz
  uint32_t start;         //!< 00Ch  unix TIMESTAMP (some computers will revert that to negative numbers if this number is grater than 32bit signed); 2106-02-07T06:28:14 is the END of this format
  double lsbval;          //!< 010h  least significant bit in mV ()
  int32_t GMToffset;      //!< 018h  not used, default 0; can be used as "UTC to GMT"
  float orig_sample_rate; //!< 01Ch  sampling frequency in Hz as ORIGINALLY recorded; this value should NOT change (for example after filtering)

  // The required data could probably found in the HardwareConfig
  std::uint16_t serial_number;           //!< 020h  Serial number of the system (logger)
  std::uint16_t serial_number_ADC_board; //!< 022h  Serial number of the ADC board (ADB)
  std::uint8_t channel_number;           //!< 024h  Channel number
  std::uint8_t chopper;                  //!< 025h  Chopper On (1) / Off (0)

  // Data from XML Job-specification
  char channel_type[2];              //!< 026h  (Ex, Ey, Ez, Hx, Hy, Hz, Jx, Jy, Jz, Px, Py, Pz, Rx, Ry, Rz and so on)
  char sensor_type[6];               //!< 028h  (MFS06 MFS06e MFS07 MFS07e MFS10e SHFT02 SHF02e FGS02 FGS03 FGS03e etc. e.g. the "-" in MFS-06e is skipped)
  std::int16_t sensor_serial_number; //!< 02Eh  Serial number of connected sensor

  float x1; //!< 030h  e.g. South negative [m]
  float y1; //!< 034h  e.g. West negative [m]
  float z1; //!< 038h  e.g. top, sky [m]
  float x2; //!< 03Ch  e.g. North positive [m]
  float y2; //!< 040h  e.g. East positive [m]
  float z2; //!< 044h  e.g. bottom, earth [m]

  // not used any more use pos values!!; GUI interfaces using Length and direction MUST calculate pos x,y,z and set above.
  float dipole_length; //!< 048h  e.g. to be calculated; should not be used - my be over written in FUTURE
  // not used any more use pos values!!
  float angle; //!< 04Ch  e.g. to be calculated; should not be used - my be over written in FUTURE

  // Data from Selftest
  float rho_probe_ohm;        //!< 050h  [ohm]
  float DC_offset_voltage_mV; //!< 054h  [mV]
  float gain_stage1;          //!< 058h  e.g. Gain Stage 1
  float gain_stage2;          //!< 05Ch  e.g. Gain Stage 2

  // Data from status information ?
  int32_t iLat_ms;           //!< 060h  must be used, UNIT = milli seconds
  int32_t iLong_ms;          //!< 064h  must be used, UNIT = milli seconds
  int32_t iElev_cm;          //!< 068h  must be used, UNIT = cm
  char Lat_Long_TYPE;        //!< 06Ch  'G' default, 'U' user, GPS should be used
  char coordinate_type;      //!< 06Dh  'U' = UTM, default empty
  std::int16_t ref_meridian; //!< 06Eh  default empty, should not be used (external)

  //!@todo can we store 64bit time and 64bit samples here ??
  double Northing;         //!< 070h  also xcoord should not be used, default 0 (external)
  double Easting;          //!< 078h  also ycoord should not be used  default 0 (external)
  char gps_clock_status;   //!< 080h  '-' unknown, 'N' no fix, 'C' full fix
  char GPS_accuracy;       //!< 081h  '0' - not used, 1 in case GF4-Fix & Syrlinks was active (system was synced before Syrlinks took over)
  std::int16_t offset_UTC; //!< 082h  UTC + iUTCOffset = GPS time for example; can be used for other offsets, not used, default 0
  char SystemType[12];     //!< 084h  MMS03e GMS05 ADU06 ADU07 ADU08 ADU09 ADU10 ADU11 SPAMMKV SPAMMKIII

  // Data from XML-Job specification
  char survey_header_filename[12]; //!< 090h  UTF-8 storage  free for usage
  char type_of_meas[4];            //!< 09Ch  free for usage MT CSMT

  double DCOffsetCorrValue;   //!< 0A0h  DAC offset double
  std::int8_t DCOffsetCorrOn; //!< 0A8h  DC offset was switched on (1) or off(0)
  std::int8_t InputDivOn;     //!< 0A9h  inputput divider on(1) off(0); e.g when coil was connected
  std::int16_t bit_indicator; //!< 0AAh  0 = 32bit int INTEL byte order, 1 = 64bit INTEL byte order, little endian; since atsheader version 81
  char result_selftest[2];    //!< 0ACh  'NO' or 'OK'
  std::uint16_t numslices;    //!< 0AEh  number of slices used (1....1024, 1 is the first, that is list.size() )

  // std::int16_t  calentries    // max 128 entries - not existing

  // Were the following fields ever used ?
  std::int16_t cal_freqs;         //!< 0B0h  not used  (external)
  std::int16_t cal_entry_length;  //!< 0B2h  not used  (external)
  std::int16_t cal_version;       //!< 0B4h  not used  (external)
  std::int16_t cal_start_address; //!< 0B6h  not used, never used  (external)

  char LF_filters[8]; //!< 0B8h  is a bitfield

  char UTMZone[12];             //!< 0C0h  not used  (external)  (formerly abyADU06CalFilename) 32U or 01G, 32UMD7403 : alway NNC od NNCCNNNN
  uint32_t system_cal_datetime; //!< 0CCh  not used  (external)
  char sensor_cal_filename[12]; //!< 0D0h  not used  ("SENSOR.CAL") (external)
  uint32_t sensor_cal_datetime; //!< 0DCh  not used  (external)

  float powerline1;   //!< 0E0h  e.g. empty  (external)
  float powerline2;   //!< 0E4h  e.g. empty  (external)
  char HF_filters[8]; //!< 0E8h  is a bitfield

  // IF uiSamples == UINT32_MAX you find
  uint64_t samples_64bit; //!< 0F0h  amount of samples (each is a 32bit /64bit int) in the file total of all slices; ONLY used in case uiSamples == UINT32_MAX; else 0; REPLACSES the uiSamples; do not add

  // double        OriginalLSBMV;              //!< 0F0h  NOT USED ANYMORE! orig lsb from selftest without gains; used for ADC values
  float external_gain;    //!< 0F8h  for external satellite box
  char ADB_board_type[4]; //!< 0FCh  LF HF or MF or BB

  ATSComments_80 comments; //!< 100h

  // size if comments is total 100h + 100h data  + (512byte comments = 200h) = 400h
  // start tsdata = 400h = 1024th byte
  // start of CEA header is 400h + 3FFh * 20h = 83E0h
  //                        1024 + 1023 * 23  = 33760
};

/*!
 * \brief The ats_sys_names
 */
std::unordered_map<std::string, std::string> ats_sys_names = {
    {"adu06", "ADU-06"},
    {"adu07", "ADU-07e"},
    {"adu08", "ADU-08e"},
    {"adu09", "ADU-09u"},
    {"adu10", "ADU-10e"},
    {"adu11", "ADU-11e"},
    {"adu12", "ADU-12e"},
    {"adu07e", "ADU-08e"},
    {"adu08e", "ADU-08e"},
    {"adu10e", "ADU-10e"},
    {"adu11e", "ADU-11e"},
    {"adu12e", "ADU-12e"}};

/*!
 * \brief The ats_sys_types
 */
std::unordered_map<std::string, int> ats_sys_types = {
    {"ADU-06", 0},
    {"ADU-07e", 0},
    {"ADU-08e", 1},
    {"ADU-09u", 4},
    {"ADU-10e", 4},
    {"ADU-11e", 5},
    {"ADU-12e", 6}};

/*!
 * \brief The ats_sys_family
 */
std::unordered_map<std::string, int> ats_sys_family = {
    {"ADU-06", 6},
    {"ADU-07e", 7},
    {"ADU-08e", 8},
    {"ADU-09u", 9},
    {"ADU-10e", 10},
    {"ADU-11e", 11},
    {"ADU-12e", 12}};

//******************************************  A T S  H E A D E R  J S O N   ************************************
/*!
 * \brief The ats_header_json struct for interacting with json; the class makes use of atsheader
 */
struct ats_header_json {
  nlohmann::ordered_json header;
  ATSHeader_80 atsh;
  std::filesystem::path filename;

  /*!
   * \brief ats_header_json use \ref atsheader for reading the binary file
   * \param ats set the binary ATSHeader_80 via atsheader.read() and finally atsheader.header
   * \param filename like  atsheader.path()
   */
  ats_header_json(const ATSHeader_80 &ats, const fs::path filename = "") {
    this->atsh = ats;
    this->filename = filename;
  }

  ~ats_header_json() {
  }

  ats_header_json(std::shared_ptr<ats_header_json> &rhs) {
    this->filename = rhs->filename;
    this->header = rhs->header;
  }

  std::filesystem::path write_meta(const std::filesystem::path &directory_path_only, const std::string filename) {
    std::filesystem::path filepath(std::filesystem::canonical(directory_path_only));
    filepath /= filename;

    std::ofstream file;
    file.open(filepath, std::fstream::out | std::fstream::trunc);

    if (!file.is_open()) {
      file.close();
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << "::can not write header, file not open " << filepath;
      throw std::runtime_error(err_str.str());
    }

    file << std::setw(2) << this->header << std::endl;
    file.close();

    return filepath;
  }

  std::filesystem::path xml_path() const {
    std::filesystem::path xp(this->filename.parent_path());
    xp += "/";
    xp += this->header["XmlHeader"].get<std::string>();
    return xp;
  }

  std::string start_date() const {
    long int utc = static_cast<int64_t>(this->atsh.start);
    return mstr::iso8601_time_t(utc);
  }

  std::string stop_date() const {
    long double sf_lhs = static_cast<long double>(this->atsh.samples) / static_cast<long double>(this->atsh.sample_rate);
    sf_lhs += static_cast<long double>(this->atsh.start);
    double f = 0.0;
    double intpart;
    f = modf(sf_lhs, &intpart);
    int64_t utc = static_cast<int64_t>(intpart);
    return mstr::iso8601_time_t(utc, 1);
  }

  std::string start_time() const {
    int64_t utc = static_cast<int64_t>(this->atsh.start);
    return mstr::iso8601_time_t(utc, 2);
  }

  std::string start_datetime() const {
    std::string udate, utime;
    int64_t utc = static_cast<int64_t>(this->atsh.start);
    return mstr::iso8601_time_t(utc);
  }
  int64_t secs_since_1970() const {
    return static_cast<int64_t>(this->atsh.start);
  }

  std::string stop_time() const {
    long double sf_lhs = static_cast<long double>(this->atsh.samples) / static_cast<long double>(this->atsh.sample_rate);
    sf_lhs += static_cast<long double>(this->atsh.start);
    double f = 0.0, intpart;
    f = modf(sf_lhs, &intpart);
    int64_t utc = static_cast<int64_t>(intpart);
    return mstr::iso8601_time_t(utc, 2, f);
  }

  double get_lat() const {
    return (this->atsh.iLat_ms / 1000.) / 3600.;
  }

  /*!
   * \brief set_double_lat
   * \param d ISO 6709, North latitude is positive, decimal fractions
   */
  void set_lat(const double &d) {
    this->atsh.iLat_ms = static_cast<int32_t>(d * 3600000.);
  }

  double get_lon() const {
    return (this->atsh.iLong_ms / 1000.) / 3600.;
  }

  /*!
   * \brief set_double_lon
   * \param d ISO 6709, East longitude is positive, decimal fractions
   */
  void set_lon(const double &d) {
    this->atsh.iLong_ms = static_cast<int32_t>(d * 3600000.);
  }

  /*!
   * \brief get_elev elevation in meter
   * \return
   */
  double get_elev() const {
    return (this->atsh.iElev_cm / 100.);
  }

  void set_elev(const double &d) {
    this->atsh.iElev_cm = static_cast<int32_t>(d * 100.);
  }

  /*!
   * \brief pos2length calculate dipole length
   * \return
   */
  double pos2length() const {
    double tx, ty, tz;
    tx = double(this->atsh.x2 - this->atsh.x1);
    ty = double(this->atsh.y2 - this->atsh.y1);
    tz = double(this->atsh.z2 - this->atsh.z1);

    double diplength = sqrt(tx * tx + ty * ty + tz * tz);
    if (diplength < 0.001)
      diplength = 0; // avoid rounding errors
    return diplength;
  }

  /*!
   * \brief pos2angle calculate angle for North to East
   * \return
   */
  double pos2angle() const {

    if (!this->header.contains("channel_type")) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << "::header not existing; read file and use get_ats_header() ";
      throw std::runtime_error(err_str.str());
    }
    double tx, ty;
    tx = double(this->atsh.x2 - this->atsh.x1);
    ty = double(this->atsh.y2 - this->atsh.y1);

    double diplength = this->pos2length();

    // avoid calculation noise
    if (std::abs(tx) < 0.001)
      tx = 0.0;
    if (std::abs(ty) < 0.001)
      ty = 0.0;

    // many user do not set coordiantes for the coils
    // Hx
    if ((diplength == 0) && (this->header["channel_type"].get<std::string>() == "Hx")) {
      return 0.; // NORTH
    }
    // Hy
    if ((diplength == 0) && (this->header["channel_type"].get<std::string>() == "Hy")) {
      return 90.; // EAST
    }
    // Hz
    if ((diplength == 0) && (this->header["channel_type"].get<std::string>() == "Hz")) {
      return 0.;
    }

    if ((tx == 0) && (ty == 0))
      return 0;

    // hmm hmm possible but you normally set the system N S E W
    double ang = atan2(ty, tx) * 180.0 / M_PI;

    // let angle from position snap
    if ((ang < 90.01) && (ang > 89.99))
      return 90.;
    if ((ang < 0.01) && (ang > 359.99))
      return 0.;
    if ((ang < 180.01) && (ang > 179.99))
      return 180.;
    if ((ang < 270.01) && (ang > 269.99))
      return 270.;

    return ang;
  }

  /*!
   * \brief get_filter
   * \param init like "ADB" - we may other board names in future
   * \return
   */
  std::string get_filter(const std::string init) const {
    std::string filter(init);
    filter += this->header["ADB_board_type"];
    if (this->header["LF_filters"].get<std::string>().size()) {
      filter += ",";
      filter += this->header["LF_filters"];
    }
    if (this->header["HF_filters"].get<std::string>().size()) {
      filter += ",";
      filter += this->header["HF_filters"];
    }

    return filter;
  }
  bool can_and_want_scale() const {

    if (!this->header.contains("channel_type")) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << "::header not existing; read file and use get_ats_header() ";
      throw std::runtime_error(err_str.str());
    }
    if (this->pos2length() == 0.0)
      return false;
    std::vector<std::string> types;
    types.emplace_back("Ex");
    types.emplace_back("Ey");
    types.emplace_back("Ez");

    std::string mytpe = this->header["channel_type"];
    for (const auto &type : types) {
      if (type == mytpe)
        return true;
    }

    return false;
  }

  /*!
   * \brief pos2tilt
   * \return tilt angle; 90 = positive downwards, 0 = horizontal
   */
  double pos2tilt() const {

    double tz = double(this->atsh.z2 - this->atsh.z1);

    double diplength = this->pos2length();
    // no coordinates given for Hz
    if ((diplength == 0) && (this->header["channel_type"].get<std::string>() == "Hz")) {
      return 90.0;
    }

    if (diplength == 0)
      return 0.0; // horizontal - no length
    if (tz < 0.001)
      return 0.0; // no z component

    //! @todo that is maybe wrong
    double ang = 90.0 - acos(tz / diplength) * 180.0 / M_PI;
    if ((ang < 0.01) && (ang > 359.99))
      return 0.;
    if ((ang < 90.01) && (ang > 89.99))
      return 90.;

    return ang;
  }

  void dip2pos(const double &length, const double &angle_north_to_east) {

    double tx = 0.5 * length * cos(angle_north_to_east * M_PI / 180.0); // North
    double ty = 0.5 * length * sin(angle_north_to_east * M_PI / 180.0); // East

    this->header["x1"] = -tx;
    this->header["y1"] = -ty;
    this->header["x2"] = tx;
    this->header["y2"] = ty;
  }

  void dip2z(const double &length, const double &tilt) {
    double tz = length * sin(tilt * M_PI / 180.0);
    this->header["z1"] = 0.0;
    this->header["z2"] = tz;
  }

  std::string measdir() const {

    return mstr::measdir_time(static_cast<int64_t>(this->atsh.start));
  }

  ChopperStatus get_chopper() const {
    if (!this->header.size())
      return ChopperStatus::off;
    if (this->header["chopper"] == 1)
      return ChopperStatus::on;
    return ChopperStatus::off;
  }

  int64_t get_run() const {
    if (!this->filename.string().size())
      return 0;
    std::string base = this->filename.stem().string();

    int64_t irun = 0;
    auto tokens = mstr::split(base, '_');
    for (auto &token : tokens) {
      if (token.starts_with('R') || token.starts_with('r')) {
        try {
          auto rstr = token.substr(1);
          irun = std::stoi(rstr);
        } catch (...) {
          irun = 0;
        }
      }
    }

    return irun;
  }

  std::unordered_map<std::string, ADU> LF_Filters;
  std::unordered_map<std::string, ADU> HF_Filters;

  std::vector<uint8_t> LFFilter_to_ints() const {
    std::vector<uint8_t> filters(8, 0);
    size_t i = 0;
    for (i = 0; i < sizeof(this->atsh.LF_filters); ++i) {
      filters[i] = static_cast<int8_t>(this->atsh.LF_filters[i]);
    }
    return filters;
  }

  std::vector<uint8_t> HFFilter_to_ints() const {
    std::vector<uint8_t> filters(8, 0);
    size_t i = 0;
    for (i = 0; i < sizeof(this->atsh.HF_filters); ++i) {
      filters[i] = static_cast<int8_t>(this->atsh.HF_filters[i]);
    }
    return filters;
  }

  /*!
   * @brief Set the filter bank object
   *
   * @param adu_gen like 7 8 9 10 11
   * @return size_t successfully created filters
   */

  size_t set_filter_bank(const std::string &ADUin) {
    std::string ADU(ADUin);
    if (ADUin == "ADU-06")
      ADU = "ADU-07e";
    if (ADUin == "ADU-07")
      ADU = "ADU-07e";
    this->LF_Filters.clear();
    this->HF_Filters.clear();

    if ((ADU == "ADU-07e") || (ADU == "ADU-08e"))
      this->LF_Filters["LF-RF-1"] = ADU::LF_RF_1; //! 0x01 ADU07/8 LF-RF-1 filter on LF board with capacitor 22pF
    if ((ADU == "ADU-07e") || (ADU == "ADU-08e"))
      this->LF_Filters["LF-RF-2"] = ADU::LF_RF_2; //! 0x02 ADU07/8 LF-RF-2 filter on LF board with capacitor 122pF
    if ((ADU == "ADU-07e"))
      this->LF_Filters["LF-RF-3"] = ADU::LF_RF_3; //! 0x04 ADU07   LF-RF-3 filter on LF board with capacitor 242pF
    if ((ADU == "ADU-07e"))
      this->LF_Filters["LF-RF-4"] = ADU::LF_RF_4; //! 0x08 ADU07   LF-RF-4 filter on LF board with capacitor 342pF
    if ((ADU == "ADU-07e") || (ADU == "ADU-08e"))
      this->LF_Filters["LF-LP-4Hz"] = ADU::LF_LP_4Hz; //! 0x10 ADU07/8 LF-LP-4Hz filter on LF board with 4 Hz Lowpass characteristic

    if ((ADU == "ADU-07e"))
      this->LF_Filters["MF-RF-1"] = ADU::MF_RF_1; //! 0x40 ADU07   MF-RF-1 filter on MF board with capacitor 470nF
    if ((ADU == "ADU-07e"))
      this->LF_Filters["MF-RF-2"] = ADU::MF_RF_2; //! 0x20 ADU07   MF-RF-2 filter on MF board with capacitor 4.7nF

    // HF Path
    // 1 Hz has been dropped for 08
    if ((ADU == "ADU-07e"))
      this->HF_Filters["HF-HP-1Hz"] = ADU::HF_HP_1Hz; //! 0x01 ADU07   HF-HP-1Hz 1Hz filter enable for HF board
    // 500Hz is the HP for 08
    if ((ADU == "ADU-08e"))
      this->HF_Filters["HF-HP-500Hz"] = ADU::HF_HP_500Hz; //! 0x02 ADU08   HF-HP-500Hz 500Hz filter enable for HF board

    return this->LF_Filters.size() + this->HF_Filters.size();

    // for LF filter may visible above 800 Hz
    // for HF filter may be visible above 8000 Hz
  }

  std::string get_lf_filter_strings() const {
    std::string sfilter;
    std::vector<uint8_t> filters(this->LFFilter_to_ints());

    // the header uses only the FIRST int
    std::map<ADU, std::string> rfilters;
    for (const auto &it : this->LF_Filters) {
      rfilters[it.second] = it.first;
    }

    for (auto it = rfilters.crbegin(); it != rfilters.crend(); ++it) {
      if (filters[0] >= uint8_t(it->first)) {
        if (sfilter.size())
          sfilter += ",";
        sfilter += it->second;
        filters[0] -= uint8_t(it->first);
      }
    }

    return sfilter;
  }

  std::string get_hf_filter_strings() const {
    std::string sfilter;
    std::vector<uint8_t> filters(this->HFFilter_to_ints());

    // the header uses only the FIRST int
    std::map<ADU, std::string> rfilters;
    for (const auto &it : this->HF_Filters) {
      rfilters[it.second] = it.first;
    }

    for (auto it = rfilters.crbegin(); it != rfilters.crend(); ++it) {
      if (filters[0] >= uint8_t(it->first)) {
        if (sfilter.size())
          sfilter += ",";
        sfilter += it->second;
        filters[0] -= uint8_t(it->first);
      }
    }

    return sfilter;
  }

  /*!
   * \brief set_hf_filter_int
   * \param cs_string comma separated list of filters
   */
  void set_hf_filter_int(const std::string &cs_string) {
    std::vector<uint8_t> filters(8, 0);

    for (const auto &it : this->HF_Filters) {
      if (mstr::contains(cs_string, it.first))
        filters[0] += uint8_t(it.second);
    }

    for (size_t i = 0; i < sizeof(this->atsh.HF_filters); ++i) {
      this->atsh.HF_filters[i] = static_cast<char>(filters[i]);
    }
  }

  void set_lf_filter_int(const std::string &cs_string) {
    std::vector<uint8_t> filters(8, 0);

    for (const auto &it : this->LF_Filters) {
      if (mstr::contains(cs_string, it.first))
        filters[0] += uint8_t(it.second);
    }

    for (size_t i = 0; i < sizeof(this->atsh.LF_filters); ++i) {
      this->atsh.LF_filters[i] = static_cast<char>(filters[i]);
    }
  }

  /*!
   * \brief get_ats_header gets the binary ats header into the JSON
   */
  void get_ats_header() {
    this->header["header_length"] = static_cast<int64_t>(this->atsh.header_length);
    this->header["header_version"] = static_cast<int64_t>(this->atsh.header_version);

    this->header["samples"] = static_cast<int64_t>(this->atsh.samples);
    this->header["sample_rate"] = static_cast<double>(this->atsh.sample_rate);
    this->header["start"] = static_cast<int64_t>(this->atsh.start);
    this->header["lsbval"] = static_cast<double>(this->atsh.lsbval);
    this->header["GMToffset"] = static_cast<int64_t>(this->atsh.GMToffset);
    this->header["orig_sample_rate"] = static_cast<double>(this->atsh.orig_sample_rate);

    this->header["serial_number"] = static_cast<int64_t>(this->atsh.serial_number);
    this->header["serial_number_ADC_board"] = static_cast<int64_t>(this->atsh.serial_number_ADC_board);
    this->header["channel_number"] = static_cast<int64_t>(this->atsh.channel_number);
    this->header["chopper"] = static_cast<int64_t>(this->atsh.chopper);

    // make sure that we have Ex .. Hy in camel case
    std::string tch = mstr::clean_bc_str(this->atsh.channel_type, 2);
    // x,y,z,T,t
    if (tch.size() == 1)
      this->header["channel_type"] = tch;
    else {
      std::transform(tch.begin(), tch.begin() + 1, tch.begin(), ::toupper);
      std::transform(tch.begin() + 1, tch.end(), tch.begin() + 1, ::tolower);
      this->header["channel_type"] = tch;
    }
    this->header["sensor_type"] = mstr::clean_bc_str(this->atsh.sensor_type, 6);
    this->header["sensor_serial_number"] = static_cast<int64_t>(this->atsh.sensor_serial_number);

    this->header["x1"] = static_cast<double>(this->atsh.x1);
    this->header["y1"] = static_cast<double>(this->atsh.y1);
    this->header["z1"] = static_cast<double>(this->atsh.z1);
    this->header["x2"] = static_cast<double>(this->atsh.x2);
    this->header["y2"] = static_cast<double>(this->atsh.y2);
    this->header["z2"] = static_cast<double>(this->atsh.z2);

    // pos and diplength not supported since 2004 - inconsistency removed

    this->header["rho_probe_ohm"] = static_cast<double>(this->atsh.rho_probe_ohm);
    this->header["DC_offset_voltage_mV"] = static_cast<double>(this->atsh.DC_offset_voltage_mV);
    this->header["gain_stage1"] = static_cast<double>(this->atsh.gain_stage1);
    this->header["gain_stage2"] = static_cast<double>(this->atsh.gain_stage2);

    // Data from status information ?
    this->header["iLat_ms"] = static_cast<int64_t>(this->atsh.iLat_ms);
    this->header["iLong_ms"] = static_cast<int64_t>(this->atsh.iLong_ms);
    this->header["iElev_cm"] = static_cast<int64_t>(this->atsh.iElev_cm);
    this->header["Lat_Long_TYPE"] = std::string(1, this->atsh.Lat_Long_TYPE);
    this->header["coordinate_type"] = std::string(1, this->atsh.coordinate_type);
    this->header["ref_meridian"] = static_cast<int64_t>(this->atsh.ref_meridian);

    this->header["Northing"] = static_cast<double>(this->atsh.Northing);
    this->header["Easting"] = static_cast<double>(this->atsh.Easting);
    this->header["gps_clock_status"] = std::string(1, this->atsh.gps_clock_status);
    this->header["GPS_accuracy"] = std::string(1, this->atsh.GPS_accuracy);
    this->header["offset_UTC"] = static_cast<int64_t>(this->atsh.offset_UTC);

    int TypeNo = 0;
    int GMSno = 0;
    std::string Name("unknown");

    auto str1 = mstr::clean_bc_str(this->atsh.SystemType, 12);
    std::transform(str1.begin(), str1.end(), str1.begin(), ::tolower);

    try {
      Name = ats_sys_names.at(str1);
    } catch (...) {
      Name = "unknown";
    }
    try {
      TypeNo = ats_sys_types.at(Name);
    } catch (...) {
      TypeNo = 0;
    }
    try {
      GMSno = ats_sys_family.at(Name);
    } catch (...) {
      GMSno = 0;
    }

    this->header["SystemType"] = Name;
    // beside from binary header
    this->header["GMSno"] = GMSno;
    this->header["TypeNo"] = TypeNo;
    this->header["ats_data_file"] = this->filename.filename();
    //
    set_filter_bank(Name);

    // Data from XML-Job specification
    this->header["survey_header_filename"] = mstr::clean_bc_str(this->atsh.survey_header_filename, 12);
    this->header["type_of_meas"] = mstr::clean_bc_str(this->atsh.type_of_meas, 4);

    this->header["DCOffsetCorrValue"] = static_cast<double>(this->atsh.DCOffsetCorrValue);
    this->header["DCOffsetCorrOn"] = static_cast<int64_t>(this->atsh.DCOffsetCorrOn);
    this->header["InputDivOn"] = static_cast<int64_t>(this->atsh.InputDivOn);
    this->header["bit_indicator"] = static_cast<int64_t>(this->atsh.bit_indicator);
    this->header["result_selftest"] = mstr::clean_bc_str(this->atsh.result_selftest, 2);
    this->header["numslices"] = static_cast<int64_t>(this->atsh.numslices);

    this->header["cal_freqs"] = static_cast<int64_t>(this->atsh.cal_freqs);
    this->header["cal_entry_length"] = static_cast<int64_t>(this->atsh.cal_entry_length);
    this->header["cal_version"] = static_cast<int64_t>(this->atsh.cal_version);
    this->header["cal_start_address"] = static_cast<int64_t>(this->atsh.cal_start_address);

    // bitfield; filterbank was set above
    this->header["LF_filters"] = get_lf_filter_strings();
    this->header["UTMZone"] = mstr::clean_bc_str(this->atsh.UTMZone, 12);
    this->header["system_cal_datetime"] = static_cast<int64_t>(this->atsh.system_cal_datetime);
    this->header["sensor_cal_filename"] = mstr::clean_bc_str(this->atsh.sensor_cal_filename, 12);
    this->header["sensor_cal_datetime"] = static_cast<int64_t>(this->atsh.sensor_cal_datetime);

    this->header["powerline1"] = static_cast<double>(this->atsh.powerline1);
    this->header["powerline2"] = static_cast<double>(this->atsh.powerline2);

    // bitfield; filterbank was set above
    this->header["HF_filters"] = get_hf_filter_strings();
    this->header["external_gain"] = static_cast<double>(this->atsh.external_gain);
    this->header["ADB_board_type"] = mstr::clean_bc_str(this->atsh.ADB_board_type, 4);

    this->header["Client"] = mstr::clean_bc_str(this->atsh.comments.Client, 16);
    this->header["Contractor"] = mstr::clean_bc_str(this->atsh.comments.Contractor, 16);
    this->header["Area"] = mstr::clean_bc_str(this->atsh.comments.Area, 16);
    this->header["SurveyID"] = mstr::clean_bc_str(this->atsh.comments.SurveyID, 16);
    this->header["Operator"] = mstr::clean_bc_str(this->atsh.comments.Operator, 16);
    this->header["SiteName"] = mstr::clean_bc_str(this->atsh.comments.SiteName, 112);
    this->header["XmlHeader"] = mstr::clean_bc_str(this->atsh.comments.XmlHeader, 64);

    // remove the useless comment weather:
    std::string wstr = mstr::clean_bc_str(this->atsh.comments.Comments, 288);
    if (wstr == "weather:")
      wstr.clear();
    this->header["Comments"] = wstr;
    this->header["SiteNameRR"] = mstr::clean_bc_str(this->atsh.comments.SiteNameRR, 112);
    this->header["SiteNameEMAP"] = mstr::clean_bc_str(this->atsh.comments.SiteNameEMAP, 112);
  }

  void set_ats_header() {

    // make a clean & empty struct
    memset(&this->atsh, 0, sizeof(this->atsh));

    this->atsh.header_length = static_cast<uint16_t>(this->header["header_length"]);
    this->atsh.header_version = static_cast<int16_t>(this->header["header_version"]);

    this->atsh.samples = static_cast<uint32_t>(this->header["samples"]);
    this->atsh.sample_rate = static_cast<float>(this->header["sample_rate"]);
    this->atsh.start = static_cast<uint32_t>(this->header["start"]);
    this->atsh.lsbval = static_cast<double>(this->header["lsbval"]);
    this->atsh.GMToffset = static_cast<int32_t>(this->header["GMToffset"]);
    this->atsh.orig_sample_rate = static_cast<float>(this->header["orig_sample_rate"]);

    this->atsh.serial_number = static_cast<uint16_t>(this->header["serial_number"]);
    this->atsh.serial_number_ADC_board = static_cast<uint8_t>(this->header["serial_number_ADC_board"]);
    this->atsh.channel_number = static_cast<uint8_t>(this->header["channel_number"]);
    this->atsh.chopper = static_cast<uint8_t>(this->header["chopper"]);
    strncpy(this->atsh.channel_type, this->header["channel_type"].get<std::string>().c_str(), sizeof(this->atsh.channel_type));

    // the header can onöy hold 6 chars here and MFS-06e is stored as MFS06e - so check it
    std::string tmp_sensor = this->header["sensor_type"].get<std::string>();
    if ((mstr::contains(tmp_sensor, "MFS")) || (mstr::contains(tmp_sensor, "SHFT")) || (mstr::contains(tmp_sensor, "FGS"))) {
      if (mstr::contains(tmp_sensor, "-"))
        tmp_sensor.erase(remove(tmp_sensor.begin(), tmp_sensor.end(), '-'), tmp_sensor.end());
    }
    if ((tmp_sensor.size() > 6) && (mstr::contains(tmp_sensor, "-")))
      tmp_sensor.erase(remove(tmp_sensor.begin(), tmp_sensor.end(), '-'), tmp_sensor.end());

    strncpy(this->atsh.sensor_type, tmp_sensor.c_str(), sizeof(this->atsh.sensor_type));
    this->atsh.sensor_serial_number = static_cast<int16_t>(this->header["sensor_serial_number"]);

    this->atsh.x1 = static_cast<float>(this->header["x1"]);
    this->atsh.y1 = static_cast<float>(this->header["y1"]);
    this->atsh.z1 = static_cast<float>(this->header["z1"]);
    this->atsh.x2 = static_cast<float>(this->header["x2"]);
    this->atsh.y2 = static_cast<float>(this->header["y2"]);
    this->atsh.z2 = static_cast<float>(this->header["z2"]);

    this->atsh.rho_probe_ohm = static_cast<float>(this->header["rho_probe_ohm"]);
    this->atsh.DC_offset_voltage_mV = static_cast<float>(this->header["DC_offset_voltage_mV"]);
    this->atsh.gain_stage1 = static_cast<float>(this->header["gain_stage1"]);
    this->atsh.gain_stage2 = static_cast<float>(this->header["gain_stage2"]);

    this->atsh.iLat_ms = static_cast<int32_t>(this->header["iLat_ms"]);
    this->atsh.iLong_ms = static_cast<int32_t>(this->header["iLong_ms"]);
    this->atsh.iElev_cm = static_cast<int32_t>(this->header["iElev_cm"]);

    this->atsh.Lat_Long_TYPE = this->header["Lat_Long_TYPE"].get<std::string>().at(0);
    this->atsh.coordinate_type = this->header["coordinate_type"].get<std::string>().at(0);

    this->atsh.ref_meridian = static_cast<int16_t>(this->header["ref_meridian"]);

    this->atsh.Northing = static_cast<double>(this->header["Northing"]);
    this->atsh.Easting = static_cast<double>(this->header["Easting"]);
    this->atsh.gps_clock_status = this->header["gps_clock_status"].get<std::string>().at(0);
    this->atsh.GPS_accuracy = this->header["GPS_accuracy"].get<std::string>().at(0);
    this->atsh.offset_UTC = static_cast<int16_t>(this->header["offset_UTC"]);

    std::string official_name = this->header["SystemType"].get<std::string>(); // like ADU-08e
    std::string header_name;
    for (const auto &name : ats_sys_names) {
      if (name.second == official_name) {
        header_name = name.first;
      }
    }
    for (auto &c : header_name)
      c = toupper(c);
    bool conv_from_06 = false;
    if (header_name == "ADU06") {
      conv_from_06 = true;
      header_name = "ADU07";
    }

    // make filters, where ADU-07 and ADU-06 use ADU-07e filters
    this->set_filter_bank(this->header["SystemType"]);
    strncpy(this->atsh.SystemType, header_name.c_str(), sizeof(this->atsh.SystemType));

    strncpy(this->atsh.survey_header_filename, this->header["survey_header_filename"].get<std::string>().c_str(), sizeof(this->atsh.survey_header_filename));
    strncpy(this->atsh.type_of_meas, this->header["type_of_meas"].get<std::string>().c_str(), sizeof(this->atsh.type_of_meas));
    this->atsh.DCOffsetCorrValue = static_cast<double>(this->header["DCOffsetCorrValue"]);
    this->atsh.DCOffsetCorrOn = static_cast<int8_t>(this->header["DCOffsetCorrOn"]);
    this->atsh.InputDivOn = static_cast<int8_t>(this->header["InputDivOn"]);
    this->atsh.bit_indicator = static_cast<int16_t>(this->header["bit_indicator"]);
    strncpy(this->atsh.result_selftest, this->header["result_selftest"].get<std::string>().c_str(), sizeof(this->atsh.result_selftest));
    this->atsh.numslices = static_cast<uint16_t>(this->header["numslices"]);
    this->atsh.cal_freqs = static_cast<int16_t>(this->header["cal_freqs"]);
    this->atsh.cal_entry_length = static_cast<int16_t>(this->header["cal_entry_length"]);
    this->atsh.cal_version = static_cast<int16_t>(this->header["cal_version"]);
    this->atsh.cal_start_address = static_cast<int16_t>(this->header["cal_start_address"]);

    this->set_lf_filter_int(this->header["LF_filters"]);

    strncpy(this->atsh.UTMZone, this->header["UTMZone"].get<std::string>().c_str(), sizeof(this->atsh.UTMZone));
    this->atsh.system_cal_datetime = static_cast<uint32_t>(this->header["system_cal_datetime"]);
    strncpy(this->atsh.sensor_cal_filename, this->header["sensor_cal_filename"].get<std::string>().c_str(), sizeof(this->atsh.sensor_cal_filename));

    this->atsh.sensor_cal_datetime = static_cast<uint32_t>(this->header["sensor_cal_datetime"]);
    this->atsh.powerline1 = static_cast<float>(this->header["powerline1"]);
    this->atsh.powerline2 = static_cast<float>(this->header["powerline2"]);

    this->set_hf_filter_int(this->header["HF_filters"]);

    this->atsh.external_gain = static_cast<float>(this->header["external_gain"]);
    strncpy(this->atsh.ADB_board_type, this->header["ADB_board_type"].get<std::string>().c_str(), sizeof(this->atsh.ADB_board_type));

    if (conv_from_06) {
      std::string s = this->header["Comments"];
      this->header["Comments"] = "converted from ADU-06";
      if (s.size()) {
        this->header["Comments"] += "; " + s;
      }
    }

    strncpy(this->atsh.comments.Client, this->header["Client"].get<std::string>().c_str(), sizeof(this->atsh.comments.Client));
    strncpy(this->atsh.comments.Contractor, this->header["Contractor"].get<std::string>().c_str(), sizeof(this->atsh.comments.Contractor));
    strncpy(this->atsh.comments.Area, this->header["Area"].get<std::string>().c_str(), sizeof(this->atsh.comments.Area));
    strncpy(this->atsh.comments.SurveyID, this->header["SurveyID"].get<std::string>().c_str(), sizeof(this->atsh.comments.SurveyID));
    strncpy(this->atsh.comments.Operator, this->header["Operator"].get<std::string>().c_str(), sizeof(this->atsh.comments.Operator));
    strncpy(this->atsh.comments.SiteName, this->header["SiteName"].get<std::string>().c_str(), sizeof(this->atsh.comments.SiteName));
    strncpy(this->atsh.comments.XmlHeader, this->header["XmlHeader"].get<std::string>().c_str(), sizeof(this->atsh.comments.XmlHeader));
    strncpy(this->atsh.comments.Comments, this->header["Comments"].get<std::string>().c_str(), sizeof(this->atsh.comments.Comments));
    strncpy(this->atsh.comments.SiteNameRR, this->header["SiteNameRR"].get<std::string>().c_str(), sizeof(this->atsh.comments.SiteNameRR));
    strncpy(this->atsh.comments.SiteNameEMAP, this->header["SiteNameEMAP"].get<std::string>().c_str(), sizeof(this->atsh.comments.SiteNameEMAP));
  }

  /*!
   * \brief create_default_header
   * \param channel_type Ex, Ey, Hx, Hy, Hz
   */
  void create_default_header(const std::string channel_type) {
    this->header["header_length"] = static_cast<int64_t>(1024);
    this->header["header_version"] = static_cast<int64_t>(80);

    this->header["samples"] = static_cast<int64_t>(0);
    this->header["sample_rate"] = static_cast<double>(0);
    this->header["start"] = static_cast<int64_t>(0);
    this->header["lsbval"] = static_cast<double>(0);
    this->header["GMToffset"] = static_cast<int64_t>(0);
    this->header["orig_sample_rate"] = static_cast<double>(0);

    this->header["x1"] = static_cast<double>(0);
    this->header["y1"] = static_cast<double>(0);
    this->header["z1"] = static_cast<double>(0);
    this->header["x2"] = static_cast<double>(0);
    this->header["y2"] = static_cast<double>(0);
    this->header["z2"] = static_cast<double>(0);

    this->header["serial_number"] = static_cast<int64_t>(999);
    if (channel_type == "Ex") {
      this->header["serial_number_ADC_board"] = static_cast<int64_t>(1);
      this->header["channel_number"] = static_cast<int64_t>(0);
      this->header["sensor_type"] = "EFP06";
      this->header["sensor_serial_number"] = 1;
      this->header["x1"] = static_cast<double>(-500);
      this->header["x2"] = static_cast<double>(500);
      this->header["InputDivOn"] = static_cast<int64_t>(0);

    } else if (channel_type == "Ey") {
      this->header["serial_number_ADC_board"] = static_cast<int64_t>(2);
      this->header["channel_number"] = static_cast<int64_t>(1);
      this->header["sensor_type"] = "EFP06";
      this->header["sensor_serial_number"] = 2;
      this->header["y1"] = static_cast<double>(-500);
      this->header["y2"] = static_cast<double>(500);
      this->header["InputDivOn"] = static_cast<int64_t>(0);

    } else if (channel_type == "Hx") {
      this->header["serial_number_ADC_board"] = static_cast<int64_t>(3);
      this->header["channel_number"] = static_cast<int64_t>(2);
      this->header["sensor_type"] = "MFS06E";
      this->header["sensor_serial_number"] = 3;
      this->header["InputDivOn"] = static_cast<int64_t>(1);

    } else if (channel_type == "Hy") {
      this->header["serial_number_ADC_board"] = static_cast<int64_t>(4);
      this->header["channel_number"] = static_cast<int64_t>(3);
      this->header["sensor_type"] = "MFS06E";
      this->header["sensor_serial_number"] = 4;
      this->header["InputDivOn"] = static_cast<int64_t>(1);

    } else if (channel_type == "Hz") {
      this->header["serial_number_ADC_board"] = static_cast<int64_t>(5);
      this->header["channel_number"] = static_cast<int64_t>(4);
      this->header["sensor_type"] = "MFS06E";
      this->header["sensor_serial_number"] = 5;
      this->header["InputDivOn"] = static_cast<int64_t>(1);
    }
    this->header["chopper"] = static_cast<int64_t>(0);
    this->header["channel_type"] = channel_type;

    // pos and diplength not supported since 2004 - inconsistency removed

    this->header["rho_probe_ohm"] = static_cast<double>(1);
    this->header["DC_offset_voltage_mV"] = static_cast<double>(0);
    this->header["gain_stage1"] = static_cast<double>(1);
    this->header["gain_stage2"] = static_cast<double>(1);

    // Data from status information ?
    this->header["iLat_ms"] = static_cast<int64_t>(0);
    this->header["iLong_ms"] = static_cast<int64_t>(0);
    this->header["iElev_cm"] = static_cast<int64_t>(0);
    this->header["Lat_Long_TYPE"] = "G";
    this->header["coordinate_type"] = "U";
    this->header["ref_meridian"] = static_cast<int64_t>(0);

    this->header["Northing"] = static_cast<double>(0);
    this->header["Easting"] = static_cast<double>(0);
    this->header["gps_clock_status"] = "C";
    this->header["GPS_accuracy"] = "1";
    this->header["offset_UTC"] = 0;

    this->header["SystemType"] = "ADU-08e";
    // beside from binary header
    this->header["GMSno"] = 8;
    this->header["TypeNo"] = 1;
    this->header["ats_data_file"] = "";
    //
    set_filter_bank("ADU-08e");

    // Data from XML-Job specification
    this->header["survey_header_filename"] = "";
    this->header["type_of_meas"] = "MT";

    this->header["DCOffsetCorrValue"] = static_cast<double>(0);
    this->header["DCOffsetCorrOn"] = static_cast<int64_t>(0);
    this->header["bit_indicator"] = static_cast<int64_t>(0);
    this->header["result_selftest"] = "OK";
    this->header["numslices"] = static_cast<int64_t>(0);

    this->header["cal_freqs"] = static_cast<int64_t>(0);
    this->header["cal_entry_length"] = static_cast<int64_t>(0);
    this->header["cal_version"] = static_cast<int64_t>(0);
    this->header["cal_start_address"] = static_cast<int64_t>(0);

    // bitfield; filterbank was set above
    this->header["LF_filters"] = "";
    this->header["UTMZone"] = "";
    this->header["system_cal_datetime"] = static_cast<int64_t>(0);
    this->header["sensor_cal_filename"] = "SENSOR.CAL";
    this->header["sensor_cal_datetime"] = static_cast<int64_t>(0);

    this->header["powerline1"] = static_cast<double>(0.0);
    this->header["powerline2"] = static_cast<double>(0.0);

    // bitfield; filterbank was set above
    this->header["HF_filters"] = "";
    this->header["external_gain"] = static_cast<double>(0);
    this->header["ADB_board_type"] = "BB";

    this->header["Client"] = "";
    this->header["Contractor"] = "";
    this->header["Area"] = "";
    this->header["SurveyID"] = "";
    this->header["Operator"] = "";
    this->header["SiteName"] = "";
    this->header["XmlHeader"] = "";

    this->header["Comments"] = "from ASCII";
    this->header["SiteNameRR"] = "";
    this->header["SiteNameEMAP"] = "";
  }
};

#endif // ATSHEADER_DEF
