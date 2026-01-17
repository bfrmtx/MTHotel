#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include "atmm.hpp"
#include "cal_base.hpp"
#include "freqs.hpp"
#include "json.hpp"
#include "mt_base.hpp"
#include "strings_etc.hpp"
////
#include <bitset>
#include <chrono>
#include <cmath>
#include <complex>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <list>
#include <mutex>
#include <queue>
#include <shared_mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <fftw3.h>

/**
 * @file atss.h
 * @brief provides the atss format and the JSON description of the atss format and the timer class
 */

// forward declaration of stats struct, needed in tsplotter
struct stats {
  size_t wl = 1024; //!< window length of FFT, in case of zero padding this is more than rl
  size_t rl = 1024; //!< window length and range length
  size_t max_samples = 0;
  std::pair<int64_t, int64_t> read_pos{0, 0}; //!< read sample position in the file, start ... < end; shared between all channels
  bool detrend = false;
  bool is_locked = false;
  bool show_spectra = false;
  bool show_ts = true;
  bool cal_on = false;
};

using jsn = nlohmann::ordered_json;

/*!
 * \brief The p_timer class allows date functions with fractions of seconds
 * remind: 12:00:00.0001 is maybe a bad start time! when ever possible, shift your samples so that you get full seconds
 * @details I HAVE THE CHOICE : START TIME AT FULL SECOND AND COMPATIBLE WITH SECONDS SINCE 1970 or faulty interfaces
 * I decided to use the full second, so we have no problems with seconds since 1970 with others
 * I must deep think about FIR filters at long periods, because they are odd.
 */

class p_timer {

public:
  /*!
   * \brief p_timer empty constructor
   */
  p_timer() = default;

  /*!
   * \brief p_timer default constructor
   * \param datetime
   * \param sample_rate
   * \param fracs
   */
  p_timer(const std::string &datetime, const double &sample_rate, double &fracs) {
    this->set_datetime_sample_rate_fracs(datetime, sample_rate, fracs);
  }

  /*!
   * \brief set_datetime_sample_rate_fracs
   * \param datetime ISO string like 2009-08-20T13:22:01 NOT 2009-08-20T13:22:01.0034
   * \param sample_rate in Hz
   * \param fracs like 0.0034
   */
  void set_datetime_sample_rate_fracs(const std::string &datetime, const double &sample_rate, const double &fracs) {
    this->tt = mstr::iso8601_to_time_t(datetime);
    this->sample_rate = sample_rate;
    if (fracs < mstr::zero_frac)
      this->fracs = 0.0;
    else
      this->fracs = fracs;
  }

  /*!
   * \brief p_timer copy constructor
   * \param rhs p_timer
   */
  p_timer(const p_timer &rhs) {
    this->tt = rhs.tt;
    this->fracs = rhs.fracs;
    this->sample_rate = rhs.sample_rate;
    this->samples = rhs.samples;
    this->spos = rhs.spos;
  }

  /*!
   * \brief p_timer copy constructor from a shared pointer
   * \param rhs
   */
  p_timer(const std::shared_ptr<p_timer> &rhs) {
    if (rhs != nullptr) {
      this->tt = rhs->tt;
      this->fracs = rhs->fracs;
      this->sample_rate = rhs->sample_rate;
      this->samples = rhs->samples;
      this->spos = rhs->spos;
    }
  }

  void clear() {
    this->tt = 0;
    this->fracs = 0.0;
    this->sample_rate = 0.0;
    this->samples = 0;
    this->spos = 0;
  }

  p_timer operator=(const p_timer &rhs) {
    if (&rhs == this)
      return *this;
    this->tt = rhs.tt;
    this->fracs = rhs.fracs;
    this->sample_rate = rhs.sample_rate;
    this->samples = rhs.samples;
    this->spos = rhs.spos;
    return *this;
  }

  /*!
   * @brief compares the START TIMES of two p_timer objects
   * @param rhs
   * @return true if earlier than rhs
   */
  bool operator<(const p_timer &rhs) const {
    if (this->tt < rhs.tt)
      return true; // second diff
    if (this->tt > rhs.tt)
      return false;                   // second diff
    return (this->fracs < rhs.fracs); // else compare sub samples
  }

  /*!
   * @brief compares the START TIMES of two p_timer objects
   * @param rhs
   * @return true if later than rhs
   */
  bool operator>(const p_timer &rhs) const {
    if (this->tt > rhs.tt)
      return true; // second diff
    if (this->tt < rhs.tt)
      return false;                   // second diff
    return (this->fracs > rhs.fracs); // else compare sub samples
  }

  p_timer operator+(const p_timer &rhs) const {
    p_timer nt(*this);
    nt.tt += rhs.tt;
    double f = nt.fracs + rhs.fracs;
    if (f < 1.0)
      nt.fracs = f;
    else {
      double fullsecs;
      nt.fracs = modf(f, &fullsecs);
      nt.tt += (uint64_t)fullsecs;
    }
    return nt;
  }

  p_timer operator+=(const p_timer &rhs) {
    this->tt += rhs.tt;
    double f = this->fracs + rhs.fracs;
    if (f < 1.)
      this->fracs = f;
    else {
      double fullsecs;
      this->fracs = modf(f, &fullsecs);
      this->tt += (uint64_t)fullsecs;
    }
    return *this;
  }

  p_timer add_secs(const int64_t &secs, const double fracs) {
    this->tt += secs;
    double f = this->fracs + fracs;
    if (f < 1.)
      this->fracs = f;
    else {
      double fullsecs;
      this->fracs = modf(f, &fullsecs);
      this->tt += (uint64_t)fullsecs;
    }
    return *this;
  }

  p_timer operator-(const p_timer &rhs) const {
    p_timer nt(*this);
    nt.tt -= rhs.tt;
    double f = nt.fracs - rhs.fracs;
    if (f > 0.0)
      nt.fracs = f;
    else {
      double fullsecs;
      nt.fracs = modf(f, &fullsecs);
      nt.tt -= (uint64_t)fullsecs;
    }
    return nt;
  }

  p_timer operator-=(const p_timer &rhs) {
    this->tt -= rhs.tt;
    double f = this->fracs - rhs.fracs;
    if (f > 0.0)
      this->fracs = f;
    else {
      double fullsecs;
      this->fracs = modf(f, &fullsecs);
      this->tt -= (uint64_t)fullsecs;
    }
    return *this;
  }

  /*!
   * @brief compares two p_timer objects for equality
   * @param rhs
   * @return true if equal START and STOP times !!!
   */
  bool operator==(const p_timer &rhs) const {
    if (this->sample_rate != rhs.sample_rate)
      return false;
    if (this->samples != rhs.samples)
      return false;
    return ((this->tt == rhs.tt) && (this->fracs == rhs.fracs));
  }

  /*!
   * @brief returns the timer at the specified sample index
   * @param nth_sample
   * @details p_timer.tt AND p_timer.fracs are SET! For intermediate usage.
   * @return timer at sample nth_sample or an empty timer if nth_sample is out of range;
   */
  p_timer sample_time(const size_t nth_sample) const {
    p_timer nt(*this); // copy current timer
    if (nth_sample <= this->samples)
      nt.spos = nth_sample;
    else
      return p_timer();

    double fullsecs, tsseg = (double(nt.spos)) / this->sample_rate;
    nt.fracs = modf(tsseg, &fullsecs); // fractions are set
    nt.tt += (uint64_t)fullsecs;       // add full seconds - a new object is born, it is a NEW START TIME!
    return nt;
  }

  int64_t duration_to_samples(const p_timer &dur) {
    double pos1 = double(dur.tt) * this->sample_rate;
    double pos2 = 0;
    if (dur.fracs > treat_as_null) {
      pos2 = dur.fracs * this->sample_rate;
    }

    return (int64_t)pos1 + (int64_t)pos2;
  }

  /*!
   * @brief returns the p_timer at the start of the channel
   * @details check that sample_rate and samples for both channels if you want to compare two channels !!! <br>
   * use also if samples are still zero !!!
   * @return start p_timer (sample 0)
   */
  p_timer p_start() const {
    return this->sample_time(0);
  }

  p_timer p_stop() const {
    return this->sample_time(this->samples);
  }

  void iso8601_to_time_t(const std::string &datetime) {
    this->tt = mstr::iso8601_to_time_t(datetime);
    this->fracs = 0.0;
    std::string sfrac = mstr::get_fractional_seconds_from_iso8601(datetime);
    if (!sfrac.empty()) {
      try {
        this->fracs = std::stod(sfrac);
      } catch (const std::exception &e) {
        std::cerr << "iso8601_to_time_t: Error parsing fractional seconds: " << e.what() << std::endl;
        this->fracs = 0.0; // reset to zero on error
      }
    }
  }

  std::string sample_datetime(const size_t &sample, const int iso_0_date_1_time_2 = 0) const {
    auto nt = this->sample_time(sample);
    return mstr::iso8601_time_t(nt.tt, iso_0_date_1_time_2, nt.fracs);
  }

  std::string stop_datetime(const int iso_0_date_1_time_2 = 0) const {
    auto nt = this->sample_time(this->samples);
    return mstr::iso8601_time_t(nt.tt, iso_0_date_1_time_2, nt.fracs);
  }

  /*!
   * \brief start_datetime
   * \param iso_0_date_1_time_2
   * \return
   */
  std::string start_datetime(const int iso_0_date_1_time_2 = 0) const {
    auto nt = this->sample_time(0);
    return mstr::iso8601_time_t(nt.tt, iso_0_date_1_time_2, nt.fracs);
  }

  std::string brief() const {
    std::stringstream ss;
    ss << this->start_datetime() << " <-> " << this->stop_datetime() << " @" << mstr::sample_rate_to_str_simple(this->sample_rate);
    auto dur = this->p_stop() - this->p_start();
    std::chrono::seconds sd(dur.tt);
    const auto days = duration_cast<std::chrono::days>(sd);
    const auto hrs = duration_cast<std::chrono::hours>(sd - days);
    const auto mins = duration_cast<std::chrono::minutes>(sd - days - hrs);
    const auto secs = duration_cast<std::chrono::seconds>(sd - days - hrs - mins);

    ss << " " << days.count() << "days " << hrs.count() << "hrs " << mins.count() << "mins " << secs.count() << "secs ";

    return ss.str();
  }

  /*!
   * \brief set_max_samples convinence function for tests ONLY, where you don't have a file or scanning
   * \param max_samples
   */
  void set_max_samples(const size_t max_samples) {
    this->samples = max_samples;
  }

  // 2007-12-24T18:21:00.01234 is probably not supported by C/C++/Python/PHP and others COMPLETELY, eg. fraction of seconds is a problem
  time_t tt = 0;            //!< generates the ISO 8601 in UTC , as time_t seconds since 1970, can be negative in some implementaions
  double fracs = 0.0;       //!< fractions of seconds from 0 ...0.9999... > 1 MHz may assumed as mstr::zero_frac
  double sample_rate = 0.0; //!< in Hz always
  size_t samples = 0;       //!< total samples of the file, e.g. CALCULATED from file size.
  size_t spos = 0;          //!< sample pos
};

//
//
//
// *****************************************************  C H A N N E L *********************************************************
//
//
//

/*!
 * \brief The channel class is the FILENAME part of the atss format consisting of binary .atss, JSON .json
 * the tags of the filename are NOT repeated in the JSON
 * For more information, visit: https://example.com/documentation
 * A channel also need all information to be treated as unique identifier.
 */
class channel {

public:
  /*!
   * \brief channel create an empty (unusable) channel
   */
  channel() {
  }

  ~channel() {
    if (this->infile.is_open())
      this->infile.close();
    if (this->outfile.is_open())
      this->outfile.close();
    if (this->fft_freqs != nullptr)
      this->fft_freqs.reset();
  }

  /*!
   * \brief channel most common minimum constructor
   * \param channel_type like Ex, Ex, ... Hx ..
   * \param sample_rate in Hz always
   */
  channel(const std::string &channel_type, const double &sample_rate, const std::string &datetime = "1970-01-01T00:00:00", const double &fracs = 0.0) {
    this->set_channel_type(channel_type);
    this->set_sample_rate(sample_rate);
    double f = fracs;
    this->set_datetime(datetime, f);
  }

  void from_ats(const std::string &channel_type, const double &sample_rate, const int64_t utc = 0, const double &fracs = 0.0) {
    this->set_channel_type(channel_type);
    this->set_sample_rate(sample_rate);
    this->pt.tt = utc;
    this->pt.fracs = fracs;
  }

  /*!
   * @brief copy constructor
   * @param rhs
   */
  channel(const std::shared_ptr<channel> &rhs) {

    if (rhs != nullptr) {

      this->pt = rhs->pt; // copy constructor, that is not a pointer here
      this->filepath_wo_ext = rhs->filepath_wo_ext;

      this->cal = std::make_shared<calibration>(rhs->cal);
      if (rhs->fft_freqs != nullptr)
        this->fft_freqs = std::make_shared<fftw_freqs>(rhs->fft_freqs);

      this->latitude = rhs->latitude;
      this->longitude = rhs->longitude;
      this->elevation = rhs->elevation;
      this->angle = rhs->angle;
      this->tilt = rhs->tilt;
      this->resistance = rhs->resistance;
      this->units = rhs->units;
      this->filter = rhs->filter;
      this->source = rhs->source;
      this->serial = rhs->serial;
      this->system = rhs->system;
      this->channel_no = rhs->channel_no;
      this->channel_type = rhs->channel_type;
      // we the later for system calibration
      this->board = rhs->board;
      this->radio_filter = rhs->radio_filter;
      this->low_pas_filter = rhs->low_pas_filter;
      this->high_pas_filter = rhs->high_pas_filter;
      this->input_divider = rhs->input_divider;
      this->gain_1 = rhs->gain_1;
      this->gain_2 = rhs->gain_2;
      this->mode = rhs->mode;
      this->is_emap = rhs->is_emap;
      this->is_remote = rhs->is_remote;
    }
  }

  /*!
   * \brief channel
   * \param in_file the file name without extension, the extension is added automatically
   */
  channel(const std::filesystem::path &in_file) {

    std::filesystem::path json_file(in_file);
    std::filesystem::path atss_file(in_file);
    // Ensure the paths have the correct extensions, even if there was no extension before
    if (json_file.extension() != ".json") {
      json_file.replace_extension(".json");
    }
    if (atss_file.extension() != ".atss") {
      atss_file.replace_extension(".atss");
    }
    // both must exist, otherwise we cannot read the header and the data
    if (!std::filesystem::exists(json_file)) {
      std::cerr << "JSON file does not exist: " << json_file << std::endl;
      return;
    }
    if (!std::filesystem::exists(atss_file)) {
      std::cerr << "ATSS file does not exist: " << atss_file << std::endl;
      std::cerr << "Samples will be set to 0" << std::endl;
      this->pt.samples = 0;
    }

    if (this->parse_json_filename(json_file)) {
      std::ifstream file;
      file.open(json_file, std::fstream::in);
      if (!file.is_open()) {
        file.close();
        return;
      }
      // so this->system, this->serial, this->channel_no, this->channel_type are set
      nlohmann::ordered_json head = nlohmann::ordered_json::parse(file);
      file.close();
      if (head.contains("latitude") && head.contains("longitude") && head.contains("elevation")) {
        this->set_lat_lon_elev(head["latitude"], head["longitude"], head["elevation"]);
      }
      this->pt.iso8601_to_time_t(head["datetime"]);
      this->filepath_wo_ext = json_file;
      this->filepath_wo_ext.replace_extension("");
      this->samples(this->filepath_wo_ext);
      if (head.contains("angle"))
        this->angle = head["angle"];
      if (head.contains("tilt"))
        this->tilt = head["tilt"];
      if (head.contains("resistance"))
        this->resistance = head["resistance"];
      if (head.contains("units"))
        this->units = head["units"];
      if (head.contains("filter"))
        this->filter = head["filter"];
      if (head.contains("source"))
        this->source = head["source"];
      // check if the atss file exists
      if (std::filesystem::exists(atss_file)) {
        auto xx = std::filesystem::file_size(atss_file);
        this->pt.samples = xx / sizeof(double);
      }
      // check the filters
      this->set_filter(false);
      if (this->cal != nullptr)
        this->cal.reset();
      this->cal = std::make_shared<calibration>();
      this->cal->parse_head(head, json_file);
    }
  }

  /*!
   * \brief set_filter this can be done ONLY after teh header is read!
   */
  void set_filter(const bool apply_corrections = false) {
    // split the filter string into a vector of strings
    std::vector<std::string> filters;
    std::vector<std::string> new_filters;

    // this->filter is a comma separated string like "hs, hb, fil3", we split it into a vector of strings
    std::string s = this->filter;
    std::string delimiter = ",";
    size_t pos = 0;
    std::string token;
    while ((pos = s.find(delimiter)) != std::string::npos) {
      token = s.substr(0, pos);
      filters.push_back(token);
      s.erase(0, pos + delimiter.length());
    }
    if (s.size())
      filters.push_back(s);
    // if the filter contains "_RF_" , we set radio_filter

    // set the board type to HF or LF
    for (const auto &sfilter : filters) {
      if (mstr::contains(sfilter, "ADB-HF")) {
        this->board = ADU::HF;
        new_filters.push_back("ADB-HF");
      } else if (mstr::contains(sfilter, "ADB-LF")) {
        this->board = ADU::LF;
        new_filters.push_back("ADB-LF");
      } else if (mstr::contains(sfilter, "ADB-MF")) {
        this->board = ADU::MF;
        new_filters.push_back("ADB-MF");
      }
    }

    for (const auto &sfilter : filters) {
      // if the filter contains "_RF_" , we set radio_filter
      if (mstr::contains(sfilter, "LF-RF-1")) {
        this->radio_filter = ADU::LF_RF_1;
        new_filters.push_back("LF-RF-1");
      } else if (mstr::contains(sfilter, "LF-RF-2")) {
        this->radio_filter = ADU::LF_RF_2;
        new_filters.push_back("LF-RF-2");
      } else if (mstr::contains(sfilter, "LF-RF-3")) {
        this->radio_filter = ADU::LF_RF_3;
        new_filters.push_back("LF-RF-3");
      } else if (mstr::contains(sfilter, "LF-RF-4")) {
        this->radio_filter = ADU::LF_RF_4;
        new_filters.push_back("LF-RF-4");
      } else if (mstr::contains(sfilter, "MF-RF-1")) {
        this->radio_filter = ADU::MF_RF_1;
        new_filters.push_back("MF-RF-1");
      } else if (mstr::contains(sfilter, "MF-RF-2")) {
        this->radio_filter = ADU::MF_RF_2;
        new_filters.push_back("MF-RF-2");
      }
    }
    for (const auto &sfilter : filters) {
      // if the filter contains "_LP_" , we set low_pas_filter
      if (mstr::contains(sfilter, "LF-LP-4Hz")) {
        this->low_pas_filter = ADU::LF_LP_4Hz;
        new_filters.push_back("LF-LP-4Hz");
      }
    }
    for (const auto &sfilter : filters) {
      // if the filter contains "_HP_" , we set high_pas_filter
      if (mstr::contains(sfilter, "HF-HP-1Hz")) {
        this->high_pas_filter = ADU::HF_HP_1Hz;
        new_filters.push_back("HF-HP-1Hz");
      } else if (mstr::contains(sfilter, "HF-HP-500Hz")) {
        this->high_pas_filter = ADU::HF_HP_500Hz;
        // the default is 1Hz for ADU-07 and ADU-07e
        new_filters.push_back("HF-HP-500Hz");
      }
    }
    // if the filter contains "_div_" , we set input_divider
    for (const auto &sfilter : filters) {
      if (mstr::contains(sfilter, "div_1")) {
        this->input_divider = ADU::div_1;
        new_filters.push_back("div-1");
      } else if (mstr::contains(sfilter, "div_8")) {
        this->input_divider = ADU::div_8;
        new_filters.push_back("div_8");
      }
    }
    // if the filter contains "_gain_" , we set gain_1 and gain_2
    for (const auto &sfilter : filters) {
      if (mstr::contains(sfilter, "gain_1")) {
        // filter can be gain_1_1 or gain_1_4 ... 8 16 32 64
        if (mstr::contains(sfilter, "gain_1_1")) {
          this->gain_1 = ADU::gain_1_1;
          new_filters.push_back("gain_1_1");
        } else if (mstr::contains(sfilter, "gain_1_4")) {
          this->gain_1 = ADU::gain_1_4;
          new_filters.push_back("gain_1_4");
        } else if (mstr::contains(sfilter, "gain_1_8")) {
          this->gain_1 = ADU::gain_1_8;
          new_filters.push_back("gain_1_8");
        } else if (mstr::contains(sfilter, "gain_1_16")) {
          this->gain_1 = ADU::gain_1_16;
          new_filters.push_back("gain_1_16");
        } else if (mstr::contains(sfilter, "gain_1_32")) {
          this->gain_1 = ADU::gain_1_32;
          new_filters.push_back("gain_1_32");
        } else if (mstr::contains(sfilter, "gain_1_64")) {
          this->gain_1 = ADU::gain_1_64;
          new_filters.push_back("gain_1_64");
        }
        // and we set this->
      } else if (mstr::contains(sfilter, "gain_2")) {
        // filter can be gain_2_1 or gain_2_4 ... 8 16 32 64
        if (mstr::contains(sfilter, "gain_2_1")) {
          this->gain_2 = ADU::gain_2_1;
          new_filters.push_back("gain_2_1");
        } else if (mstr::contains(sfilter, "gain_2_4")) {
          this->gain_2 = ADU::gain_2_4;
          new_filters.push_back("gain_2_4");
        } else if (mstr::contains(sfilter, "gain_2_8")) {
          this->gain_2 = ADU::gain_2_8;
          new_filters.push_back("gain_2_8");
        } else if (mstr::contains(sfilter, "gain_2_16")) {
          this->gain_2 = ADU::gain_2_16;
          new_filters.push_back("gain_2_16");
        } else if (mstr::contains(sfilter, "gain_2_32")) {
          this->gain_2 = ADU::gain_2_32;
          new_filters.push_back("gain_2_32");
        } else if (mstr::contains(sfilter, "gain_2_64")) {
          this->gain_2 = ADU::gain_2_64;
          new_filters.push_back("gain_2_64");
        }
      }
    }
    // if the filter contains "_direct_" , we set mode
    for (const auto &sfilter : filters) {
      if (mstr::contains(sfilter, "direct_mode_on")) {
        this->mode = ADU::direct_mode_on;
        new_filters.push_back("direct_mode_on");
      } else if (mstr::contains(sfilter, "direct_mode_off")) {
        this->mode = ADU::direct_mode_off;
        new_filters.push_back("direct_mode_off");
      }
    }

    // // 2025 - seems  forgotten to note the filters in the header for HF
    // if (apply_corrections) {
    //   if (mstr::contains(this->system, "ADU-07") && this->board == ADU::HF && this->high_pas_filter == ADU::off) {
    //     this->high_pas_filter = ADU::HF_HP_1Hz;
    //     new_filters.push_back("HF-HP-1Hz");
    //   }
    //   if (mstr::contains(this->system, "ADU-08") && this->board == ADU::HF && this->high_pas_filter == ADU::off) {
    //     this->high_pas_filter = ADU::HF_HP_500Hz;
    //     new_filters.push_back("HF-HP-500Hz");
    //   }
    // }
    this->filter.clear();
    for (const auto &sfilter : new_filters) {
      this->filter += sfilter;
      this->filter += ",";
    }
    this->filter.pop_back(); // remove the last comma
  }

  /*!
   * @brief set_high_pas_filter need some overwrite for airborne systems
   * @param filter
   */
  void
  set_high_pas_filter(const ADU &filter) {
    this->high_pas_filter = filter;
  }

  void reset_filters() {
    this->radio_filter = ADU::off;
    this->low_pas_filter = ADU::off;
    this->high_pas_filter = ADU::off;
    this->input_divider = ADU::off;
    this->gain_1 = ADU::off;
    this->gain_2 = ADU::off;
    this->board = ADU::off;
    this->mode = ADU::off;
    this->filter.clear();
  }

  /*!
   * @brief Set the date and time
   * @param datetime
   * @param fracs here you can explicitly set the fractions of seconds, without control
   */
  void set_datetime(const std::string &datetime = "1970-01-01T00:00:00", const double &fracs = 0.0) {
    this->pt.tt = mstr::iso8601_to_time_t(datetime);
    this->pt.fracs = fracs;
  }

  /*!
   * @brief Set the Unix timestamp
   * @param tt
   * @param fracs here you can explicitly set the fractions of seconds, without control
   */
  void set_unix_timestamp(const time_t tt, const double &fracs = 0) {
    this->pt.tt = tt;
    this->pt.fracs = fracs;
  }

  uint64_t get_unix_timestamp() const {
    return this->pt.tt;
  } // seconds since 1970

  std::string get_datetime() const {
    return this->pt.start_datetime();
  }

  std::string stop_datetime(const int iso_0_date_1_time_2 = 0) const {
    return this->pt.stop_datetime(iso_0_date_1_time_2);
  }

  std::string start_datetime(const int iso_0_date_1_time_2 = 0) const {
    return this->pt.start_datetime(iso_0_date_1_time_2);
  }

  void set_cal(const std::shared_ptr<calibration> &cal) {
    if (this->cal != nullptr)
      this->cal.reset();
    this->cal = cal;
  }

  void set_base_file(const std::string &system = "", const int64_t &ser = 0, const int64_t &channel_no = 0) {
    this->set_system(system);
    this->set_serial(ser);
    this->set_channel_no(channel_no);
  }
  /*!
   * \brief parse_json_filename - put it in a try block
   * \param in_json_file
   * \return
   */
  bool parse_json_filename(const std::filesystem::path &in_json_file) {

    if (!std::filesystem::exists(in_json_file)) {
      std::ostringstream err_str((std::string("channel::") + __func__), std::ios_base::ate);
      err_str << "::file not exists " << in_json_file;
      throw std::runtime_error(err_str.str());
    }

    std::stringstream ss(in_json_file.stem().string());
    std::string item;
    size_t i = 0;
    while (std::getline(ss, item, '_')) {
      if (i == 0)
        this->serial = size_t(std::abs(std::stol(item)));
      else if (i == 1)
        this->system = item;
      else if (i == 2)
        this->channel_no = size_t(std::abs(std::stol(item.substr(1))));
      else if (i == 3)
        this->channel_type = item.substr(1);
      else if (i == 4)
        this->pt.sample_rate = mstr::str_to_sample_rate(item);
      ++i;
    }

    if (i != 5) {
      std::ostringstream err_str((std::string("channel::") + __func__), std::ios_base::ate);
      err_str << "::error parsing filename " << in_json_file;
      this->pt.clear();
      throw std::runtime_error(err_str.str());
    }

    this->filepath_wo_ext = in_json_file;
    this->filepath_wo_ext.replace_extension("");

    return true;
  }

  std::filesystem::path get_json_filepath() const {
    std::filesystem::path jsn(this->filepath_wo_ext.parent_path());
    return jsn /= this->filename(".json");
  }

  std::filesystem::path get_atss_filepath() const {
    std::filesystem::path atsf(this->filepath_wo_ext.parent_path());
    return atsf /= this->filename(".atss");
  }

  std::filesystem::path get_filepath_wo_ext() const {
    return this->filepath_wo_ext;
  }

  std::filesystem::path get_run_dir() const {
    return this->filepath_wo_ext.parent_path();
  }

  std::filesystem::path get_station_dir() const {
    return this->filepath_wo_ext.parent_path().parent_path();
  }

  std::filesystem::path get_station_name() const {
    return this->filepath_wo_ext.parent_path().parent_path().filename();
  }

  std::filesystem::path get_survey_dir() const {
    return this->filepath_wo_ext.parent_path().parent_path().parent_path();
  }
  std::filesystem::path get_survey_name() const {
    return this->filepath_wo_ext.parent_path().parent_path().parent_path().filename();
  }

  // simple set and get

  void set_serial(const auto &ser) {
    this->serial = size_t(ser);
  }

  size_t get_serial() const {
    return this->serial;
  }

  std::string get_serial_str(const unsigned int FieldWidth) const {
    return mstr::zero_fill_field(this->serial, FieldWidth);
  }

  void set_system(const std::string &system) {
    this->system = mstr::simplify(system, true);
  }

  std::string get_system() {
    return this->system;
  }

  std::string get_sensor() const {
    if (this->cal != nullptr)
      return this->cal->sensor;
    return std::string();
  }

  void set_channel_type(const std::string &channel_type) {
    this->channel_type = mstr::simplify(channel_type, true);
  }

  /*!
   * @brief returns the channel type, like Ex, Hx, Vx, etc.
   * \note this is used for the filename, so it is always simplified
   * @return
   */
  std::string get_channel_type() const {
    return this->channel_type;
  }

  /*!
   * @brief get the channel type classification of electric, magnetic, etc.
   * @param type string like E, H, J or x, where x is a wildcard for x, y, z, U for Euler angles Pitch, Roll, Yaw
   * @return
   */
  bool is_type(const std::string &type) const {
    if (type.size() > 1) {
      std::cerr << "channel::is_type: type string is too long, must be 1: " << type << std::endl;
      return false; // only one char allowed
    }
    if (type == "E")
      return (this->channel_type[0] == 'E' || this->channel_type[0] == 'e');
    if (type == "H")
      return (this->channel_type[0] == 'H' || this->channel_type[0] == 'h');
    if (type == "J")
      return (this->channel_type[0] == 'J' || this->channel_type[0] == 'j');
    if (type == "x")
      return (this->channel_type[0] == 'x' || this->channel_type[0] == 'X' || this->channel_type[0] == 'y' ||
              this->channel_type[0] == 'Y' || this->channel_type[0] == 'z' || this->channel_type[0] == 'Z');
    // temperature
    if (type == "T") {
      return (this->channel_type[0] == 'T' || this->channel_type[0] == 't');
    }
    // Euler angles Y(aw) Uy, P(itch) Up, R(oll) Ur
    if (type == "U") {
      return (this->channel_type[0] == 'U' || this->channel_type[0] == 'u');
    }
    // if we reach here, we have no match
    std::cerr << "channel::is_type: unknown type: " << type << std::endl;
    return false;
  }

  std::filesystem::path set_dir(const std::filesystem::path &dir_only) {
    this->filepath_wo_ext = dir_only / this->filename();
    return this->filepath_wo_ext;
  }

  bool set_run(const auto &run) {

    bool success = false;
    std::filesystem::path newrun = this->filepath_wo_ext.parent_path().parent_path();
    if (newrun == filepath_wo_ext.root_path()) {
      std::ostringstream err_str((std::string("channel::") + __func__), std::ios_base::ate);
      err_str << "::can not create run, I am a root dir";
      throw std::runtime_error(err_str.str());
    }
    newrun /= mstr::run2string(run, run_digits);
    if (!std::filesystem::exists(newrun)) {
      success = std::filesystem::create_directory(newrun);
    } else
      success = true;

    if (!success) {
      std::ostringstream err_str((std::string("channel::") + __func__), std::ios_base::ate);
      err_str << "::can not create run " << mstr::zero_fill_field(run, 3);
      throw std::runtime_error(err_str.str());
    }
    this->filepath_wo_ext = newrun / this->filepath_wo_ext.filename();
    return success;
  }

  size_t get_run() const {
    std::filesystem::path srun = this->filepath_wo_ext.parent_path();
    if (srun == filepath_wo_ext.root_path())
      return SIZE_MAX;
    return mstr::string2run(srun.filename().string());
  }

  void set_channel_no(const auto &channel_no) {
    this->channel_no = size_t(channel_no);
  }

  /*!
   * @brief get_channel_no returns the channel number as a size_t; that is faster than parsing the file name
   * @return
   */
  size_t get_channel_no() const {
    return this->channel_no;
  }

  /*!
   * @brief get_channel_no_str returns the channel number as a like 003 0r 012
   * \note this is used for the filename, so it is always 3 digits
   * @return
   */
  std::string get_channel_no_str(bool prepend_C = false, unsigned int FieldWidth = 3) const {
    if (prepend_C)
      return "C" + mstr::zero_fill_field(this->channel_no, FieldWidth);
    else
      return mstr::zero_fill_field(this->channel_no, FieldWidth);
  }

  /*!
   * @brief get_channel_no_pair returns the channel number and type as a pair of size_t and string for building a map;
   * @return
   */
  std::pair<size_t, std::string> get_channel_no_type() const {
    return std::make_pair(this->channel_no, this->channel_type);
  }

  double set_sample_rate(const double &sample_rate) {

    if (sample_rate < treat_as_null) {
      std::ostringstream err_str((std::string("channel::") + __func__), std::ios_base::ate);
      err_str << ":: you can not have NULL negative frequencies";
      throw std::runtime_error(err_str.str());
    } else
      this->pt.sample_rate = sample_rate;
    return this->pt.sample_rate;
  }

  double get_sample_rate() const {
    return this->pt.sample_rate;
  }

  std::string get_sample_rate_str(const bool add_space = false, const bool append_space = false) const {
    return mstr::sample_rate_to_str_simple(this->pt.sample_rate, add_space, append_space);
  }

  /*!
   * \brief filename can be used to create files
   * \param extension_wo_dot
   * \return
   */
  std::string filename(const std::string &extension_with_dot = "") const {

    if (this->pt.sample_rate < treat_as_null) {
      std::ostringstream err_str((std::string("channel::") + __func__), std::ios_base::ate);
      err_str << "::you can not have NULL or negative frequencies";
      throw std::runtime_error(err_str.str());
    }
    std::string str;
    str += mstr::zero_fill_field(this->serial, 3) + "_";
    str += this->system + "_";
    str += "C" + mstr::zero_fill_field(this->channel_no, 3) + "_";
    str += "T" + this->channel_type + "_";
    double f_or_s;
    std::string unit;
    double diff_s = mstr::sample_rate_to_str(this->pt.sample_rate, f_or_s, unit, true);
    // can we make an int?
    if (((floor(f_or_s) - f_or_s) < treat_as_null) && (diff_s < treat_as_null))
      str += std::to_string(int(f_or_s)) + unit;
    else
      str += std::to_string(f_or_s) + unit;
    if (extension_with_dot.size())
      str += extension_with_dot;
    return str;
  }

  void ltrim(const size_t cut_first_n_samples) {
    if (cut_first_n_samples >= this->pt.samples) {
      std::ostringstream err_str((std::string("channel::") + __func__), std::ios_base::ate);
      err_str << "::can not cut more samples than available: " << cut_first_n_samples << " >= " << this->pt.samples;
      throw std::runtime_error(err_str.str());
    }
    this->pt.samples -= cut_first_n_samples;
    this->pt.spos = 0;
    this->pt.tt += static_cast<time_t>(cut_first_n_samples / this->pt.sample_rate);
    double rem = std::fmod(static_cast<double>(cut_first_n_samples), this->pt.sample_rate);
    this->pt.fracs += rem / this->pt.sample_rate;
    if (this->pt.fracs >= 1.0) {
      this->pt.tt += static_cast<time_t>(this->pt.fracs);
      this->pt.fracs = std::fmod(this->pt.fracs, 1.0);
    }
    // update the json file
    this->write_header();
    // cut the first n samples from the data file
    std::filesystem::path atss_file_in = this->get_atss_filepath();
    // check input file
    std::ifstream infile(atss_file_in, std::ios::binary);
    if (!std::filesystem::exists(atss_file_in)) {
      std::ostringstream err_str((std::string("channel::") + __func__), std::ios_base::ate);
      err_str << "::data file does not exist: " << atss_file_in;
      throw std::runtime_error(err_str.str());
    }
    // open input file
    if (!infile.is_open()) {
      std::ostringstream err_str((std::string("channel::") + __func__), std::ios_base::ate);
      err_str << "::can not open data file for reading: " << atss_file_in;
      throw std::runtime_error(err_str.str());
    }
    std::filesystem::path atss_file_out = atss_file_in;
    atss_file_out += ".tmp";
    std::ofstream outfile(atss_file_out, std::ios::binary | std::ios::trunc);
    // open output file
    if (!outfile.is_open()) {
      std::ostringstream err_str((std::string("channel::") + __func__), std::ios_base::ate);
      err_str << "::can not open temporarydata file for writing: " << atss_file_out;
      throw std::runtime_error(err_str.str());
    }
    // read and write in chunks
    size_t chunk_size = 8388608; // 8 MB

    // bytes to cut
    size_t bytes_to_cut = cut_first_n_samples * sizeof(double);
    infile.seekg(bytes_to_cut, std::ios::beg);
    std::vector<char> buffer(chunk_size);
    while (infile) {
      infile.read(buffer.data(), buffer.size());
      std::streamsize bytes_read = infile.gcount();
      if (bytes_read > 0) {
        outfile.write(buffer.data(), bytes_read);
      }
    }
    infile.close();
    outfile.close();
    // replace original file with the new file
    std::filesystem::remove(atss_file_in);
    std::filesystem::rename(atss_file_out, atss_file_in);
  }

  /*!
   * @brief trim cut samples from the end NO change of header needed
   * @param cut_last_n_samples
   */
  void rtrim(const size_t cut_last_n_samples) {
    if (!cut_last_n_samples)
      return;
    if (cut_last_n_samples >= this->pt.samples) {
      std::ostringstream err_str((std::string("channel::") + __func__), std::ios_base::ate);
      err_str << "::can not cut more samples than available: " << cut_last_n_samples << " >= " << this->pt.samples;
      throw std::runtime_error(err_str.str());
    }
    this->pt.samples -= cut_last_n_samples;
    // update the json file not needed

    // cut the last n samples from the data file
    std::filesystem::path atss_file_in = this->get_atss_filepath();
    size_t new_file_size = this->pt.samples * sizeof(double);
    try {
      std::filesystem::resize_file(atss_file_in, new_file_size);
    } catch (const std::filesystem::filesystem_error &e) {
      std::ostringstream err_str((std::string("channel::") + __func__), std::ios_base::ate);
      err_str << "::can not resize data file: " << atss_file_in << " error: " << e.what();
      throw std::runtime_error(err_str.str());
    }
  }

  /*!
   * @brief trim cut samples from begin and end and change header accordingly
   * @param cut_first_n_samples
   * @param cut_last_n_samples
   */
  void trim(const size_t cut_first_n_samples, const size_t cut_last_n_samples) {
    this->ltrim(cut_first_n_samples);
    this->rtrim(cut_last_n_samples);
  }

  std::filesystem::path filepath_wo_ext; //!< path without extension ... aka remember me
  // *********** maybe add parent node from survey tree later **************
  p_timer pt;                       //!< that is the timer class with fractions of seconds
  std::shared_ptr<calibration> cal; //!< calibration class

  // JSON values + datetime from p_timer
  double latitude = 0.0;    //!< decimal degree such as 52.2443, ISO 6709, +/- 90
  double longitude = 0.0;   //!< decimal degree such as 10.5594, ISO 6709, +/- 180
  double elevation = 0.0;   //!< elevation in meter
  double angle = 0.0;       //!< orientation from North to East (90 = East, -90 or 270 = West, 180 South, 0 North)
  double tilt = 0.0;        //!< angle positive down 90 = down, 0 = horizontal - in case it had been measured
  double resistance = 0.0;  //!< e.g. contact resistance of the electrodes
  std::string filter;       //!< comma separated string; system board name and filter like ADB-LF_LF-RF-1_LF-LP-4Hz which is the LF board with Radio Filter 1 and 4Hz low pass switched on
  std::string units = "mV"; //!< for ADUs it will be mV H or whatever or scaled E mV/km
  std::string source = "";  //!< empty or indicate as, ns, ca, cp, tx or what ever

  // not content of JSON -  these are the elements of the filename, generated on the fly
  std::size_t serial = 0;        //!< such as 1234 (no negative numbers please) for the system
  std::string system = "";       //!< such as ADU-08e, XYZ (a manufacturer is not needed because the system indicates it)
  std::size_t channel_no = 0;    //!< channel number - you can integrate EAMP stations as channels if the have the SAME!! timings
  std::string channel_type = ""; //!< type such as Ex, Ey, Hx, Hy, Hz or currents like Jx, Jy, Jz or Pitch, Roll, Yaw or x, y, z or T for temperature
  /*!
   * sample_rate contains sample_rate. Unit: Hz (samples per second) - "Hz" or "s" will be appended while writing in real time
   * the FILENAME contains a UNIT for better readability; you MUST have 256Hz (sample rate 256) OR 4s (sample rate 0.25);
   * a "." in the FILENAME is possible on modern systems, 16.6666Hz is possible
   */
  ADU board = ADU::off;           // from enum class ADU
  ADU radio_filter = ADU::off;    // from enum class ADU
  ADU low_pas_filter = ADU::off;  // from enum class ADU
  ADU high_pas_filter = ADU::off; // from enum class ADU
  ADU input_divider = ADU::off;   // from enum class ADU
  ADU gain_1 = ADU::off;          // from enum class ADU
  ADU gain_2 = ADU::off;          // from enum class ADU
  ADU mode = ADU::off;            // from enum class ADU, DIRECT mode

  std::string tmp_station;          // temporary station name, used for conversion
  double tmp_lsb = 1.0;             //!< temporary lsb value, used for conversion
  std::filesystem::path tmp_origin; //!< temporary origin path, used for conversion
  std::filesystem::path tmp_xml;    //!< temporary xml file, used for conversion

  std::vector<double> ts_slice;                       //!< data slice in time domain, that is read length; mostly the same a window length, except for zero padding
  std::vector<double> ts_slice_padded;                //!< data slice in time domain with zero padding, data from ts_slice WILL BE COPIED HERE
  std::vector<double> ts_chunk;                       //!< if wl = 1024 and overlapping is 50%, ts_chunk is 512; when filter 32x wl = 471 and ts_chunk = 32
  std::vector<std::complex<double>> spc_slice;        //!< data slice in spectral domain
  std::vector<double> ampl_slice;                     //!< single spectra amplitude slice, e.g. for plotting
  std::vector<double> ts_slice_inv;                   //!< data slice in time domain after ftt, calibration, inverse fft; always rl size, respectively wl ts_slice.
  std::vector<size_t> sample_vector;                  //!< sample vector for the x-axis
  size_t read_count = 0;                              //!< read count; increment this when you read a slice; reset to zero when you read a new file or change FFT
  std::queue<std::vector<std::complex<double>>> qspc; //!< spectra queue
  std::vector<std::vector<std::complex<double>>> spc; //!< spectra vector
  std::vector<std::complex<double>> caldata;          //!< calibration data complex for performing the calibration
  std::vector<double> caldata_f;                      //!< calibration data in frequency domain, tied with the calibration data;
  double bw = 0.0;                                    //!< bandwidth of FFT
  std::ifstream infile;                               //!< read binary data
  std::ofstream outfile;                              //!< write binary data
  fftw_plan plan;                                     //!< forward fftw plan (default)
  fftw_plan plan_inv;                                 //!< inverse fftw plan
  std::shared_ptr<fftw_freqs> fft_freqs;              //!< frequencies for FFT SHARE this pointer with other channels from the SAME RUN!
  bool is_remote = false;                             //!< set this to true if the data is remote or you want this data to be treated as remote
  bool is_emap = false;                               //!< set this to true if the data is from an EAMP station
  std::pair<int64_t, int64_t> read_pos{0, 0};         //!< sample position in the file, so 0 and 1024 for rl = 1024

  void init_fftw(std::shared_ptr<fftw_freqs> in_fft_freqs = nullptr, bool force_padded = false, const size_t &wl = 0, const size_t &rl = 0) {
    // in this case we have an fft created already - re use it
    if ((in_fft_freqs == nullptr) && (this->fft_freqs != nullptr) && (wl == 0) && (rl == 0)) {
      this->set_fftw_plan(force_padded);
      return;
    }

    if (in_fft_freqs != nullptr)
      this->fft_freqs = in_fft_freqs;
    else {
      if ((wl == 0) || (rl == 0))
        throw std::runtime_error(std::string(__func__) + " :: you must provide a valid window length and a valid resolution length");
      if (this->fft_freqs != nullptr)
        this->fft_freqs.reset();
      this->fft_freqs = std::make_shared<fftw_freqs>(this->pt.sample_rate, wl, rl);
    }
    this->set_fftw_plan(force_padded);
  }

  void init_inv_fftw() {
    if (this->fft_freqs == nullptr)
      throw std::runtime_error(std::string(__func__) + " :: you must provide a valid fftw_freqs inside the class object");
    this->set_inv_fftw_plan();
  }

  /*!
   * \brief set_lat_lon_elev according to ISO 6709, +/- 90, +/- 180, elevation in meter
   * \param lat
   * \param lon
   * \param elev
   */
  void set_lat_lon_elev(const double &lat, const double &lon, const double elev = 0.) {
    this->latitude = lat;
    if (lon > 180.0)
      this->longitude = lon - 360.0;
    else if (lon < -180.0)
      this->longitude = lon + 360.0;
    else
      this->longitude = lon;
    this->elevation = elev;
  }

  /*!
   * \brief write_header
   * \param jsn_cal the calibration part, in case you have a calibration object from outside
   */
  std::filesystem::path write_header(const std::shared_ptr<calibration> &jsn_cal = nullptr) {
    jsn head;
    head["datetime"] = this->start_datetime();
    head["latitude"] = this->latitude;
    head["longitude"] = this->longitude;
    head["elevation"] = this->elevation;
    head["angle"] = this->angle;
    head["tilt"] = this->tilt;
    head["resistance"] = this->resistance;
    head["units"] = this->units;
    head["filter"] = this->filter;
    head["source"] = this->source;
    if (jsn_cal != nullptr)
      head.update(jsn_cal->toJson_embedd());
    else if (this->cal != nullptr)
      head.update(this->cal->toJson_embedd());
    else {
      this->cal = std::make_shared<calibration>();
      head.update(this->cal->toJson_embedd());
    }

    std::filesystem::path filepath = this->filepath_wo_ext;
    if (std::filesystem::is_directory(filepath))
      filepath /= this->filename(".json");
    else
      filepath.replace_extension(".json");
    std::ofstream file;
    file.open(filepath, std::fstream::out | std::fstream::trunc);

    if (!file.is_open()) {
      file.close();
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << "::can not write header, file not open " << filepath;
      throw std::runtime_error(err_str.str());
    }

    file << std::setw(2) << head << std::endl;
    file.close();

    return filepath;
  }

  bool write_data(const std::vector<double> &data) {
    // write a slice in case
    if (this->outfile.is_open()) {
      for (auto dat : data) {
        this->outfile.write(static_cast<char *>(static_cast<void *>(&dat)), 8);
      }
      return true;
    } else {
      std::filesystem::path filepath = this->filepath_wo_ext;
      filepath.replace_extension(".atss");
      this->outfile.open(filepath, std::ios::out | std::ios::trunc | std::ios::binary);

      if (!this->outfile.is_open()) {
        this->outfile.close();
        std::ostringstream err_str(__func__, std::ios_base::ate);
        err_str << "::file not open " << filepath;
        throw std::runtime_error(err_str.str());
      }
      for (auto dat : data) {
        this->outfile.write(static_cast<char *>(static_cast<void *>(&dat)), 8);
      }
    }

    return true;
  }

  bool outfile_is_good() const {
    return this->outfile.good();
  }

  /*!
   * @brief close_outfile (will be called by destructor)
   */
  void close_outfile() {
    if (this->outfile.is_open())
      this->outfile.close();
  }

  bool write_all_data(const std::vector<double> &data) const {
    // write all data at once
    std::ofstream file;

    std::filesystem::path filepath = this->filepath_wo_ext;
    filepath.replace_extension(".atss");
    file.open(filepath, std::ios::out | std::ios::trunc | std::ios::binary);

    if (!file.is_open()) {
      file.close();
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << "::file not open " << filepath;
      throw std::runtime_error(err_str.str());
    }
    for (auto dat : data) {
      file.write(static_cast<char *>(static_cast<void *>(&dat)), 8);
    }

    file.close();

    return true;
  }

  bool write_slice() const {
    return this->write_all_data(this->ts_slice);
  }

  void to_ascii(const std::filesystem::path &outdir = "") {
    this->ts_slice.clear();
    this->ts_slice.resize(1024);
    this->ts_chunk.clear(); // no overlapping

    std::filesystem::path filepath = this->filepath_wo_ext;
    filepath.replace_extension(".dat");
    if (outdir != std::filesystem::path())
      filepath = outdir / filepath.filename();
    std::ofstream file_dat;
    file_dat.open(filepath, std::ios::out | std::ios::trunc);

    if (!file_dat.is_open()) {
      file_dat.close();
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << "::file not open " << filepath;
      throw std::runtime_error(err_str.str());
    }
    while (this->read_data(true) != -1) {
      for (auto dat : this->ts_slice) {
        file_dat << dat << std::endl;
      }
    }
    file_dat.close();
  }

  /*!
   * \brief open_atss_read
   * \return true in case of success, false else
   */
  bool open_atss_read() {
    bool ok = false;

    try {
      ok = std::filesystem::exists(this->get_atss_filepath());
    } catch (std::filesystem::filesystem_error &e) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << " " << e.what() << " can not open";
      throw std::runtime_error(err_str.str());
    }

    if (!ok)
      return false;
    this->infile.open(this->get_atss_filepath(), std::ios::in | std::ios::binary);
    if (!this->infile.is_open()) {
      this->infile.close();
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << "::can not open, file not open " << this->get_atss_filepath();
      throw std::runtime_error(err_str.str());
    }

    return true;
  }

  /*!
   * @brief close_atss_read (will be called by destructor)
   */
  void close_atss_read() {
    if (this->infile.is_open())
      this->infile.close();
    this->read_count = 0;
  }

  /*!
   * @brief skip_samples move the file pointer sizeof(double) * samples (positive or negative); file must be open
   * @param samples
   * @return
   */
  int64_t skip_samples(const int64_t &samples) {
    if (!samples)
      return this->infile.tellg();

    if (!this->infile.is_open()) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << "::file is NOT open! " << this->get_atss_filepath();
      throw std::runtime_error(err_str.str());
    }

    if (samples > this->samples()) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << "::samples to skip are more than total samples " << this->get_atss_filepath();
      throw std::runtime_error(err_str.str());
    }

    this->infile.seekg(samples * sizeof(double), this->infile.cur);
    return this->infile.tellg();
  }

  int64_t shift_to_read_time(const p_timer &new_tt) {
    if (!this->infile.is_open()) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << "::file is NOT open! " << this->get_atss_filepath();
      throw std::runtime_error(err_str.str());
    }

    if (new_tt < this->pt) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << "::new time is earlier than start time " << this->get_atss_filepath();
      throw std::runtime_error(err_str.str());
    }

    p_timer diff_time = (new_tt - this->pt);
    diff_time.sample_rate = this->pt.sample_rate; // we need our sample rate, not from the parent
    int64_t samples = diff_time.duration_to_samples(diff_time);
    std::cout << "diff time " << diff_time.tt << " " << diff_time.fracs << " , samples: " << samples << " " << mstr::sample_rate_to_str_simple(this->pt.sample_rate);
    this->infile.seekg(samples * sizeof(double), this->infile.cur);
    return this->infile.tellg();
  }

  /*!
   * \brief read_data
   * \param read_last_chunk
   * \return file position or -1 in case of fail or unexpected end (-1 should NOT happen when read for fft)
   */

  int64_t read_data(const bool read_last_chunk = false, const std::shared_ptr<atmm> &sel = nullptr) {
    if (this->infile.is_open()) {
      return this->read_bin(this->ts_slice, this->infile, read_last_chunk);
    }

    else {

      if (this->open_atss_read()) {
        return this->read_bin(this->ts_slice, this->infile, read_last_chunk);
      } else
        return -1;
    }

    return -1;
  }

  std::vector<double> read_single(const size_t &read_samples, const size_t &skip_samples_read, const bool bdetrend) {
    if (!read_samples) {
      throw std::runtime_error(std::string(__func__) + " :: read_samples is zero");
    }
    this->open_atss_read();
    if (this->samples() < (read_samples + skip_samples_read)) {
      throw std::runtime_error(std::string(__func__) + " :: read_samples + skip_samples_read is larger than total samples");
    }
    if (skip_samples_read)
      this->skip_samples(skip_samples_read);
    this->ts_slice.resize(read_samples);
    int64_t control = 0;
    control = this->read_bin(this->ts_slice, this->infile, false);
    this->infile.close();
    if (control < 0) {
      throw std::runtime_error(std::string(__func__) + " :: read error");
    }
    if (bdetrend)
      detrend<double>(this->ts_slice.begin(), this->ts_slice.end());
    return this->ts_slice;
  }

  std::pair<int64_t, int64_t> read_plotter(const std::shared_ptr<stats> &status, const size_t &skip_samples_read) {
    if (!status->rl) {
      throw std::runtime_error(std::string(__func__) + " :: read_samples is zero");
    }
    if (this->samples() < (status->rl + skip_samples_read)) {
      std::string samples = std::to_string(this->samples());
      throw std::runtime_error(std::string(__func__) + " :: read_samples + skip_samples_read is larger than total samples: " + samples);
    }
    if (skip_samples_read)
      this->skip_samples(skip_samples_read);
    this->ts_slice.resize(status->rl);
    int64_t control = 0;
    control = this->read_bin(this->ts_slice, this->infile, false);
    // this->infile.close();
    if (control < 0) {
      throw std::runtime_error(std::string(__func__) + " :: read error");
    }
    this->plotter_fft_and_inverse(status); // do this before detrend
    if (status->detrend)
      detrend<double>(this->ts_slice.begin(), this->ts_slice.end());
    for (size_t i = skip_samples_read; i < this->ts_slice.size(); ++i) {
      this->sample_vector.push_back(i); // default x-axis with sample nos
    }
    return this->read_pos; // first and second sample are updated by read_bin
  }

  void plotter_fft_and_inverse(const std::shared_ptr<stats> &status) {
    if (read_count == 1) {
      this->ampl_slice.resize(this->spc_slice.size()); // always needed; FFTW result is always in spc_slice whether padded or not
      if (this->cal == nullptr) {                      // do we have a calibration?
        // throw error runtime
        std::ostringstream err_str(__func__, std::ios_base::ate);
        err_str << "::calibration nullptr; normally this should not happen, f can be empty,but not nullptr";
        throw std::runtime_error(err_str.str());
      }

      if (!this->cal->is_empty()) { // we have a calibration with data
        if (this->cal->f.size() == this->fft_freqs->index_range().second) {
          this->cal->get_cplx_cal(this->caldata_f, this->caldata);
        } else { // we have but it does not fit; should be same size at this point
          // throw error runtime
          std::ostringstream err_str(__func__, std::ios_base::ate);
          err_str << "::calibration data size does not fit the fft size";
          throw std::runtime_error(err_str.str());
        }
      }
    }

    // copy the ts_slice; we need to keep the original for the inverse fft
    // copy the ts_slice to ts_slice_padded; padded is initialized in set_fftw_plan with forced_padded
    std::copy(this->ts_slice.begin(), this->ts_slice.end(), this->ts_slice_padded.begin());
    detrend<double>(this->ts_slice_padded.begin(), this->ts_slice_padded.begin() + this->ts_slice.size());
    // fill the rest with zeros
    // std::fill(this->ts_slice_padded.begin() + this->ts_slice.size(), this->ts_slice_padded.end(), 0.0);
    // this->ts_slice_inv.resize(this->ts_slice_padded.size()); // inverse fft, make sure that it fits
    fftw_execute(this->plan);

    // raw spectra
    if (!status->cal_on) {
      for (size_t i = 0; i < this->spc_slice.size(); ++i) {
        this->ampl_slice[i] = std::abs(this->spc_slice[i]);
      }
      return;
    }
    if (this->cal->is_empty())
      return;

    // skip the DC part; almost all calibration may have 0 for DC
    // and detrended data may not have a DC part as well
    for (size_t i = 1, j = 0; i < this->spc_slice.size(); ++i, ++j) {
      this->spc_slice[i] /= this->caldata[i];
    }
    for (size_t i = 0; i < this->spc_slice.size(); ++i) {
      this->ampl_slice[i] = std::abs(this->spc_slice[i]);
    }

    fftw_execute(this->plan_inv);
    for (auto &val : this->ts_slice_inv) {
      val /= status->wl;
      if (fabs(val) < 1E-12)
        val = 0.0;
    }
  }

  int64_t goto_sample_pos(const int64_t &sample_pos) {
    if (this->infile.is_open()) {
      this->infile.seekg(sample_pos * sizeof(double), this->infile.beg);
      return this->infile.tellg();
    } else
      return -1;
  }

  std::vector<double> read_all_at_once() {
    this->open_atss_read();
    this->ts_slice.resize(this->samples());
    int64_t control = 0;
    control = this->read_bin(this->ts_slice, this->infile, false);
    this->infile.close();
    if (control < 0) {
      throw std::runtime_error(std::string(__func__) + " :: read error");
    }
    return this->ts_slice;
  }

  /*!
   * \brief read_all_fftw that includes the COMPLETE spectra inclusive DC part and Nyquist; executes this->plan and stores INSIDE channel
   * \param read_last_chunk false for MT processing - last chunk has != read length (fft) size
   * \param sel atmm selection vector 0 = not excluded, 1 excluded from reading / processing
   */
  void read_all_fftw(const bool read_last_chunk = false, const std::shared_ptr<atmm> &sel = nullptr) {
    int64_t reads = 0;
    // clear the queue
    while (!this->qspc.empty())
      this->qspc.pop();
    do {

      reads = this->read_data(read_last_chunk, sel);
      if (reads > 0) {
        detrend_and_hanning<double>(this->ts_slice.begin(), this->ts_slice.end());
        if (this->ts_slice_padded.size()) {
          // this->ts_slice_padded.insert(this->ts_slice_padded.begin(), this->ts_slice.cbegin(), this->ts_slice.cend());
          for (size_t i = 0; i < ts_slice.size(); ++i) {
            this->ts_slice_padded[i] = ts_slice[i]; // copy first part; rest is zero
          }
        }
        fftw_execute(this->plan);
        this->qspc.push(this->spc_slice);
      }

    } while (reads > 0);
    if (this->infile.is_open())
      this->infile.close();
  }

  void read_all_fftw_gaussian_noise(const std::vector<double> double_noise, const bool bdetrend_hanning = true) {
    if (!this->ts_slice.size())
      return;
    if (double_noise.size() < this->ts_slice.size())
      return;

    auto itb = double_noise.cbegin();
    auto ite = double_noise.cbegin();
    std::advance(ite, this->ts_slice.size());

    for (;;) {
      this->ts_slice.assign(itb, ite);
      if (bdetrend_hanning)
        detrend_and_hanning<double>(this->ts_slice.begin(), this->ts_slice.end());
      if (this->ts_slice_padded.size()) {
        for (size_t i = 0; i < ts_slice.size(); ++i) {
          this->ts_slice_padded[i] = ts_slice[i];
        }
      }
      fftw_execute(this->plan);
      this->qspc.push(this->spc_slice);
      if (std::distance(ite, double_noise.cend()) > (int64_t)(this->ts_slice.size() - 1)) {
        std::advance(itb, this->ts_slice.size());
        std::advance(ite, this->ts_slice.size());
      } else
        break;
    }
  }

  /*!
   * \brief ats_simple_stack_all
   * \return non calibrated stacked complex spectra
   */
  std::vector<std::complex<double>> ats_simple_stack_all(const std::shared_ptr<fftw_freqs> &fft_freqs, const bool bwincal = true) {
    std::vector<std::complex<double>> sa;
    sa.resize(this->qspc.front().size());
    double dn = 0;
    while (!this->qspc.empty()) {
      std::vector<std::complex<double>> &v = this->qspc.front();
      size_t i = 0;
      for (auto &val : v) {
        sa[i++] += val;
      }
      this->qspc.pop();
      dn++;
    }
    for (auto &val : sa)
      val /= dn;
    if (bwincal)
      fft_freqs->scale(sa);
    return sa;
  }

  /*!
   * \brief prepare_raw_spc converts the local queue to a vector spc AND scales with the window wincal
   * \param fft_freqs where the fft settings are stored; YOU MUST interpolate / extend the calibration to the same length as the FFT in ADVANCE
   * \param bcal
   */
  void prepare_raw_spc(const bool bcal = true, const bool syscal = true, const bool bwincal = true) {
    this->spc.reserve(this->qspc.size());
    size_t j = 0;
    bool bcaldata = bcal;
    std::vector<std::complex<double>> syscal_data;
    if (this->cal->f.size() == 0) {
      bcaldata = false;
      std::cerr << "prepare_raw_spc no calibration data available for " << this->cal->sensor << std::endl;
    }
    while (!this->qspc.empty()) {
      this->spc.emplace_back(this->fft_freqs->trim_fftw_result(this->qspc.front()));
      // fetch interpolated calibration data first time
      if (bcaldata && bcal && !j) {
        if (this->cal == nullptr)
          throw std::runtime_error(std::string(__func__) + " :: no calibration available");
        if (this->cal->f.size() != this->spc.back().size()) {
          std::ostringstream err_str(__func__, std::ios_base::ate);
          err_str << " :: calibration size is smaller than FFT size " << this->cal->f.size() << " " << this->spc.back().size();
          throw std::runtime_error(err_str.str());
        }
        this->cal->get_cplx_cal(this->caldata_f, this->caldata);
      }
      if (syscal && !j) {
        // get the board from cal_synthetic.h
        if (this->system == "ADU-08e" && this->board == ADU::LF) {
          syscal_data = gen_trf_adb_08e_lf(this->fft_freqs->get_frequencies(), this->radio_filter, this->low_pas_filter, this->input_divider, this->resistance, this->mode, this->gain_1);
        } else if (this->system == "ADU-08e" && this->board == ADU::HF) {
          syscal_data = gen_trf_adb_08e_hf(this->fft_freqs->get_frequencies(), this->high_pas_filter, this->gain_1);
        }
        // check the sizes with existing spectra
        if (syscal_data.size() != this->spc.back().size()) {
          std::ostringstream err_str(__func__, std::ios_base::ate);
          err_str << " :: synthetic calibration size is smaller than FFT size " << syscal_data.size() << " " << this->spc.back().size();
          throw std::runtime_error(err_str.str());
        }
      }

      if ((syscal_data.size() == this->spc.back().size())) {
        std::transform(this->spc.back().begin(), this->spc.back().end(), syscal_data.begin(), this->spc.back().begin(), std::divides<std::complex<double>>());
      }

      if (bcaldata) {
        // divide this->spc.back() by caldata
        std::transform(this->spc.back().begin(), this->spc.back().end(), this->caldata.begin(), this->spc.back().begin(), std::divides<std::complex<double>>());
      }
      if (bwincal)
        this->fft_freqs->scale(this->spc.back());
      this->qspc.pop();
      ++j;
    }

    this->fft_freqs->set_raw_stacks(j);

    return;
  }
  void prepare_to_raw_spc(const std::shared_ptr<fftw_freqs> &in_fft_freqs, const bool bcal = true, const bool bwincal = true) {
    this->spc.reserve(this->qspc.size());
    size_t j = 0;
    while (!this->qspc.empty()) {
      this->spc.emplace_back(in_fft_freqs->trim_fftw_result(this->qspc.front()));
      if (bcal && !j) {
        // create cal
      }
      if (bcal) {
        // cal
      }
      if (bwincal)
        in_fft_freqs->scale(this->spc.back());
      this->qspc.pop();
      ++j;
    }

    in_fft_freqs->set_raw_stacks(j);

    return;
  }

  size_t samples(const std::filesystem::path &filepath_wo_ext = "") {
    if (this->filepath_wo_ext.empty() && filepath_wo_ext.empty()) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << "::empty class file AND no argument given ";
      throw std::runtime_error(err_str.str());
    }
    std::filesystem::path filepath;
    if (!filepath_wo_ext.empty())
      filepath = filepath_wo_ext;
    else
      filepath = this->filepath_wo_ext;
    filepath.replace_extension(".atss");

    uintmax_t xx = 0;

    try {
      xx = std::filesystem::file_size(filepath);
    } catch (std::filesystem::filesystem_error &e) {
      std::cerr << e.what() << std::endl;
      return 0;
    }

    this->pt.samples = size_t(xx / sizeof(double));

    return this->pt.samples;
  }

  std::string brief() const {
    std::stringstream ss;
    ss << this->pt.brief();
    if (this->cal != nullptr)
      ss << " " << cal->brief();

    return ss.str();
  }
  bool operator==(const channel &rhs) const {
    if (pt != rhs.pt) {
      return false;
    }

    double eps = 1E-10; // tight tolerance for sampling rate
    if (std::fabs(get_sample_rate() - rhs.get_sample_rate()) > eps) {
      return false;
    }

    eps = 1E-6; // relaxed tolerance for position-related fields
    if (std::fabs(latitude - rhs.latitude) > eps) {
      return false;
    }
    if (std::fabs(longitude - rhs.longitude) > eps) {
      return false;
    }
    if (std::fabs(elevation - rhs.elevation) > eps) {
      return false;
    }
    if (std::fabs(angle - rhs.angle) > eps) {
      return false;
    }
    if (std::fabs(tilt - rhs.tilt) > eps) {
      return false;
    }
    if (std::fabs(resistance - rhs.resistance) > eps) {
      return false;
    }

    if (filter != rhs.filter) {
      return false;
    }
    if (units != rhs.units) {
      return false;
    }
    if (source != rhs.source) {
      return false;
    }
    if (serial != rhs.serial) {
      return false;
    }
    if (system != rhs.system) {
      return false;
    }
    if (channel_no != rhs.channel_no) {
      return false;
    }
    if (channel_type != rhs.channel_type) {
      return false;
    }

    return true;
  }

  bool operator!=(const channel &rhs) const {
    return !(*this == rhs);
  }

private:
  /*!
   * \brief read_bin binary core routine
   * \param data data slice to read
   * \param file ifstream
   * \param read_last_chunk - false in case of fft, true incase you want to read all and the last data vector is smaller than the previous ones
   * \return -1 in case of failure, else the sample position
   */
  int64_t read_bin(std::vector<double> &data, std::ifstream &file, const bool read_last_chunk = false) {
    // too much checking ?
    if (file.peek() == EOF) {
      data.resize(0);
      this->infile.close();
      return -1;
    }

    this->read_pos.first = file.tellg() / sizeof(double);
    size_t i = 0;

    if (!this->ts_chunk.size()) {

      while (!file.eof() && i < data.size()) {
        file.read(static_cast<char *>(static_cast<void *>(&data[i++])), 8);
      }
      this->read_count++;

      // read last chunk in case
      if ((file.eof()) && read_last_chunk && (i > 1)) {
        --i;
        data.resize(i); // keeps the elements; i was incremented BEFORE the while loop above terminated
      } else if ((i == 1) || !i) {
        data.resize(0);
        this->infile.close();
        return -1;
      }
    }
    // we read chunks, so ts_data can be 471 and ts_chunk 32
    else {
      while (!file.eof() && i < this->ts_chunk.size()) {
        file.read(static_cast<char *>(static_cast<void *>(&this->ts_chunk[i++])), 8);
      }
      this->read_count++;

      // read last chunk in case
      if ((file.eof()) && read_last_chunk && (i > 1)) {
        --i;
        this->ts_chunk.resize(i);  // keeps the elements; i was incremented BEFORE the while loop above terminated
      } else if ((i == 1) || !i) { // here we can take the original data
        data.resize(0);
        this->infile.close();
        return -1;
      }

      // ******************************** reading chunks means: you already have read one slice at the beginning! so data IS EXISTING
      if (this->ts_chunk.size() >= data.size())
        data = this->ts_chunk;
      else {
        size_t j = 0;
        for (i = ts_chunk.size(); i < data.size(); ++i) {
          data[j++] = data[i]; // that moves the data to the beginning
        }
        for (i = 0; i < this->ts_chunk.size(); ++i) {
          data[j++] = this->ts_chunk[i]; // and now we append the new data
        }
      }
    }
    this->read_pos.second = file.tellg() / sizeof(double);
    return this->read_pos.second;
  }

  /*!
   * @brief set_fftw_plan
   * @param force_padded - this is for tsplotter, in order to have an independent copy of the ts_slice
   */
  void set_fftw_plan(bool force_padded = false) {
    if (this->fft_freqs == nullptr)
      throw std::runtime_error(std::string(__func__) + " :: no fft frequencies available");
    if (this->fft_freqs->get_rl() == 0)
      throw std::runtime_error(std::string(__func__) + " :: no read length available");
    if (this->fft_freqs->get_wl() == 0)
      throw std::runtime_error(std::string(__func__) + " :: no window length available");
    this->ts_slice.resize(this->fft_freqs->get_rl());  // need this always for reading rl of data
    this->spc_slice.resize(this->fft_freqs->get_fl()); //!< get frequency lines, e.g. wl/2 +1 : rl 1024 -> 513
    if ((this->fft_freqs->get_rl() == this->fft_freqs->get_wl() && !force_padded)) {
      this->plan = fftw_plan_dft_r2c_1d(this->fft_freqs->get_wl(), &this->ts_slice[0], reinterpret_cast<fftw_complex *>(&this->spc_slice[0]), FFTW_ESTIMATE);
    }
    if ((this->fft_freqs->get_wl() > this->fft_freqs->get_rl() || force_padded)) {
      this->ts_slice_padded.resize(this->fft_freqs->get_wl(), 0.0); // wl is always equal or larger than rl
      // after reading rl, the slice will be filled up to wl with zeros
      this->plan = fftw_plan_dft_r2c_1d(this->fft_freqs->get_wl(), &this->ts_slice_padded[0], reinterpret_cast<fftw_complex *>(&this->spc_slice[0]), FFTW_ESTIMATE);
    }

    this->bw = this->fft_freqs->get_bw();
  }

  void set_inv_fftw_plan() {
    if (this->fft_freqs == nullptr)
      throw std::runtime_error(std::string(__func__) + " :: no fft frequencies available");
    if (this->fft_freqs->get_rl() == 0)
      throw std::runtime_error(std::string(__func__) + " :: no read length available");
    if (this->fft_freqs->get_wl() == 0)
      throw std::runtime_error(std::string(__func__) + " :: no window length available");
    this->ts_slice_inv.resize(this->fft_freqs->get_wl(), 0.0);
    this->plan_inv = fftw_plan_dft_c2r_1d(this->fft_freqs->get_wl(), reinterpret_cast<fftw_complex *>(&this->spc_slice[0]), &this->ts_slice_inv[0], FFTW_ESTIMATE);
  }

}; // end channel class

// helper to enforce non-null channel pointers in comparators
static inline const std::shared_ptr<channel> &
ensure_channel_ptr(const std::shared_ptr<channel> &ptr, const char *ctx) {
  if (!ptr) {
    throw std::runtime_error(std::string(ctx) + ": null channel pointer");
  }
  return ptr;
}

/*!
 * @brief lexicographical sort by channel filename, Ex is before Ey, Hx before Hy before Hz and so on
 */
static auto compare_channel_name_lt = [](const std::shared_ptr<channel> &lhs, const std::shared_ptr<channel> &rhs) -> bool {
  ensure_channel_ptr(lhs, "compare_channel_name_lt");
  ensure_channel_ptr(rhs, "compare_channel_name_lt");
  return lhs->filename() < rhs->filename();
};

/*!
 * @brief returns true if lhs start time is less than rhs start time; using std::sort you get the earliest first
 */
static auto compare_channel_start_lt = [](const std::shared_ptr<channel> &lhs, const std::shared_ptr<channel> &rhs) -> bool {
  ensure_channel_ptr(lhs, "compare_channel_start_lt");
  ensure_channel_ptr(rhs, "compare_channel_start_lt");
  return lhs->pt < rhs->pt;
};

/*!
 * @brief returns true if lhs start time is equal to rhs start time
 */
static auto compare_channel_start_eq = [](const std::shared_ptr<channel> &lhs, const std::shared_ptr<channel> &rhs) -> bool {
  ensure_channel_ptr(lhs, "compare_channel_start_eq");
  ensure_channel_ptr(rhs, "compare_channel_start_eq");
  return lhs->pt == rhs->pt;
};

/*!
 * @brief returns true if lhs sampling rate is equal to rhs sampling rate
 */
static auto compare_channel_sampling_rate_eq = [](const std::shared_ptr<channel> &lhs, const std::shared_ptr<channel> &rhs) -> bool {
  ensure_channel_ptr(lhs, "compare_channel_sampling_rate_eq");
  ensure_channel_ptr(rhs, "compare_channel_sampling_rate_eq");
  return (lhs->get_sample_rate() == rhs->get_sample_rate());
};

/*!
 * @brief returns true if lhs run directory is equal to rhs run directory, that is the parent dir ../run_xxxx/
 */
static auto compare_channel_run_eq = [](const std::shared_ptr<channel> &lhs, const std::shared_ptr<channel> &rhs) -> bool {
  ensure_channel_ptr(lhs, "compare_channel_run_eq");
  ensure_channel_ptr(rhs, "compare_channel_run_eq");
  return (lhs->get_run_dir() == rhs->get_run_dir());
};

/*!
 * @brief returns true if lhs station directory is equal to rhs station directory. that is the parent, parent directory ../run_xxxx/station_yyyy/
 */
static auto compare_channel_station_eq = [](const std::shared_ptr<channel> &lhs, const std::shared_ptr<channel> &rhs) -> bool {
  ensure_channel_ptr(lhs, "compare_channel_station_eq");
  ensure_channel_ptr(rhs, "compare_channel_station_eq");
  return (lhs->get_station_dir() == rhs->get_station_dir());
};

/*!
 * @brief returns true if lhs station directory and run directory are equal to rhs station directory and run directory. e.g. we are in the same station and same run
 */
static auto compare_channel_station_run_eq = [](const std::shared_ptr<channel> &lhs, const std::shared_ptr<channel> &rhs) -> bool {
  ensure_channel_ptr(lhs, "compare_channel_station_run_eq");
  ensure_channel_ptr(rhs, "compare_channel_station_run_eq");
  return ((lhs->get_station_dir() == rhs->get_station_dir()) && (lhs->get_run_dir() == rhs->get_run_dir()));
};

/*!
 * @brief returns true if lhs start time and sample rate are equal to rhs start time and sample rate, but channel numbers are different
 */
static auto same_run = [](const std::shared_ptr<channel> &lhs, const std::shared_ptr<channel> &rhs) -> bool {
  ensure_channel_ptr(lhs, "same_run");
  ensure_channel_ptr(rhs, "same_run");
  return ((lhs->pt == rhs->pt) && (lhs->get_sample_rate() == rhs->get_sample_rate() && (lhs->get_channel_no() != rhs->get_channel_no())));
};

/*!
 * @brief returns true if lhs start time and sample rate are equal to rhs start time and sample rate, channel types and stop times are also equal
 */
static auto is_parallel_channel = [](const std::shared_ptr<channel> &lhs, const std::shared_ptr<channel> &rhs) -> bool {
  ensure_channel_ptr(lhs, "is_parallel_channel");
  ensure_channel_ptr(rhs, "is_parallel_channel");
  return ((lhs->pt == rhs->pt) && (lhs->get_sample_rate() == rhs->get_sample_rate() && (lhs->get_channel_type() == rhs->get_channel_type())));
};

/*!
 * @brief repair_run_files
 * @param channels vector of channels pointers to repair
 * @details throws an error if start time is not the same (serious problem!), perform a rtrim to the shortest length and make sure that all channels have the same number of samples
 */
static size_t repair_run_files(std::vector<std::shared_ptr<channel>> &channels) {
  if (channels.size() < 2)
    return 0; // nothing to do
  // first check start times
  auto first_start = channels[0]->pt;
  for (size_t i = 1; i < channels.size(); ++i) {
    if (channels[i]->pt != first_start) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << "::different start times found between channels " << channels[0]->filename() << " and " << channels[i]->filename();
      throw std::runtime_error(err_str.str());
    }
  }
  // now find the minimum samples
  size_t min_samples = channels[0]->samples();
  for (size_t i = 1; i < channels.size(); ++i) {
    if (channels[i]->samples() < min_samples)
      min_samples = channels[i]->samples();
  }
  // now rtrim all to min_samples
  for (size_t i = 0; i < channels.size(); ++i) {
    size_t samples_to_trim = channels[i]->samples() - min_samples;
    if (samples_to_trim > 0) {
      channels[i]->rtrim(samples_to_trim);
    }
  }
  return min_samples;
}

static std::string make_time_string_now() {
  auto now = std::chrono::system_clock::now();
  auto now_c = std::chrono::system_clock::to_time_t(now);
  // Convert to tm struct for formatting
  std::tm now_tm = *std::localtime(&now_c);
  std::ostringstream oss;
  oss << std::put_time(&now_tm, "%Y-%m-%dT%H:%M:%S");
  return oss.str();
}

/*!
 * @brief make default channel with EFP-6 and MFS-06e; the calibration data is still empty!
 * @param chan an existing channel
 * @param sample_rate
 * @param channel_type Ex, Ey, Hx, Hy, Hz, T,
 * @param system default is ADU-08e
 */
static void make_channel(std::shared_ptr<channel> &chan, const double &sample_rate, const std::string &channel_type, const std::string &system = "ADU-08e") {
  if (chan == nullptr)
    throw std::runtime_error(std::string(__func__) + " :: channel is nullptr");
  // start with base values
  auto dt_string = make_time_string_now();
  chan->set_datetime(dt_string);
  //
  chan->set_sample_rate(sample_rate);
  if (system.size())
    chan->set_system(system);
  else
    chan->set_system("ADU-08e");
  chan->set_serial(1);
  chan->channel_type = channel_type;
  // now we continue with defaults
  // ... from famous Northern Mining Example
  chan->latitude = 39.02;
  chan->longitude = 29.124;
  chan->elevation = 1088.31;
  // create some channel data
  if (chan->cal == nullptr)
    chan->cal = std::make_shared<calibration>();
  if (channel_type == "Ex") {
    chan->angle = 0.0;
    chan->tilt = 0.0;
    chan->units = "mV/km";
    chan->set_channel_no(0);
    chan->cal->sensor = "EFP-06";
    chan->cal->serial = 1;
    chan->cal->units_amplitude = "mV";
    chan->cal->units_phase = "degrees";
    chan->cal->units_frequency = "Hz";
    chan->cal->datetime = dt_string;
  } else if (channel_type == "Ey") {
    chan->angle = 90.0;
    chan->tilt = 0.0;
    chan->units = "mV/km";
    chan->set_channel_no(1);
    chan->cal->sensor = "EFP-06";
    chan->cal->serial = 2;
    chan->cal->units_amplitude = "mV";
    chan->cal->units_phase = "degrees";
    chan->cal->units_frequency = "Hz";
    chan->cal->datetime = dt_string;
  } else if (channel_type == "Hx") {
    chan->angle = 0.0;
    chan->tilt = 0.0;
    chan->units = "mV";
    chan->set_channel_no(2);
    chan->cal->sensor = "MFS-06e";
    chan->cal->serial = 1;
    if (chan->get_sample_rate() < 512.1)
      chan->cal->chopper = ChopperStatus::on;
    else
      chan->cal->chopper = ChopperStatus::off;
    chan->cal->units_amplitude = "mV/nT";
    chan->cal->units_phase = "degrees";
    chan->cal->units_frequency = "Hz";
    chan->cal->datetime = dt_string;
  } else if (channel_type == "Hy") {
    chan->angle = 90.0;
    chan->tilt = 0.0;
    chan->units = "mV";
    chan->set_channel_no(3);
    chan->cal->sensor = "MFS-06e";
    chan->cal->serial = 1;
    if (chan->get_sample_rate() < 512.1)
      chan->cal->chopper = ChopperStatus::on;
    else
      chan->cal->chopper = ChopperStatus::off;
    chan->cal->units_amplitude = "mV/nT";
    chan->cal->units_phase = "degrees";
    chan->cal->units_frequency = "Hz";
    chan->cal->datetime = dt_string;
  } else if (channel_type == "Hz") {
    chan->angle = 0.0;
    chan->tilt = 90.0;
    chan->units = "mV";
    chan->set_channel_no(4);
    chan->cal->sensor = "MFS-06e";
    chan->cal->serial = 1;
    if (chan->get_sample_rate() < 512.1)
      chan->cal->chopper = ChopperStatus::on;
    else
      chan->cal->chopper = ChopperStatus::off;
    chan->cal->units_amplitude = "mV/nT";
    chan->cal->units_phase = "degrees";
    chan->cal->units_frequency = "Hz";
    chan->cal->datetime = dt_string;
  } else if (channel_type == "T") {
    chan->angle = 0.0;
    chan->tilt = 0.0;
    chan->units = "°C";
    chan->set_channel_no(20);
  } else {
    chan->angle = 0.0;
    chan->tilt = 0.0;
    chan->units = "mV";
    chan->set_channel_no(21);
  }
  chan->resistance = 1000;
}

// helper to enforce non-null channel pointers in comparators
#endif // CHANNEL_HPP