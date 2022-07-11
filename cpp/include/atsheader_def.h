#ifndef ATSHEADER_DEF
#define ATSHEADER_DEF


/**
  * @file atsheader_def.h
  * Contains the binary header of ats files AND ats_json
  *
*/

#include <cstdint>
#include <cstdio>
#include <string>
#include <cstring>
#include <iterator>
#include <unordered_map>
#include <filesystem>
#include "strings_etc.h"
#include "atss_time.h"
#include "json.h"
#include "mt_base.h"

namespace fs = std::filesystem;

//
//
//
//         array of chars are not NULL terminated !
//         achChanType   [2] = "Ex" is not NULL terminated
//
//

#define C_ATS_CEA_NUM_HEADERS   1023     /*!< maximum number of 1023 headers of the ATS sliced file.*/


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

                struct ATSSliceHeader_1080
{
    std::uint32_t uiSamples;                    //!< 000h  number of samples for this slice
    std::uint32_t uiStartDateTime;              //!< 004h  startdate/time as UNIX timestamp (UTC)
    double        dbDCOffsetCorrValue;          //!< 008h  DC offset correction value in mV
    float         rPreGain;                     //!< 010h  originally used pre-gain (GainStage1) - LSB is the same for all slices
    float         rPostGain;                    //!< 014h  originally used post-gain (GainStage2) - LSB is the same for all slices
    std::int8_t   DCOffsetCorrOn;               //!< 018h  DC offset correction was on/off for this slice
    std::int8_t   reserved[7];                  //!< 020h  reserved bytes to get to word / dword boundary
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
    char          Client           [16];        //!< 000h  UTF-8 storage, used in EDI
    char          Contractor       [16];        //!< 010h  UTF-8 storage, used in EDI
    char          Area             [16];        //!< 020h  UTF-8 storage, used in EDI
    char          SurveyID         [16];        //!< 030h  UTF-8 storage, used in EDI
    char          Operator         [16];        //!< 040h  UTF-8 storage, used in EDI
    char          SiteName        [112];        //!< 050h  UTF-8 storage, no BOM at the beginning!; WORST case = 28 Chinese/Japanese chars (multibyte chars)
    char          XmlHeader        [64];        //!< 0C0h  UTF-8 storage  points to the corresponding XML file, containing addtional information for this header; no PATH=XML file is in the same directory as the ats file
    // deleted September 2018 char          achComments   [512];          //!< 100h  UTF-8 storage  comment block starts (comment field can start with "weather:" but has no meaning
    // new September 2018
    char          Comments        [288];        //!< 100h  UTF-8 storage  comment block starts (comment field can start with "weather:" but has no meaning
    char          SiteNameRR      [112];        //!< 220h  UTF-8 storage, no BOM at the beginning!; WORST case = 28 Chinese/Japanese chars (multibyte chars)
    char          SiteNameEMAP    [112];        //!< 290h  UTF-8 storage, no BOM at the beginning!; WORST case = 28 Chinese/Japanese chars (multibyte chars); INDICATES a default EMAP center station, where we expect the H (magnetic)


};


/*!
   \brief The ATSHeader_80 struct<br>
   reference is lat and long; additional entries like northing and easting should be empty // not used<br>
   these entries can be overwritten by the user in case of high density grids <br>
   The site_name (site name, achSiteName, is in the comment field; the site_no, site_num, site_number, site number is nver stored; this is part of external numeration
 */
struct ATSHeader_80 {
    std::uint16_t header_length;                //!< 000h  length of header in bytes (default 1024 and  33760 for CEA)
    std::int16_t  header_version;               //!< 002h  80 for ats, 81 for 64bit possible / metronix, 1080 for CEA / sliced header

    // This information can be found in the ChannelTS datastructure
    std::uint32_t samples;                      //!< 004h  amount of samples (each is a 32bit / 64bit int INTEL byte order little endian) in the file total of all slices; if uiSamples == UINT32_MAX, uiSamples64bit at address 0F0h will be used instead; uiSamples64bit replaces uiSamples in that case; do not add!

    float         sample_rate;                  //!< 008h  sampling frequency in Hz
    std::uint32_t start;                        //!< 00Ch  unix TIMESTAMP (some computers will revert that to negative numbers if this number is grater than 32bit signed); 2106-02-07T06:28:14 is the END of this format
    double        lsbval;                       //!< 010h  least significant bit in mV ()
    std::int32_t  GMToffset;                    //!< 018h  not used, default 0; can be used as "UTC to GMT"
    float         orig_sample_rate;             //!< 01Ch  sampling frequency in Hz as ORIGINALLY recorded; this value should NOT change (for example after filtering)

    //The required data could probably found in the HardwareConfig
    std::uint16_t serial_number;                //!< 020h  Serial number of the system (logger)
    std::uint16_t serial_number_ADC_board;      //!< 022h  Serial number of the ADC board (ADB)
    std::uint8_t  channel_number;               //!< 024h  Channel number
    std::uint8_t  chopper;                      //!< 025h  Chopper On (1) / Off (0)

    // Data from XML Job-specification
    char          channel_type   [2];           //!< 026h  (Ex, Ey, Ez, Hx, Hy, Hz, Jx, Jy, Jz, Px, Py, Pz, Rx, Ry, Rz and so on)
    char          sensor_type [6];              //!< 028h  (MFS06 MFS06e MFS07 MFS07e MFS10e SHFT02 SHF02e FGS02 FGS03 FGS03e etc. e.g. the "-" in MFS-06e is skipped)
    std::int16_t  sensor_serial_number;         //!< 02Eh  Serial number of connected sensor

    float         x1;                           //!< 030h  e.g. South negative [m]
    float         y1;                           //!< 034h  e.g. West negative [m]
    float         z1;                           //!< 038h  e.g. top, sky [m]
    float         x2;                           //!< 03Ch  e.g. North positive [m]
    float         y2;                           //!< 040h  e.g. East positive [m]
    float         z2;                           //!< 044h  e.g. bottom, earth [m]

    // not used any more use pos values!!; GUI interfaces using Length and direction MUST calculate pos x,y,z and set above.
    float         dipole_length;                //!< 048h  e.g. to be calculated; should not be used - my be over written in FUTURE
    // not used any more use pos values!!
    float         angle;                        //!< 04Ch  e.g. to be calculated; should not be used - my be over written in FUTURE

    // Data from Selftest
    float         rho_probe_ohm;                //!< 050h  [ohm]
    float         DC_offset_voltage_mV;         //!< 054h  [mV]
    float         gain_stage1;                  //!< 058h  e.g. Gain Stage 1
    float         gain_stage2;                  //!< 05Ch  e.g. Gain Stage 2

    // Data from status information ?
    std::int32_t  iLat_ms;                      //!< 060h  must be used, UNIT = milli seconds
    std::int32_t  iLong_ms;                     //!< 064h  must be used, UNIT = milli seconds
    std::int32_t  iElev_cm;                     //!< 068h  must be used, UNIT = cm
    char          Lat_Long_TYPE;                //!< 06Ch  'G' default, 'U' user, GPS should be used
    char          coordinate_type;              //!< 06Dh  'U' = UTM, default empty
    std::int16_t  ref_meridian;                 //!< 06Eh  default empty, should not be used (external)



    //!@todo can we store 64bit time and 64bit samples here ??
    double        Northing;                     //!< 070h  also xcoord should not be used, default 0 (external)
    double        Easting;                      //!< 078h  also ycoord should not be used  default 0 (external)
    char          gps_clock_status;             //!< 080h  '-' unknown, 'N' no fix, 'C' full fix
    char          GPS_accuracy;                 //!< 081h  '0' - not used, 1 in case GF4-Fix & Syrlinks was active (system was synced before Syrlinks took over)
    std::int16_t  offset_UTC;                   //!< 082h  UTC + iUTCOffset = GPS time for example; can be used for other offsets, not used, default 0
    char          SystemType[12];               //!< 084h  MMS03e GMS05 ADU06 ADU07 ADU08 ADU09 ADU10 ADU11 SPAMMKV SPAMMKIII

    // Data from XML-Job specification
    char          survey_header_filename [12];  //!< 090h  UTF-8 storage  free for usage
    char          type_of_meas           [4];   //!< 09Ch  free for usage MT CSMT


    double        DCOffsetCorrValue;            //!< 0A0h  DAC offset double
    std::int8_t   DCOffsetCorrOn;               //!< 0A8h  DC offset was switched on (1) or off(0)
    std::int8_t   InputDivOn;                   //!< 0A9h  inputput divider on(1) off(0); e.g when coil was connected
    std::int16_t  bit_indicator;                //!< 0AAh  0 = 32bit int INTEL byte order, 1 = 64bit INTEL byte order, little endian; since atsheader version 81
    char          result_selftest [2];          //!< 0ACh  'NO' or 'OK'
    std::uint16_t numslices;                    //!< 0AEh  number of slices used (1....1024, 1 is the first, that is list.size() )

    //std::int16_t  calentries    // max 128 entries - not existing

    // Were the following fields ever used ?
    std::int16_t  cal_freqs;                    //!< 0B0h  not used  (external)
    std::int16_t  cal_entry_length;             //!< 0B2h  not used  (external)
    std::int16_t  cal_version;                  //!< 0B4h  not used  (external)
    std::int16_t  cal_start_address;            //!< 0B6h  not used, never used  (external)


    char          LF_filters [8];               //!< 0B8h  is a bitfield

    char          UTMZone     [12];             //!< 0C0h  not used  (external)  (formerly abyADU06CalFilename) 32U or 01G, 32UMD7403 : alway NNC od NNCCNNNN
    std::uint32_t system_cal_datetime;          //!< 0CCh  not used  (external)
    char          sensor_cal_filename [12];     //!< 0D0h  not used  ("SENSOR.CAL") (external)
    std::uint32_t sensor_cal_datetime;          //!< 0DCh  not used  (external)

    float         powerline1;                   //!< 0E0h  e.g. empty  (external)
    float         powerline2;                   //!< 0E4h  e.g. empty  (external)
    char          HF_filters[8];                //!< 0E8h  is a bitfield


    // IF uiSamples == UINT32_MAX you find
    std::uint64_t samples_64bit;                //!< 0F0h  amount of samples (each is a 32bit /64bit int) in the file total of all slices; ONLY used in case uiSamples == UINT32_MAX; else 0; REPLACSES the uiSamples; do not add

    //double        OriginalLSBMV;              //!< 0F0h  NOT USED ANYMORE! orig lsb from selftest without gains; used for ADC values
    float         external_gain;                //!< 0F8h  for external satellite box
    char          ADB_board_type[4];            //!< 0FCh  LF HF or MF or BB


    ATSComments_80 comments;                    //!< 100h

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
    {"adu12", "ADU-12e"}
};

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
    {"ADU-12e", 6}
};

/*!
 * \brief The ats_sys_family
 */
std::unordered_map<std::string, int> ats_sys_family = {
    {"ADU-06", 6},
    {"ADU-07e", 7},
    {"ADU-08e", 8},
    {"ADU-09u", 9},
    {"ADU-10e", 10},
    {"ADU-11e", 11}
};




/*!
 * \brief The ats_header_json struct
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

    std::filesystem::path xml_path() const {
        std::filesystem::path xp(this->filename.parent_path());
        xp += "/";
        xp += header["XmlHeader"].get<std::string>();
        return xp;
    }


    std::string start_date() const {
        std::string udate, utime;
        double f = 0.0;
        long int utc = static_cast<int64_t>(this->atsh.start);
        auto result = mtime::time_t_iso8601_utc(utc, udate, utime, f);
        return udate;        
    }

    std::string stop_date() const {
        long double sf_lhs = static_cast<long double>(this->atsh.samples) / static_cast<long double>(this->atsh.sample_rate);
        sf_lhs +=  static_cast<long double>(this->atsh.start);
        double f = 0.0, intpart;
        f = modf(sf_lhs, &intpart);
        int64_t utc = static_cast<int64_t>(intpart);
        std::string udate, utime;
        auto result = mtime::time_t_iso8601_utc(utc, udate, utime, f);
        return udate;
    }

    std::string start_time() const {
        std::string udate, utime;
        double f = 0.0;
        long int utc = static_cast<int64_t>(this->atsh.start);
        auto result = mtime::time_t_iso8601_utc(utc, udate, utime, f);
        return utime;
    }

    std::string stop_time() const {
        long double sf_lhs = static_cast<long double>(this->atsh.samples) / static_cast<long double>(this->atsh.sample_rate);
        sf_lhs +=  static_cast<long double>(this->atsh.start);
        double f = 0.0, intpart;
        f = modf(sf_lhs, &intpart);
        int64_t utc = static_cast<int64_t>(intpart);
        std::string udate, utime;
        auto result = mtime::time_t_iso8601_utc(utc, udate, utime, f);
        return utime;
    }

    double get_lat() const {
        return (this->atsh.iLat_ms / 1000.) / 3600.;
    }

    double get_lon() const {
        return (this->atsh.iLong_ms / 1000.) / 3600.;
    }

    double get_elev() const {
        return (this->atsh.iElev_cm / 100.);
    }

    double pos2length() const {
        double tx, ty, tz;
        tx = double(this->atsh.x2 - this->atsh.x1);
        ty = double(this->atsh.y2 - this->atsh.y1);
        tz = double(this->atsh.z2 - this->atsh.z1);

        double diplength = sqrt(tx * tx  + ty * ty + tz * tz);
        if (diplength < 0.001) diplength = 0;                   // avoid rounding errors
        return diplength;
    }

    double pos2angle() const {

        if (!this->header.contains("channel_type") ) {
            std::string err_str = __func__;
            err_str += "::header not existing; read file and use get_ats_header() ";
            throw err_str;
        }
        double tx, ty;
        tx = double(this->atsh.x2 - this->atsh.x1);
        ty = double(this->atsh.y2 - this->atsh.y1);

        double diplength = this->pos2length();

        // avoid calculation noise
        if (fabs(tx) < 0.001) tx = 0.0;
        if (fabs(ty) < 0.001) ty = 0.0;

        // many user do not set coordiantes for the coils
        // Hx
        if ((diplength == 0) &&  (this->header["channel_type"].get<std::string>() == "Hx")) {
            return 0.;                                                  // NORTH
        }
        // Hy
        if ((diplength == 0) && (this->header["channel_type"].get<std::string>() == "Hy")) {
            return 90.;                                                 // EAST
        }
        // Hz
        if ((diplength == 0) && (this->header["channel_type"].get<std::string>() == "Hz")) {
            return  0.;
        }

        if ((tx == 0) && (ty == 0)) return 0;

        // hmm hmm possible but you normally set the system N S E W
        double ang = atan2(ty,tx) * 180.0 / M_PI;

        // let angle from position snap
        if ( (ang < 90.01) && (ang > 89.99 )) return 90.;
        if ( (ang < 0.01) && (ang > 359.99 )) return 0.;
        if ( (ang < 180.01) && (ang > 179.99 )) return 180.;
        if ( (ang < 270.01) && (ang > 269.99 )) return 270.;

        return ang;

    }

    double pos2dip() const {

        double tz = double(this->atsh.z2 - this->atsh.z1);


        double diplength = this->pos2length();
        // no coordiantes given for Hz
        if ((diplength == 0) && (this->header["channel_type"].get<std::string>() == "Hz")) {
            return  90.0;
        }

        if (diplength == 0) return 0.0; // horizontal - no length
        if (tz < 0.001) return 0.0;       // no z component

        //! @todo that is maybe wrong
        double ang = 90.0 - acos(tz/diplength) * 180.0 / M_PI;
        if ( (ang < 0.01) && (ang > 359.99 )) return 0.;
        if ( (ang < 90.01) && (ang > 89.99 )) return 90.;

        return ang;


    }

    std::string measdir() const {

        return mtime::measdir_time(static_cast<int64_t>(this->atsh.start));
    }


    ChopperStatus get_chopper() const {
        if (!this->header.size()) return ChopperStatus::off;
        if (this->header["chopper"] == 1) return ChopperStatus::on;
        return ChopperStatus::off;
    }

    int64_t get_run() const {
        if (!this->filename.string().size()) return 0;
        std::string base = this->filename.stem().string();

        int64_t irun = 0;
        auto tokens = mstr::split(base, '_');
        for (auto &token : tokens) {
            if (token.starts_with('R') || token.starts_with('r')) {
                try{
                    auto rstr = token.substr(1);
                    irun = std::stoi(rstr);
                }
                catch  (...) {
                    irun = 0;
                }
            }
        }

        return irun;


    }

    std::unordered_map<std::string, uint8_t> LF_Filters;
    std::unordered_map<std::string, uint8_t> HF_Filters;

    std::vector<uint8_t> LFFilter_to_ints() const {
        std::vector<uint8_t> filters(8,0);
        size_t i = 0;
        for (i = 0; i < sizeof(this->atsh.LF_filters); ++i) {
            filters[i] = static_cast<int8_t>(this->atsh.LF_filters[i]);
        }
        return filters;
    }

    std::vector<uint8_t> HFFilter_to_ints() const {
        std::vector<uint8_t> filters(8,0);
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
        if (ADUin == "ADU-06") ADU = "ADU-07e";
        if (ADUin == "ADU-07") ADU = "ADU-07e";
        this->LF_Filters.clear();
        this->HF_Filters.clear();

        if ((ADU == "ADU-07e") || (ADU == "ADU-08e"))   this->LF_Filters["LF-RF-1"] =     1;  //! 0x01 ADU07/8_LF-RF-1 filter on LF board with capacitor 22pF
        if ((ADU == "ADU-07e") || (ADU == "ADU-08e"))   this->LF_Filters["LF-RF-2"] =     2;  //! 0x02 ADU07/8_LF-RF-2 filter on LF board with capacitor 122pF
        if ((ADU == "ADU-07e"))                         this->LF_Filters["LF-RF-3"] =     4;  //! 0x04 ADU07_LF-RF-3 filter on LF board with capacitor 242pF
        if ((ADU == "ADU-07e"))                         this->LF_Filters["LF-RF-4"] =     8;  //! 0x08 ADU07_LF-RF-4 filter on LF board with capacitor 342pF
        if ((ADU == "ADU-07e") || (ADU == "ADU-08e"))   this->LF_Filters["LF_LP_4HZ"] =   16; //! 0x10 ADU07/8_LF_LP_4HZ filter on LF board with 4 Hz Lowpass characteristic

        if ((ADU == "ADU-07e"))                         this->LF_Filters["MF-RF-1"] =     32; //! 0x40 ADU07_MF_RF_1 filter on MF board with capacitor 470nF
        if ((ADU == "ADU-07e"))                         this->LF_Filters["MF-RF-2"] =     64; //! 0x20 ADU07_MF_RF_2 filter on MF board with capacitor 4.7nF

        // HF Path
        // 1 Hz has been dropped for 08
        if ((ADU == "ADU-07e"))                         this->HF_Filters["HF-HP-1Hz"] =   1;  //! 0x01 ADU07_HF-HP-1Hz 1Hz filter enable for HF board
        // 500Hz is the HP for 08
        if ((ADU == "ADU-08e"))                         this->HF_Filters["HF-HP-500Hz"] = 2;  //! 0x02 ADU08_HF-HP-500Hz 500Hz filter enable for HF board

        return this->LF_Filters.size() + this->HF_Filters.size();

    }

    std::string get_lf_filter_strings() const {
        std::string sfilter;
        std::vector<uint8_t> filters(this->LFFilter_to_ints());

        // the header uses only the FIRST int
        std::map<uint8_t, std::string> rfilters;
        for (const auto &it : this->LF_Filters) {
            rfilters[it.second] = it.first;
        }

        for (auto it = rfilters.crbegin(); it != rfilters.crend(); ++it) {
            if (filters[0] >= it->first) {
                if (sfilter.size()) sfilter += ",";
                sfilter += it->second;
                filters[0] -= it->first;
            }
        }

        return sfilter;
    }

    std::string get_hf_filter_strings() const {
        std::string sfilter;
        std::vector<uint8_t> filters(this->HFFilter_to_ints());

        // the header uses only the FIRST int
        std::map<uint8_t, std::string> rfilters;
        for (const auto &it : this->HF_Filters) {
            rfilters[it.second] = it.first;
        }

        for (auto it = rfilters.crbegin(); it != rfilters.crend(); ++it) {
            if (filters[0] >= it->first) {
                if (sfilter.size()) sfilter += ",";
                sfilter += it->second;
                filters[0] -= it->first;
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
            if (mstr::contains(cs_string, it.first)) filters[0] += it.second;
        }

        for (size_t i = 0; i < sizeof(this->atsh.HF_filters); ++i) {
            this->atsh.HF_filters[i] = static_cast<char>(filters[i]);
        }

    }

    void set_lf_filter_int(const std::string &cs_string) {
        std::vector<uint8_t> filters(8, 0);

        for (const auto &it : this->LF_Filters) {
            if (mstr::contains(cs_string, it.first)) filters[0] += it.second;
        }

        for (size_t i = 0; i < sizeof(this->atsh.LF_filters); ++i) {
            this->atsh.LF_filters[i] = static_cast<char>(filters[i]);
        }

    }

    /*!
     * \brief get_ats_header gets the binary ats header into the JSON
     */
    void get_ats_header() {
        header["header_length"] =                 static_cast<int64_t>(this->atsh.header_length);
        header["header_version"] =                static_cast<int64_t>(this->atsh.header_version);

        header["samples"] =                       static_cast<int64_t>(this->atsh.samples);
        header["sample_rate"] =                   static_cast<double>(this->atsh.sample_rate);
        header["start"] =                         static_cast<int64_t>(this->atsh.start);
        header["lsbval"] =                        static_cast<double>(this->atsh.lsbval);
        header["GMToffset"] =                     static_cast<int64_t>(this->atsh.GMToffset);
        header["orig_sample_rate"] =              static_cast<double>(this->atsh.orig_sample_rate);

        header["serial_number"] =                 static_cast<int64_t>(this->atsh.serial_number);
        header["serial_number_ADC_board"] =       static_cast<int64_t>(this->atsh.serial_number_ADC_board);
        header["channel_number"] =                static_cast<int64_t>(this->atsh.channel_number);
        header["chopper"] =                       static_cast<int64_t>(this->atsh.chopper);

        std::string tch =                         mstr::clean_bc_str(this->atsh.channel_type, 2);
        std::transform(tch.begin(), tch.begin()+1, tch.begin(), ::toupper);
        std::transform(tch.begin()+1, tch.end(), tch.begin()+1, ::tolower);
        header["channel_type"] =                  tch;
        header["sensor_type"] =                   mstr::clean_bc_str(this->atsh.sensor_type, 6);
        header["sensor_serial_number"] =          static_cast<int64_t>(this->atsh.sensor_serial_number);

        header["x1"] =                            static_cast<double>(this->atsh.x1);
        header["y1"] =                            static_cast<double>(this->atsh.y1);
        header["z1"] =                            static_cast<double>(this->atsh.z1);
        header["x2"] =                            static_cast<double>(this->atsh.x2);
        header["y2"] =                            static_cast<double>(this->atsh.y2);
        header["z2"] =                            static_cast<double>(this->atsh.z2);

        // pos and diplength not supported since 2004 - inconsistency removed

        header["rho_probe_ohm"] =                 static_cast<double>(this->atsh.rho_probe_ohm);
        header["DC_offset_voltage_mV"] =          static_cast<double>(this->atsh.DC_offset_voltage_mV);
        header["gain_stage1"] =                   static_cast<double>(this->atsh.gain_stage1);
        header["gain_stage2"] =                   static_cast<double>(this->atsh.gain_stage2);

        // Data from status information ?
        header["iLat_ms"] =                       static_cast<int64_t>(this->atsh.iLat_ms);
        header["iLong_ms"] =                      static_cast<int64_t>(this->atsh.iLong_ms);
        header["iElev_cm"] =                      static_cast<int64_t>(this->atsh.iElev_cm);
        header["Lat_Long_TYPE"] =                 std::string(1, this->atsh.Lat_Long_TYPE);
        header["coordinate_type"] =               std::string(1, this->atsh.coordinate_type);
        header["ref_meridian"] =                  static_cast<int64_t>(this->atsh.ref_meridian);

        header["Northing"] =                      static_cast<double>(this->atsh.Northing);
        header["Easting"] =                       static_cast<double>(this->atsh.Easting);
        header["gps_clock_status"] =              std::string(1, this->atsh.gps_clock_status);
        header["GPS_accuracy"] =                  std::string(1, this->atsh.GPS_accuracy);
        header["offset_UTC"] =                    static_cast<int64_t>(this->atsh.offset_UTC);

        int TypeNo = 0;
        int GMSno = 0;
        std::string Name("unknown");

        auto str1 = mstr::clean_bc_str(this->atsh.SystemType, 12);
        std::transform(str1.begin(), str1.end(), str1.begin(), ::tolower);

        try {
            Name = ats_sys_names.at(str1);
        } catch (...) {
            Name = "unkown";
        }
        try {
            TypeNo = ats_sys_types.at(Name);
        } catch (...) {
            TypeNo = 0;
        }
        try {
            TypeNo = ats_sys_family.at(Name);
        } catch (...) {
            GMSno = 0;
        }

        header["SystemType"] =                    Name;
        // beside from binary header
        header["GMSno"] =                         GMSno;
        header["TypeNo"] =                        TypeNo;
        header["ats_data_file"] =                 this->filename.filename();
        //
        set_filter_bank(Name);

        // Data from XML-Job specification
        header["survey_header_filename"] =        mstr::clean_bc_str(this->atsh.survey_header_filename, 12);
        header["type_of_meas"] =                  mstr::clean_bc_str(this->atsh.type_of_meas, 4);

        header["DCOffsetCorrValue"] =             static_cast<double>(this->atsh.DCOffsetCorrValue);
        header["DCOffsetCorrOn"] =                static_cast<int64_t>(this->atsh.DCOffsetCorrOn);
        header["InputDivOn"] =                    static_cast<int64_t>(this->atsh.InputDivOn);
        header["bit_indicator"] =                 static_cast<int64_t>(this->atsh.bit_indicator);
        header["result_selftest"] =               mstr::clean_bc_str(this->atsh.result_selftest, 2);
        header["numslices"] =                     static_cast<int64_t>(this->atsh.numslices);

        header["cal_freqs"] =                     static_cast<int64_t>(this->atsh.cal_freqs);
        header["cal_entry_length"] =              static_cast<int64_t>(this->atsh.cal_entry_length);
        header["cal_version"] =                   static_cast<int64_t>(this->atsh.cal_version);
        header["cal_start_address"] =             static_cast<int64_t>(this->atsh.cal_start_address);

        // bitfield; filterbank was set above      
        header["LF_filters"] =                    get_lf_filter_strings();
        header["UTMZone"] =                       mstr::clean_bc_str(this->atsh.UTMZone, 12);
        header["system_cal_datetime"] =           static_cast<int64_t>(this->atsh.system_cal_datetime);
        header["sensor_cal_filename"] =           mstr::clean_bc_str(this->atsh.sensor_cal_filename, 12);
        header["sensor_cal_datetime"] =           static_cast<int64_t>(this->atsh.sensor_cal_datetime);

        header["powerline1"] =                    static_cast<double>(this->atsh.powerline1);
        header["powerline2"] =                    static_cast<double>(this->atsh.powerline2);

        // bitfield; filterbank was set above 
        header["HF_filters"] =                    get_hf_filter_strings();
        header["external_gain"] =                 static_cast<double>(this->atsh.external_gain);
        header["ADB_board_type"] =                mstr::clean_bc_str(this->atsh.ADB_board_type, 4);

        header["Client"] =                        mstr::clean_bc_str(this->atsh.comments.Client, 16);
        header["Contractor"] =                    mstr::clean_bc_str(this->atsh.comments.Contractor, 16);
        header["Area"] =                          mstr::clean_bc_str(this->atsh.comments.Area, 16);
        header["SurveyID"] =                      mstr::clean_bc_str(this->atsh.comments.SurveyID, 16);
        header["Operator"] =                      mstr::clean_bc_str(this->atsh.comments.Operator, 16);
        header["SiteName"] =                      mstr::clean_bc_str(this->atsh.comments.SiteName, 112);
        header["XmlHeader"] =                     mstr::clean_bc_str(this->atsh.comments.XmlHeader, 64);

        // remove the useless comment weather:
        std::string wstr =                        mstr::clean_bc_str(this->atsh.comments.Comments, 288);
        if (wstr == "weather:") wstr.clear();
        header["Comments"] =                      wstr;
        header["SiteNameRR"] =                    mstr::clean_bc_str(this->atsh.comments.SiteNameRR, 112);
        header["SiteNameEMAP"] =                  mstr::clean_bc_str(this->atsh.comments.SiteNameEMAP, 112);

    }


    void set_ats_header() {

        // make a clean & empty struct
        memset(&this->atsh, 0, sizeof(this->atsh));



        this->atsh.header_length        = static_cast<uint16_t>(header["header_length"]);
        this->atsh.header_version       = static_cast<int16_t>(header["header_version"]);

        this->atsh.samples              = static_cast<uint32_t>(header["samples"]);
        this->atsh.sample_rate          = static_cast<float>(header["sample_rate"]);
        this->atsh.start                = static_cast<uint32_t>(header["start"]);
        this->atsh.lsbval               = static_cast<double>(header["lsbval"]);
        this->atsh.GMToffset            = static_cast<int32_t>(header["GMToffset"]);
        this->atsh.orig_sample_rate     = static_cast<float>(header["orig_sample_rate"]);

        this->atsh.serial_number        = static_cast<uint16_t>(header["serial_number"]);
        this->atsh.serial_number_ADC_board = static_cast<uint8_t>(header["serial_number_ADC_board"]);
        this->atsh.channel_number       = static_cast<uint8_t>(header["channel_number"]);
        this->atsh.chopper              = static_cast<uint8_t>(header["chopper"]);
        strncpy(this->atsh.channel_type, header["channel_type"].get<std::string>().c_str(), sizeof(this->atsh.channel_type));
        strncpy(this->atsh.sensor_type, header["sensor_type"].get<std::string>().c_str(), sizeof(this->atsh.sensor_type));
        this->atsh.sensor_serial_number = static_cast<int16_t>(header["sensor_serial_number"]);

        this->atsh.x1                   = static_cast<float>(header["x1"]);
        this->atsh.y1                   = static_cast<float>(header["y1"]);
        this->atsh.z1                   = static_cast<float>(header["z1"]);
        this->atsh.x2                   = static_cast<float>(header["x2"]);
        this->atsh.y2                   = static_cast<float>(header["y2"]);
        this->atsh.z2                   = static_cast<float>(header["z2"]);

        this->atsh.rho_probe_ohm        = static_cast<float>(header["rho_probe_ohm"]);
        this->atsh.DC_offset_voltage_mV = static_cast<float>(header["DC_offset_voltage_mV"]);
        this->atsh.gain_stage1          = static_cast<float>(header["gain_stage1"]);
        this->atsh.gain_stage2          = static_cast<float>(header["gain_stage2"]);

        this->atsh.iLat_ms              = static_cast<int32_t>(header["iLat_ms"]);
        this->atsh.iLong_ms             = static_cast<int32_t>(header["iLong_ms"]);
        this->atsh.iElev_cm             = static_cast<int32_t>(header["iElev_cm"]);

        this->atsh.Lat_Long_TYPE        = header["Lat_Long_TYPE"].get<std::string>().at(0);
        this->atsh.coordinate_type      = header["coordinate_type"].get<std::string>().at(0);

        this->atsh.ref_meridian         = static_cast<int16_t>(header["ref_meridian"]);

        this->atsh.Northing             = static_cast<double>(header["Northing"]);
        this->atsh.Easting              = static_cast<double>(header["Easting"]);
        this->atsh.gps_clock_status     = header["gps_clock_status"].get<std::string>().at(0);
        this->atsh.GPS_accuracy         = header["GPS_accuracy"].get<std::string>().at(0);
        this->atsh.offset_UTC           = static_cast<int16_t>(header["offset_UTC"]);

        std::string official_name = header["SystemType"].get<std::string>(); //like ADU-08e
        std::string header_name;
        for (const auto &name : ats_sys_names) {
            if (name.second == official_name) {
                header_name = name.first;
            }
        }
        for (auto & c: header_name) c = toupper(c);
        bool conv_from_06 = false;
        if (header_name == "ADU06") {
            conv_from_06 = true;
            header_name = "ADU07";
        }

        // make filters, where ADU-07 and ADU-06 use ADU-07e filters
        this->set_filter_bank(header["SystemType"]);
        strncpy(this->atsh.SystemType, header_name.c_str(), sizeof(this->atsh.SystemType));

        strncpy(this->atsh.survey_header_filename, header["survey_header_filename"].get<std::string>().c_str(), sizeof(this->atsh.survey_header_filename));
        strncpy(this->atsh.type_of_meas, header["type_of_meas"].get<std::string>().c_str(), sizeof(this->atsh.type_of_meas));
        this->atsh.DCOffsetCorrValue    = static_cast<double>(header["DCOffsetCorrValue"]);
        this->atsh.DCOffsetCorrOn       = static_cast<int8_t>(header["DCOffsetCorrOn"]);
        this->atsh.InputDivOn           = static_cast<int8_t>(header["InputDivOn"]);
        this->atsh.bit_indicator        = static_cast<int16_t>(header["bit_indicator"]);
        strncpy(this->atsh.result_selftest, header["result_selftest"].get<std::string>().c_str(), sizeof(this->atsh.result_selftest));
        this->atsh.numslices            = static_cast<uint16_t>(header["numslices"]);
        this->atsh.cal_freqs            = static_cast<int16_t>(header["cal_freqs"]);
        this->atsh.cal_entry_length     = static_cast<int16_t>(header["cal_entry_length"]);
        this->atsh.cal_version          = static_cast<int16_t>(header["cal_version"]);
        this->atsh.cal_start_address    = static_cast<int16_t>(header["cal_start_address"]);


        this->set_lf_filter_int(header["LF_filters"]);


        strncpy(this->atsh.UTMZone, header["UTMZone"].get<std::string>().c_str(), sizeof(this->atsh.UTMZone));
        this->atsh.system_cal_datetime  = static_cast<uint32_t>(header["system_cal_datetime"]);
        strncpy(this->atsh.sensor_cal_filename, header["sensor_cal_filename"].get<std::string>().c_str(), sizeof(this->atsh.sensor_cal_filename));

        this->atsh.sensor_cal_datetime  = static_cast<uint32_t>(header["sensor_cal_datetime"]);
        this->atsh.powerline1           = static_cast<float>(header["powerline1"]);
        this->atsh.powerline2           = static_cast<float>(header["powerline2"]);

        this->set_hf_filter_int(header["HF_filters"]);

        this->atsh.external_gain  =        static_cast<float>(header["external_gain"]);
        strncpy(this->atsh.ADB_board_type, header["ADB_board_type"].get<std::string>().c_str(), sizeof(this->atsh.ADB_board_type));


        if (conv_from_06) {
            std::string s = header["Comments"];
            header["Comments"] = "converted from ADU-06";
            if (s.size()) {
                header["Comments"] += "; " + s;
            }
        }

        strncpy(this->atsh.comments.Client, header["Client"].get<std::string>().c_str(), sizeof(this->atsh.comments.Client));
        strncpy(this->atsh.comments.Contractor, header["Contractor"].get<std::string>().c_str(), sizeof(this->atsh.comments.Contractor));
        strncpy(this->atsh.comments.Area, header["Area"].get<std::string>().c_str(), sizeof(this->atsh.comments.Area));
        strncpy(this->atsh.comments.SurveyID, header["SurveyID"].get<std::string>().c_str(), sizeof(this->atsh.comments.SurveyID));
        strncpy(this->atsh.comments.Operator, header["Operator"].get<std::string>().c_str(), sizeof(this->atsh.comments.Operator));
        strncpy(this->atsh.comments.SiteName, header["SiteName"].get<std::string>().c_str(), sizeof(this->atsh.comments.SiteName));
        strncpy(this->atsh.comments.XmlHeader, header["XmlHeader"].get<std::string>().c_str(), sizeof(this->atsh.comments.XmlHeader));
        strncpy(this->atsh.comments.Comments, header["Comments"].get<std::string>().c_str(), sizeof(this->atsh.comments.Comments));
        strncpy(this->atsh.comments.SiteNameRR, header["SiteNameRR"].get<std::string>().c_str(), sizeof(this->atsh.comments.SiteNameRR));
        strncpy(this->atsh.comments.SiteNameEMAP, header["SiteNameEMAP"].get<std::string>().c_str(), sizeof(this->atsh.comments.SiteNameEMAP));




    }


};


#endif //ATSHEADER_DEF
