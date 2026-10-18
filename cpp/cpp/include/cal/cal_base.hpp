#ifndef CAL_BASE_H
#define CAL_BASE_H

#include <climits>
#include <cmath>
#include <complex>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <list>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

/// @file cal_base.hpp

//
// that is now in cmake as add_compile_definitions( _USE_MATH_DEFINES _msvc )
// #define _USE_MATH_DEFINES // for C++ and MSVC
//
#include "cal_synthetic.hpp"
#include "freqs.hpp"
#include "json.hpp"
#include "mt_base.hpp"
#include "strings_etc.hpp"
#include "tinyxmlwriter.h"
#include "vector_math.hpp"

/// @class calibration
/// @brief Represents a calibration object for a sensor. The class forces units in mV/nT, Hz, and degrees - not normalized by f!
/// temporarily the class may contain different units after reading from file, but the class will convert a.s.a.p. to mV/nT, Hz, and degrees
/// @details spectra will be divided by caldata, so: spectra_calibrated = spectra /= caldata
class calibration {
public:
  std::string sensor;                         ///< The name of the sensor.
  uint64_t serial = 0;                        ///< The serial number of the sensor.
  ChopperStatus chopper = ChopperStatus::off; ///< The status of the chopper. off is same as unknown
  std::string units_amplitude;                ///< The units of the amplitude.
  std::string units_frequency;                ///< The units of the frequency.
  std::string units_phase;                    ///< The units of the phase.
  std::string datetime;                       ///< The date and time of the calibration.
  std::string Operator;                       ///< The operator who made the calibration, we us uppercase because operator is a keyword in C++.
  std::vector<double> f;                      ///< The frequency values in Hz. That is the driver of this class. ASCENDING ORDER IS REQUIRED.
  std::vector<double> a;                      ///< The amplitude values in mV/nT. That is the driver of this class.
  std::vector<double> p;                      ///< The phase values ind degrees. That is the driver of this class.
  std::string board;                          ///< The name of the ADU board
  std::string system;                         ///< The name of the ADU system.
  double R = 0.0;                             ///< The resistance value of the ADU board.
  double C = 0.0;                             ///< The capacitance value of the ADU board.
  double sampling_rate = 0.0;                 ///< The sampling rate of the ADU board.
  std::filesystem::path file_path_in;         ///< The file path of the calibration data. saved after reading from file.
  std::filesystem::path file_path_out;        ///< The file path where the calibration data will be saved.
  // NOT part of the calibration data in JSON files
  // in JSON ist must be ct = CalibrationType::mtx to allow MTH5
  CalibrationType ct = CalibrationType::nn; ///< The type of calibration.
  // avoid unnecessary multiplication/division by small numbers
  double phase_zero = 0.01;              ///< The phase assumed to be zero in degrees.
  double amplitude_one_deviation = 0.01; ///< The amplitude assumed to be one if it deviates by this amount from 1.
  double DIV = 1.0;                      ///< The division factor for for input; can be DIV_1 (1) or DIV_8 (8).

  /// @brief Default constructor. Creates an empty calibration object.
  calibration() {
    this->clear();
  }

  /// @brief Constructor. Creates a calibration object with the specified sensor, serial number, chopper status, and calibration type.
  /// @param sensor The name of the sensor.
  /// @param serial The serial number of the sensor.
  /// @param chopper The status of the chopper.
  /// @param ct The type of calibration.
  calibration(const std::string &sensor, const uint64_t &serial, const ChopperStatus chopper = ChopperStatus::off, const CalibrationType ct = CalibrationType::mtx) :
      sensor(mstr::trim(sensor)), serial(serial) {
    this->set_format(ct, false);
    this->chopper = chopper;
  }

  /// @brief Rounds the calibration values based on phase_zero and amplitude_one_deviation.
  /// @details If a phase value is smaller than phase_zero, it is set to zero. AVOIDS scaling errors in PLOTS.
  ///          If an amplitude value deviates from one by less than amplitude_one_deviation, it is set to one.
  void calibration_rounding() {
    for (size_t i = 0; i < this->p.size(); ++i) {
      if (std::abs(this->p[i]) < phase_zero) {
        this->p[i] = 0.0;
      }
    }
    for (size_t i = 0; i < this->a.size(); ++i) {
      if (std::abs(this->a[i] - 1.0) < amplitude_one_deviation) {
        this->a[i] = 1.0;
      }
    }
  }

  void set_div(const int &div) {
    if (div != 1 && div != 8) {
      throw std::runtime_error("Invalid division factor. Must be 1 or 8.");
    }
    this->DIV = double(div);
  }

  /// @brief Multiplies amplitudes and adds phases in degrees, without phase wrapping.
  /// @throws std::runtime_error if frequency vectors differ or amplitude/phase vectors are empty or have inconsistent sizes.
  /// @details Only a and p are modified; frequencies, metadata, and backup/theoretical/master data are unchanged.
  calibration &operator*=(const calibration &rhs) {
    this->validate_arithmetic(rhs, "operator*=");
    for (size_t i = 0; i < this->f.size(); ++i) {
      this->a[i] *= rhs.a[i];
      this->p[i] += rhs.p[i];
    }
    return *this;
  }

  /// @brief Divides amplitudes and subtracts phases in degrees, without phase wrapping.
  /// @throws std::runtime_error if frequency vectors differ, amplitude/phase vectors are empty or have inconsistent sizes, or a divisor is zero.
  /// @details Only a and p are modified; validation completes before modifying either vector.
  calibration &operator/=(const calibration &rhs) {
    this->validate_arithmetic(rhs, "operator/=");
    for (size_t i = 0; i < rhs.a.size(); ++i) {
      if (rhs.a[i] == 0.0) {
        throw std::runtime_error("operator/=:: divisor amplitude is zero at index " + std::to_string(i));
      }
    }
    for (size_t i = 0; i < this->f.size(); ++i) {
      this->a[i] /= rhs.a[i];
      this->p[i] -= rhs.p[i];
    }
    return *this;
  }

  std::string get_sensor() const {
    return this->sensor;
  }
  /// @brief Sets the sensor name.
  /// @param sensor The name of the sensor.
  void set_sensor(const std::string &sensor) {
    if (mstr::trim(sensor).empty()) {
      throw std::runtime_error("Sensor name cannot be empty");
    }
    this->sensor = mstr::trim(sensor);
    this->board.clear();
    this->system.clear();
  }

  /// @brief Sets the board name.
  /// @param board The name of the ADU board.
  void set_board(const std::string &board) {
    if (mstr::trim(board).empty()) {
      throw std::runtime_error("Board name cannot be empty");
    }
    if (this->system.empty()) {
      throw std::runtime_error("Cannot set board name when system name is empty");
    }
    this->board = mstr::trim(board);
  }

  /// @brief Sets the system name.
  /// @param system The name of the ADU system.
  void set_system(const std::string &system) {
    if (mstr::trim(system).empty()) {
      throw std::runtime_error("System name cannot be empty");
    }
    this->system = mstr::trim(system);
    this->sensor.clear();
  }

  void set_item(const std::string &item) {
    auto trimmed_item = mstr::trim(item);
    if (trimmed_item.contains("ADU"))
      this->set_system(trimmed_item);
    else
      this->set_sensor(trimmed_item);
  }
  /// @brief Gets the system name.
  /// @return The name of the ADU system.
  std::string get_system() const {
    return this->system;
  }
  /// @brief Gets the board name.
  /// @return The name of the ADU board.
  std::string get_board() const {
    return this->board;
  }

  uint64_t get_serial() const {
    return this->serial;
  }
  void set_serial(const uint64_t &serial) {
    this->serial = serial;
  }

  /// @brief Sets the sampling rate. Needed for board / system calibration.
  /// @param sampling_rate The sampling rate in Hz. Must be non-negative.
  void set_sampling_rate(const double &sampling_rate) {
    if (sampling_rate < 0.0) {
      throw std::invalid_argument("Sampling rate cannot be negative");
    }
    this->sampling_rate = sampling_rate;
  }

  /// @brief Gets the sampling rate.
  /// @return The sampling rate in Hz.
  double get_sampling_rate() const {
    return this->sampling_rate;
  }

  /// @brief Sets the resistance value of the ADU board. For coils does not matter
  /// @param R The resistance value in ohms. Must be non-negative.
  void set_resistance(const double &R) {
    if (R < 0.0) {
      throw std::invalid_argument("Resistance cannot be negative");
    }
    this->R = R;
  }
  /// @brief Gets the resistance value of the ADU board. For coils does not matter.
  /// @return The resistance value in ohms.
  double get_resistance() const {
    return this->R;
  }

  /// @brief Sets the capacitance value of the ADU board. For coils does not matter.
  /// @param C The capacitance value in farads. Must be non-negative.
  void set_capacitance(const double &C) {
    if (C < 0.0) {
      throw std::invalid_argument("Capacitance cannot be negative");
    }
    this->C = C;
  }
  /// @brief Gets the capacitance value of the ADU board. For coils does not matter.
  /// @return The capacitance value in farads.
  double get_capacitance() const {
    return this->C;
  }

  std::string get_serial_string(const unsigned int FieldWidth = 0) const {
    std::ostringstream oss;
    if (FieldWidth > 0)
      oss << std::setw(FieldWidth) << std::setfill('0') << this->serial;
    else
      oss << this->serial;
    return oss.str();
  }
  std::string get_sensor_and_serial(const unsigned int FieldWidth = 0) const {
    std::ostringstream oss;
    oss << this->sensor << " #" << this->get_serial_string(FieldWidth);
    return oss.str();
  }

  // get the chopper status as string "ON" "OFF"
  std::string get_chopper_status() const {
    switch (this->chopper) {
    case ChopperStatus::on:
      return "ON";
    case ChopperStatus::off:
      return "OFF";
    default:
      return "UNKNOWN";
    }
  }

  /// @brief Checks if the chopper is on.
  /// @return True if the chopper is on, false otherwise.
  bool is_chopper_on() const {
    return this->chopper == ChopperStatus::on;
  }

  /// @brief Gets the frequency vector of the calibration data.
  /// @return The frequency vector as a std::vector<double>.
  std::vector<double> get_f() const {
    return this->f;
  }

  /// @brief Gets the amplitude vector of the calibration data.
  /// @return The amplitude vector as a std::vector<double>.
  std::vector<double> get_a() const {
    return this->a;
  }

  /// @brief Gets the phase vector of the calibration data.
  /// @return The phase vector as a std::vector<double>.
  std::vector<double> get_p() const {
    return this->p;
  }

  /// @brief Generates a logarithmically spaced frequency vector for the calibration data. Useful for creating theoretical / synthetic calibration data.
  /// @param f_start The starting frequency in Hz.
  /// @param f_stop The stopping frequency in Hz.
  /// @param num_points_decade The number of points per decade (default is 10).
  /// @throws std::invalid_argument if the frequency generation parameters are invalid.
  void gen_freqs(const double f_start, const double f_stop, const size_t num_points_decade = 10) {
    if (f_start <= 0.0 || f_stop <= 0.0 || num_points_decade < 1) {
      throw std::invalid_argument("Invalid frequency generation parameters");
    }
    const size_t num_points = static_cast<size_t>(std::ceil(std::log10(f_stop / f_start) * num_points_decade)) + 1;

    this->f.resize(num_points);
    const double log_f_start = std::log10(f_start);
    const double log_f_stop = std::log10(f_stop);
    const double delta = (log_f_stop - log_f_start) / (num_points - 1);
    for (size_t i = 0; i < num_points; ++i) {
      this->f[i] = std::pow(10.0, log_f_start + i * delta);
    }
  }

  /// @brief Converts theoretical calibration data to actual data.
  /// @details when you ONLY create a theoretical calibration, this function will convert it to actual data.
  void theo_to_data() {
    this->f = this->f_theo;
    this->a = this->a_theo;
    this->p = this->p_theo;
  }

  double f_min() const {
    return this->f.empty() ? 0.0 : this->f.front();
  }

  double f_max() const {
    return this->f.empty() ? 0.0 : this->f.back();
  }

  std::pair<double, double> f_range() const {
    return this->f.empty() ? std::make_pair(0.0, 0.0) : std::make_pair(this->f.front(), this->f.back());
  }

  size_t check_duplicates(const std::filesystem::path &source = {}) {
    if ((this->f.size() != this->a.size()) || (this->f.size() != this->p.size())) {
      throw std::runtime_error("calibration vectors f, a, and p have different sizes ->" + std::filesystem::absolute(source).string());
    }

    size_t removed = 0;
    size_t i = 1;
    while (i < this->f.size()) {
      if (this->f[i] < this->f[i - 1]) {
        throw std::runtime_error("calibration frequencies are not increasing ->" + std::filesystem::absolute(source).string());
      }
      if (this->f[i] != this->f[i - 1]) {
        ++i;
        continue;
      }
      const auto relative_deviation = [](const double lhs, const double rhs) {
        const double lhs_abs = std::abs(lhs);
        const double rhs_abs = std::abs(rhs);
        const double scale = lhs_abs > rhs_abs ? lhs_abs : rhs_abs;
        return scale == 0.0 ? 0.0 : std::abs(lhs - rhs) / scale;
      };
      const double amplitude_deviation = relative_deviation(this->a[i], this->a[i - 1]);
      const double phase_deviation = relative_deviation(this->p[i], this->p[i - 1]);
      if ((amplitude_deviation > 0.01) || (phase_deviation > 0.01)) {
        std::ostringstream error;
        error << "duplicate calibration frequency " << this->f[i] << " Hz for " << this->get_sensor_and_serial() << " (chopper " << this->get_chopper_status()
              << ") has conflicting values: (a=" << this->a[i - 1] << ", p=" << this->p[i - 1]
              << ") and (a=" << this->a[i] << ", p=" << this->p[i] << "), deviations: a=" << amplitude_deviation * 100.0 << "%, p=" << phase_deviation * 100.0
              << "% ->" << std::filesystem::absolute(source);
        throw std::runtime_error(error.str());
      }

      std::cerr << "Warning: removing duplicate calibration entry within 1% at " << this->f[i] << " Hz for " << this->get_sensor_and_serial() << " (chopper " << this->get_chopper_status()
                << "), deviations: a=" << amplitude_deviation * 100.0 << "%, p=" << phase_deviation * 100.0 << "% ->" << std::filesystem::absolute(source) << std::endl;
      this->f.erase(this->f.begin() + static_cast<std::ptrdiff_t>(i));
      this->a.erase(this->a.begin() + static_cast<std::ptrdiff_t>(i));
      this->p.erase(this->p.begin() + static_cast<std::ptrdiff_t>(i));
      ++removed;
    }
    return removed;
  }

  size_t get_cal_data(std::vector<double> &f, std::vector<double> &a, std::vector<double> &p, const CalibrationType ct_type = CalibrationType::mtx) const {
    f = this->f;
    a = this->a;
    p = this->p;
    if (ct_type == CalibrationType::mtx_old) {
      for (size_t i = 0; i < f.size(); ++i) {
        a[i] /= (1000. * f[i]);
      }
    }
    return f.size();
  }

  size_t get_master_cal(std::vector<double> &f, std::vector<double> &a, std::vector<double> &p, const CalibrationType ct_type = CalibrationType::mtx) const {
    if (!this->f_master.size()) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << ":: f_master.size() is 0";
      throw std::runtime_error(err_str.str());
    }
    f = this->f_master;
    a = this->a_master;
    p = this->p_master;
    if (ct_type == CalibrationType::mtx_old) {
      for (size_t i = 0; i < f.size(); ++i) {
        a[i] /= (1000. * f[i]);
      }
    }
    return f.size();
  }

  size_t get_theo_cal(std::vector<double> &f, std::vector<double> &a, std::vector<double> &p, const CalibrationType ct_type = CalibrationType::mtx) const {
    if (!this->f_theo.size()) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << ":: f_master.size() is 0";
      throw std::runtime_error(err_str.str());
    }
    f = this->f_theo;
    a = this->a_theo;
    p = this->p_theo;
    if (ct_type == CalibrationType::mtx_old) {
      for (size_t i = 0; i < f.size(); ++i) {
        a[i] /= (1000. * f[i]);
      }
    }
    return f.size();
  }

  /// @brief get a subrange of the frequency vector
  /// @param range min max
  /// @return new frequency vector
  std::vector<double> get_f(std::pair<double, double> &range) const {
    if ((range.first == range.second) && (range.first == 0)) // this is maybe an uninitialized range -> return all
      return this->f;
    if (range.first > range.second) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << ":: range.first > range.second ->";
      throw std::runtime_error(err_str.str());
    }

    std::vector<double> ret;
    ret.reserve(this->f.size());
    for (auto const &v : this->f) {
      if ((v >= range.first) && (v <= range.second))
        ret.push_back(v);
    }
    return ret;
  }

  /// @brief get a subrange of the amplitude vector
  /// @param range   min max amplitude
  /// @return amplitude vector withing range
  std::vector<double> get_a(std::pair<double, double> &range) const {
    if ((range.first == range.second) && (range.first == 0)) // this is maybe an uninitialized range -> return all
      return this->a;
    if (range.first > range.second) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << ":: range.first > range.second ->";
      throw std::runtime_error(err_str.str());
    }

    std::vector<double> ret;
    ret.reserve(this->a.size());
    for (auto const &v : this->a) {
      if ((v >= range.first) && (v <= range.second))
        ret.push_back(v);
    }
    return ret;
  }

  /// @brief get a subrange of the phase vector
  /// @param range   min max phase
  /// @return phase vector withing range
  std::vector<double> get_p(std::pair<double, double> &range) const {
    if ((range.first == range.second) && (range.first == 0)) // this is maybe an uninitialized range -> return all
      return this->p;
    if (range.first > range.second) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << ":: range.first > range.second ->";
      throw std::runtime_error(err_str.str());
    }

    std::vector<double> ret;
    ret.reserve(this->p.size());
    for (auto const &v : this->p) {
      if ((v >= range.first) && (v <= range.second))
        ret.push_back(v);
    }
    return ret;
  }

  /// \brief squeeze .. remove all entries which are not in the range
  /// \param f_range min max frequency
  /// \return size of the altered calibration
  size_t squeeze(std::pair<double, double> &f_range) {
    if ((f_range.first == f_range.second) && (f_range.first == 0)) // this is maybe an uninitialized range -> return all
      return this->f.size();
    if (f_range.first > f_range.second) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << ":: range.first > range.second ->";
      throw std::runtime_error(err_str.str());
    }

    // swap the internal f,a,p vectors
    std::vector<double> tmpf;
    std::vector<double> tmpa;
    std::vector<double> tmpp;
    std::swap(tmpf, this->f);
    std::swap(tmpa, this->a);
    std::swap(tmpp, this->p);

    this->f.reserve(tmpf.size());
    this->a.reserve(tmpa.size());
    this->p.reserve(tmpp.size());

    for (size_t i = 0; i < tmpf.size(); ++i) {
      if ((tmpf[i] >= f_range.first) && (tmpf[i] <= f_range.second)) {
        this->f.emplace_back(tmpf[i]);
        this->a.emplace_back(tmpa[i]);
        this->p.emplace_back(tmpp[i]);
      }
    }

    this->f.shrink_to_fit();
    this->a.shrink_to_fit();
    this->p.shrink_to_fit();

    return f.size();
  }

  void set_master_as_caldata() {
    if (!this->f_master.size()) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << ":: f_master.size() is 0";
      throw std::runtime_error(err_str.str());
    }
    this->f = this->f_master;
    this->a = this->a_master;
    this->p = this->p_master;
  }

  void set_theo_as_caldata() {
    if (!this->f_theo.size()) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << ":: f_theo.size() is 0";
      throw std::runtime_error(err_str.str());
    }
    this->f = this->f_theo;
    this->a = this->a_theo;
    this->p = this->p_theo;
  }

  // find value range inside frequency range and cout to console

  void find_fRange_valueRange(const std::pair<double, double> &f_range, const std::pair<double, double> &v_range, bool use_phase = true) const {
    if ((f_range.first == f_range.second) && (f_range.first == 0)) // this is maybe an uninitialized range -> return all
      return;
    if (f_range.first > f_range.second) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << ":: range.first > range.second ->";
      throw std::runtime_error(err_str.str());
    }

    if ((v_range.first == v_range.second) && (v_range.first == 0)) // this is maybe an uninitialized range -> return all
      return;
    if (v_range.first > v_range.second) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << ":: range.first > range.second ->";
      throw std::runtime_error(err_str.str());
    }

    size_t cnt = 0;
    for (size_t i = 0; i < this->f.size(); ++i) {
      if ((this->f[i] >= f_range.first) && (this->f[i] <= f_range.second)) {
        if (use_phase) {
          if ((this->p[i] >= v_range.first) && (this->p[i] <= v_range.second)) {
            std::cout << "f: " << this->f[i] << " a: " << this->a[i] << " p: " << this->p[i] << std::endl;
            ++cnt;
          }
        } else {
          if ((this->a[i] >= v_range.first) && (this->a[i] <= v_range.second)) {
            std::cout << "f: " << this->f[i] << " a: " << this->a[i] << " p: " << this->p[i] << std::endl;
            ++cnt;
          }
        }
      }
    }
    if (cnt)
      std::cout << this->sensor << " " << this->serial2string() << " " << this->chopper2string() << " found " << cnt << " entries" << std::endl;
  }

  std::string gen_json_filename_from_blank(const ChopperStatus &chopper) {
    std::string fname(this->sensor);
    fname += "_" + mstr::zero_fill_field(this->serial, 4);
    if (chopper == ChopperStatus::on)
      fname += "_chopper_on.json";
    else if (chopper == ChopperStatus::off)
      fname += "_chopper_off.json";
    else
      fname += ".json";

    return fname;
  }

  std::string basename() const {
    if (this->sensor.empty())
      return std::string();

    std::string fname(this->sensor);
    fname += "-" + mstr::zero_fill_field(this->serial, 4);
    if (this->chopper == ChopperStatus::on)
      fname += "_chopper_on";
    else if (this->chopper == ChopperStatus::off)
      fname += "_chopper_off";
    else
      fname += "_ukn";
    return fname;
  }

  /// @brief gets the plot name for the calibration object, like MFS-06e 1234 (on) or (off), or the board/system name if sensor is not set.
  /// @return The plot name as a string.
  std::string plot_name() const {
    if (!this->sensor.empty()) {

      std::string fname(this->sensor);
      fname += " " + mstr::zero_fill_field(this->serial, 4);
      if (this->chopper == ChopperStatus::on)
        fname += " (on)";
      else if (this->chopper == ChopperStatus::off)
        fname += " (off)";
      return fname;
    }
    if (!board.empty())
      return board;
    if (!system.empty())
      return system;

    return std::string();
  }

  /// @brief copy constructor
  /// @param rhs the calibration object to copy from
  calibration(const std::shared_ptr<calibration> &rhs) {
    if (rhs != nullptr) {
      this->sensor = rhs->sensor;
      this->serial = rhs->serial;
      this->chopper = rhs->chopper;
      this->units_frequency = rhs->units_frequency;
      this->units_amplitude = rhs->units_amplitude;
      this->units_phase = rhs->units_phase;
      this->datetime = rhs->datetime;
      this->Operator = rhs->Operator;
      this->f = rhs->f;
      this->a = rhs->a;
      this->p = rhs->p;
      this->ct = rhs->ct;
      this->f_master = rhs->f_master;
      this->a_master = rhs->a_master;
      this->p_master = rhs->p_master;
      this->f_theo = rhs->f_theo;
      this->a_theo = rhs->a_theo;
      this->p_theo = rhs->p_theo;
      this->f_backup = rhs->f_backup;
      this->a_backup = rhs->a_backup;
      this->p_backup = rhs->p_backup;
      this->R = rhs->R;
      this->C = rhs->C;
      this->sampling_rate = rhs->sampling_rate;
      this->board = rhs->board;
      this->system = rhs->system;
    }
  }

  /// @brief clear all values
  void clear() {
    this->sensor.clear();
    this->serial = 0;
    this->datetime.clear();
    this->Operator.clear();
    this->chopper = ChopperStatus::off;
    this->units_frequency = "unknown";
    this->units_amplitude = "unknown";
    this->units_phase = "unknown";
    this->ct = CalibrationType::nn;
    this->f.clear();
    this->a.clear();
    this->p.clear();
    this->f_master.clear();
    this->a_master.clear();
    this->p_master.clear();
    this->f_theo.clear();
    this->a_theo.clear();
    this->p_theo.clear();
    this->f_backup.clear();
    this->a_backup.clear();
    this->p_backup.clear();
    this->R = 1.0;
    this->C = 1.0;
    this->sampling_rate = 0.0;
    this->board.clear();
    this->system.clear();
    this->file_path_in.clear();
    this->file_path_out.clear();
  }

  void set_format(const CalibrationType ct, bool skip_date_time = true) {
    if (ct == CalibrationType::mtx_old) {
      this->units_frequency = "Hz";
      this->units_amplitude = "V/(nT*Hz)";
      this->units_phase = "degrees";
      if (!skip_date_time) {
        this->datetime = "1970-01-01T00:00:00";
      }
      this->Operator = "";
      this->ct = CalibrationType::mtx_old;
    } else if (ct == CalibrationType::mtx) {
      this->units_frequency = "Hz";
      this->units_amplitude = "mV/nT";
      this->units_phase = "degrees";
      if (!skip_date_time) {
        this->datetime = "1970-01-01T00:00:00";
      }
      this->Operator = "";
      this->ct = CalibrationType::mtx;

    } else if (ct == CalibrationType::nn) {
      this->units_frequency = "unknown";
      this->units_amplitude = "unknown";
      this->units_phase = "unknown";
      if (!skip_date_time) {
        this->datetime = "1970-01-01T00:00:00";
      }
      this->Operator = "";
      this->ct = CalibrationType::nn;
    }
  }

  size_t tasks_todo(const bool ampl_div_f = false, const bool ampl_mul_f = false, const bool ampl_mul_by_1000 = false,
                    const bool old_to_new = false, const bool new_to_old = false) {

    if (!this->f.size()) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << ":: no data! ->";
      throw std::runtime_error(err_str.str());
    }

    if ((this->f.size() != this->a.size()) || (this->f.size() != this->p.size())) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << ":: no data corrupted ->";
      throw std::runtime_error(err_str.str());
    }

    if (ampl_div_f) {
      for (size_t i = 0; i < f.size(); ++i) {
        this->a[i] /= this->f[i];
      }
    }

    if (ampl_mul_f) {
      for (size_t i = 0; i < f.size(); ++i) {
        this->a[i] *= this->f[i];
      }
    }
    if (ampl_mul_by_1000) {
      for (size_t i = 0; i < f.size(); ++i) {
        this->a[i] *= 1000.;
      }
    }

    if (old_to_new) {
      if (this->ct == CalibrationType::mtx_old) {
        for (size_t i = 0; i < f.size(); ++i) {
          this->a[i] *= (1000. * this->f[i]);
        }
        this->set_format(CalibrationType::mtx);
      }
    }

    if (new_to_old) {
      if (this->ct == CalibrationType::mtx) {
        for (size_t i = 0; i < f.size(); ++i) {
          this->a[i] /= (1000. * this->f[i]);
        }
        this->set_format(CalibrationType::mtx_old);
      }
    }

    return f.size();
  }

  bool is_empty() const {
    if (this->f.size())
      return false;
    return true;
  }

  std::string chopper2string() const {
    if (this->chopper == ChopperStatus::on)
      return std::string("on");
    return std::string("off");
  }

  int get_int_chopper() const {
    if (this->chopper == ChopperStatus::on)
      return 1;
    return 0;
  }

  void set_int_chopper(const int chopper) {
    if (chopper == 1)
      this->chopper = ChopperStatus::on;
    else
      this->chopper = ChopperStatus::off;
  }

  void set_chopper(const ChopperStatus chopper) {
    this->chopper = chopper;
  }

  std::string serial2string(const int digits = 4) const {
    return mstr::zero_fill_field(this->serial, 4);
    // return std::to_string(this->serial);
  }

  CalibrationType cal_type() const {
    if (this->units_amplitude == "mV/nT")
      return CalibrationType::mtx;
    else if (this->units_amplitude == "V/(nT*Hz)")
      return CalibrationType::mtx_old;
    return CalibrationType::nn;
  }

  /// \brief toJson_embedd
  /// \return returns a nlohmann::ordered_json for embedding; do NOT use for files! For files sensor, serial and chopper are part of file name
  nlohmann::ordered_json toJson_embedd() const {
    nlohmann::ordered_json head; // use ordered because of readability (vectors last)
    // use other.update(head); to join
    head["sensor_calibration"]["sensor"] = this->sensor;
    head["sensor_calibration"]["serial"] = this->serial;
    head["sensor_calibration"]["chopper"] = int(this->chopper);
    head["sensor_calibration"]["units_frequency"] = this->units_frequency;
    head["sensor_calibration"]["units_amplitude"] = this->units_amplitude;
    head["sensor_calibration"]["units_phase"] = this->units_phase;
    head["sensor_calibration"]["datetime"] = this->datetime;
    head["sensor_calibration"]["Operator"] = this->Operator;

    head["sensor_calibration"]["f"] = f;
    head["sensor_calibration"]["a"] = a;
    head["sensor_calibration"]["p"] = p;

    return head;
  }

  /// \brief write_file write a json file with sensor, serial and chopper are part of file name; CONTENT hast NOT! these parts
  /// hence that if e.g. sensor would be included inside the file, filename AND content must be altered in case - that is stupid
  /// \param directory_path_only
  /// \return site of calibration frequencies
  size_t write_file(const std::filesystem::path &directory_path_only) const {

    std::filesystem::path filepath(std::filesystem::canonical(directory_path_only));
    std::string fname(this->sensor);
    fname += "_" + mstr::zero_fill_field(this->serial, 4);
    if (this->chopper == ChopperStatus::on)
      fname += "_chopper_on.json";
    else if (this->chopper == ChopperStatus::off)
      fname += "_chopper_off.json";
    else
      fname += ".json";
    filepath /= fname;
    std::ofstream file;
    file.open(filepath, std::fstream::out | std::fstream::trunc);
    nlohmann::ordered_json head; // use ordered because of readability (vectors last)
    // use other.update(head); to join

    head["sensor_calibration"]["sensor"] = this->sensor;
    head["sensor_calibration"]["serial"] = this->serial;
    head["sensor_calibration"]["chopper"] = int(this->chopper);
    head["sensor_calibration"]["units_frequency"] = this->units_frequency;
    head["sensor_calibration"]["units_amplitude"] = this->units_amplitude;
    head["sensor_calibration"]["units_phase"] = this->units_phase;
    head["sensor_calibration"]["datetime"] = this->datetime;
    head["sensor_calibration"]["Operator"] = this->Operator;

    head["sensor_calibration"]["f"] = f;
    head["sensor_calibration"]["a"] = a;
    head["sensor_calibration"]["p"] = p;

    // std::cout << std::setw(2) << head << std::endl; // debug only

    if (!file.is_open()) {
      file.close();
      return 0;
    }

    file << std::setw(2) << head << std::endl;
    file.close();

    return this->f.size();
  }

  /// \brief extract_from_filename .. does NOT call clear (again)
  /// \param filepath
  /// \return
  int extract_from_filename(const std::filesystem::path &filepath) {
    std::filesystem::path name(filepath.filename());
    name.replace_extension("");
    std::string calfilename = name.string();
    std::string tmpcalfilename(calfilename);

    std::transform(tmpcalfilename.begin(), tmpcalfilename.end(), tmpcalfilename.begin(), ::tolower);
    bool has_chopname = false;
    if (mstr::ends_with(tmpcalfilename, "_chopper_off")) {
      has_chopname = true;
      this->chopper = ChopperStatus::off;
    } else if (mstr::ends_with(tmpcalfilename, "_chopper_on")) {
      has_chopname = true;
      this->chopper = ChopperStatus::on;
    }
    auto items = mstr::split(calfilename, '_');
    if (!items.size())
      return 0;

    if (has_chopname) {
      if (items.size() >= 4) {
        items.resize(items.size() - 2);
      }
    }

    this->serial = std::stoi(items.back());
    items.pop_back();
    for (auto const &str : items) {
      this->sensor += str + "_";
    }
    this->sensor.pop_back();

    return 1;
  }

  size_t old_to_newformat() {
    bool go_mtx_f = false;
    bool go_mtx_p = false;
    if (this->units_frequency == "Hz")
      go_mtx_f = true;
    if (this->units_phase == "degrees")
      go_mtx_p = true;

    if ((this->units_amplitude == "V/(nT*Hz)") && go_mtx_f && go_mtx_p) {

      this->ct = CalibrationType::mtx_old;
    }

    if ((this->units_amplitude == "mV/nT") && go_mtx_f && go_mtx_p) {

      this->ct = CalibrationType::mtx;
      return this->f.size();
    }

    if (this->ct == CalibrationType::mtx_old) {
      auto fi = this->f.cbegin();
      for (auto &v : this->a) {
        v *= 1000.0 * *fi++;
      }
      this->set_format(CalibrationType::mtx, true);
      return this->f.size();
    }

    return 0;
  }

  /// \brief parse_head .. hence that for electrodes we may have data, serial, only EFP-06 or so
  /// \return
  size_t parse_head(const nlohmann::ordered_json &head, const std::filesystem::path &filepath = "") {

    int64_t ch = 0;
    if (!head.contains("sensor_calibration"))
      return 0;

    if (head["sensor_calibration"].contains("sensor"))
      this->sensor = mstr::trim(std::string(head["sensor_calibration"]["sensor"]));
    if (head["sensor_calibration"].contains("serial"))
      this->serial = uint64_t(head["sensor_calibration"]["serial"]);
    else
      this->serial = 0;
    if (head["sensor_calibration"].contains("chopper"))
      ch = int64_t(head["sensor_calibration"]["chopper"]);
    if (ch == 1)
      this->chopper = ChopperStatus::on;
    else
      this->chopper = ChopperStatus::off;

    if (head["sensor_calibration"].contains("units_frequency"))
      this->units_frequency = std::string(head["sensor_calibration"]["units_frequency"]);
    if (head["sensor_calibration"].contains("units_amplitude"))
      this->units_amplitude = std::string(head["sensor_calibration"]["units_amplitude"]);
    if (head["sensor_calibration"].contains("units_phase"))
      this->units_phase = std::string(head["sensor_calibration"]["units_phase"]);
    if (head["sensor_calibration"].contains("datetime"))
      this->datetime = std::string(head["sensor_calibration"]["datetime"]);
    if (head["sensor_calibration"].contains("Operator"))
      this->Operator = std::string(head["sensor_calibration"]["Operator"]);

    if (head["sensor_calibration"].contains("f"))
      this->f = std::vector<double>(head["sensor_calibration"]["f"]);
    if (head["sensor_calibration"].contains("a"))
      this->a = std::vector<double>(head["sensor_calibration"]["a"]);
    if (head["sensor_calibration"].contains("p"))
      this->p = std::vector<double>(head["sensor_calibration"]["p"]);

    if ((this->f.size() != this->a.size()) || (this->f.size() != this->p.size())) {
      this->clear();
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << ":: calibration vecors f,a, are inconsistent ->" << std::filesystem::absolute(filepath);
      throw std::runtime_error(err_str.str());
    }
    if (!this->f.empty()) {
      for (size_t i = 0; i < this->f.size(); ++i) {
        if (!std::isfinite(this->f[i]) || !std::isfinite(this->a[i]) || !std::isfinite(this->p[i])) {
          throw std::runtime_error(std::string(__func__) + ":: calibration contains a non-finite value ->" + std::filesystem::absolute(filepath).string());
        }
        if (i > 0 && this->f[i] == this->f[i - 1]) {
          throw std::runtime_error(std::string(__func__) + ":: duplicate calibration frequency " + std::to_string(this->f[i]) + " Hz ->" + std::filesystem::absolute(filepath).string());
        }
        if (i > 0 && this->f[i] < this->f[i - 1]) {
          throw std::runtime_error(std::string(__func__) + ":: calibration frequencies are not strictly increasing ->" + std::filesystem::absolute(filepath).string());
        }
      }
    }
    bool go_mtx_f = false;
    bool go_mtx_p = false;
    if (this->units_frequency == "Hz")
      go_mtx_f = true;
    if (this->units_phase == "degrees")
      go_mtx_p = true;

    if ((this->units_amplitude == "V/(nT*Hz)") && go_mtx_f && go_mtx_p) {

      this->ct = CalibrationType::mtx_old;
    }

    if ((this->units_amplitude == "mV/nT") && go_mtx_f && go_mtx_p) {

      this->ct = CalibrationType::mtx;
    }
    this->backup();

    return this->f.size();
  }

  /// \brief read_file JSON format
  /// \param filepath from the extracted filename we generate type sensor serial chopper
  /// \return
  size_t read_file(const std::filesystem::path &filepath, const bool auto_convert = true) {
    this->clear();
    if (!std::filesystem::exists(filepath)) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << ":: file not found ->" << std::filesystem::absolute(filepath);
      throw std::runtime_error(err_str.str());
    }

    if (!this->extract_from_filename(filepath)) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << ":: filename can not be parsed! -> " << std::filesystem::absolute(filepath);
      throw std::runtime_error(err_str.str());
    }

    std::ifstream file;
    file.open(filepath, std::fstream::in);
    if (!file.is_open()) {
      file.close();
      return 0;
    }
    nlohmann::ordered_json head = nlohmann::ordered_json::parse(file);
    file.close();
    // std::cout << std::setw(2) << head << std::endl; // debug only

    this->parse_head(head, filepath);

    bool go_mtx_f = false;
    bool go_mtx_p = false;
    if (this->units_frequency == "Hz")
      go_mtx_f = true;
    if (this->units_phase == "degrees")
      go_mtx_p = true;

    if ((this->units_amplitude == "V/(nT*Hz)") && go_mtx_f && go_mtx_p) {

      this->ct = CalibrationType::mtx_old;
    }

    if ((this->units_amplitude == "mV/nT") && go_mtx_f && go_mtx_p) {

      this->ct = CalibrationType::mtx;
    }

    if (auto_convert) {
      if (this->ct == CalibrationType::mtx_old) {
        auto fi = this->f.cbegin();
        for (auto &v : this->a) {
          v *= 1000.0 * *fi++;
        }
        this->set_format(CalibrationType::mtx, true);
      }
    }

    return this->f.size();
  }

  void add_to_xml_1_of_3(std::shared_ptr<tinyxmlwriter> &tix, const std::string identifier = "") const {
    tix->push("calibration");

    tix->element("cal_version", "1.0");
    tix->element("creator", "cal_base");
    tix->element("user", "metronix");

    tix->push("calibrated_item");
    if (identifier.size()) {
      tix->element_attr("ci", "identifier", identifier, this->sensor);
    } else if (mstr::begins_with(this->sensor, "MFS")) {
      tix->element_attr("ci", "identifier", "coil", this->sensor);
    } else if (mstr::begins_with(this->sensor, "SHFT")) {
      tix->element_attr("ci", "identifier", "coil", this->sensor);
    } else if (mstr::begins_with(this->sensor, "FGS")) {
      tix->element_attr("ci", "identifier", "fluxgate", this->sensor);
    } else if (mstr::begins_with(this->sensor, "ADB")) {
      tix->element_attr("ci", "identifier", "board", this->sensor);
    } else
      tix->element("ci", this->sensor);
    tix->element("ci_serial_number", this->serial);
    tix->element_empty("ci_revision");
    tix->element("ci_date", this->datetime.substr(0, 9));
    tix->element("ci_time", this->datetime.substr(11));
    tix->element_empty("ci_calibration_valid_until");
    tix->element_empty("ci_next_calibration");
    tix->element_empty("ci_tag");
    tix->element_empty("ci_owner");
    tix->element_empty("ci_owners_address");
    tix->element("ci_manufacturer", "metronix");
    tix->element("ci_manufacturers_address", "Kocher Str. 3, 38120 Braunschweig, Germany");

    tix->pop("calibrated_item");
  }

  void add_to_xml_2_of_3(std::shared_ptr<tinyxmlwriter> &tix) const {

    for (size_t i = 0; i < this->f.size(); ++i) {
      auto chop = this->chopper2string();
      tix->push_caldata(static_cast<int>(this->chopper), 0, 0, 0, std::complex<double>(0, 0));
      tix->element_attr("c0", "unit", "V", 0.0, 6, true);
      tix->element_attr("c1", "unit", this->units_frequency, this->f.at(i), 6, true);
      tix->element_attr("c2", "unit", this->units_amplitude, this->a.at(i), 6, true);
      if (this->units_phase == "degrees")
        tix->element_attr("c3", "unit", "deg", this->p.at(i), 6, true);
      else
        tix->element_attr("c3", "unit", this->units_phase, this->p.at(i), 6, true);
      tix->pop("caldata");
    }
  }

  void add_to_xml_3_of_3(std::shared_ptr<tinyxmlwriter> &tix) const {

    tix->pop("calibration");
  }

  std::filesystem::path mtx_cal_head(const std::filesystem::path &directory_path_only, bool create_filepath_only) const {

    if (this->ct == CalibrationType::nn) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << ":: calibration type is CalibrationType::nn  -> no idea what to do in " << directory_path_only;
      throw std::runtime_error(err_str.str());
    }

    std::filesystem::path filepath(std::filesystem::canonical(directory_path_only));
    std::string fname(this->sensor);
    fname += "_" + (mstr::zero_fill_field(this->serial, 4) + ".txt");
    filepath /= fname;

    if (create_filepath_only) {
      return filepath;
    }

    std::ofstream file;

    file.open(filepath, std::fstream::out | std::fstream::trunc);

    if (!file.is_open()) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << ":: can not open ->" << std::filesystem::absolute(filepath);
      throw std::runtime_error(err_str.str());
    }

    // ISO is a date hint for YY/mm/DD
    file << "Calibration measurement with ISO" << std::endl;

    file << "Magnetometer: " << this->sensor << " #" << std::to_string(this->serial);
    file << " DateTime: " << this->datetime << std::endl
         << std::endl;

    file.close();

    return filepath;
  }

  void write_csv(const std::filesystem::path &directory_only) {
    std::filesystem::path filepath(std::filesystem::canonical(directory_only));
    std::string fname(this->sensor);
    fname += "_" + mstr::zero_fill_field(this->serial, 4);
    if (this->chopper == ChopperStatus::on)
      fname += "_chopper_on.csv";
    else
      fname += "_chopper_off.csv";
    filepath /= fname;
    std::ofstream file;
    file.open(filepath, std::fstream::out | std::fstream::trunc);

    if (!file.is_open()) {
      file.close();
      return;
    }
    file.setf(std::ios::scientific, std::ios::floatfield);
    file.precision(8);

    for (size_t i = 0; i < this->f.size(); ++i) {
      file << this->f.at(i) << ",";
      if (this->ct == CalibrationType::mtx)
        file << this->a.at(i) << ",";
      else
        file << (this->a.at(i) / (1000. * this->f[i])) << ",";
      file << this->p.at(i) << std::endl;
    }
    file << std::endl;
    file.close();
  }

  void mtx_cal_body(const std::filesystem::path &full_path_filename) const {

    std::ofstream file;
    file.open(full_path_filename, std::fstream::out | std::fstream::app);

    if (!file.is_open()) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << ":: can not open ->" << std::filesystem::absolute(full_path_filename);
      throw std::runtime_error(err_str.str());
    }

    file << "FREQUENCY    MAGNITUDE    PHASE" << std::endl;
    file << "Hz           V/(nT*Hz)    deg" << std::endl;
    if (this->chopper == ChopperStatus::off) {
      file << "Chopper off" << std::endl;
    }
    if (this->chopper == ChopperStatus::on) {
      file << "Chopper on" << std::endl;
    }

    file.setf(std::ios::scientific, std::ios::floatfield);
    file.precision(8);

    for (size_t i = 0; i < this->f.size(); ++i) {
      file << this->f.at(i) << "  ";
      if (this->ct == CalibrationType::mtx_old)
        file << this->a.at(i) << "  ";
      if (this->ct == CalibrationType::mtx)
        file << (this->a.at(i) / (1000. * this->f[i])) << "  ";
      file << this->p.at(i) << std::endl;
    }
    file << std::endl;
    file.close();
  }

  std::string brief() const {
    std::stringstream ss;
    ss << "Sensor: " << this->sensor << " Serial: " << this->serial << " Entries: " << this->f.size();
    return ss.str();
  }

  /// @brief interpolate the calibration data to a new frequency vector - which is the same as the FFT; do this FIRST!
  /// @param new_f e.g. the frequency vector of the FFT
  /// @return size of the new frequency vector
  size_t interpolate(const std::vector<double> &new_f) {

    if (!new_f.size()) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << ":: new_f.size() == 0 ->";
      throw std::runtime_error(err_str.str());
    }

    if (!this->f.size()) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << ":: this->f.size() == 0 ->";
      throw std::runtime_error(err_str.str());
    }

    // use the range of this->f and create a new vector from new_f
    std::vector<double> new_xf;
    new_xf.reserve(new_f.size());
    for (auto const &v : new_f) {
      if ((v >= this->f.front()) && (v <= this->f.back()))
        new_xf.push_back(v);
    }

    // to small to interpolate; delete my data; hence that cal is attached to the time series individually
    // we don't care about other calibrations or runs
    if (new_xf.size() < min_cal_size) {
      this->f.clear();
      this->a.clear();
      this->p.clear();
      return 0;
    }

    std::vector<double> new_a;
    std::vector<double> new_p;
    bvec::akima_vector_double(this->f, this->a, new_xf, new_a);
    bvec::akima_vector_double(this->f, this->p, new_xf, new_p);
    if (new_a.size() != new_xf.size()) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << ":: interpolation amplitude failed ->";
      throw std::runtime_error(err_str.str());
    }
    if (new_p.size() != new_xf.size()) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << ":: interpolation phase failed ->";
      throw std::runtime_error(err_str.str());
    }

    this->f = new_xf;
    this->a = new_a;
    this->p = new_p;

    return this->f.size();
  }

  /// @brief interpolate the calibration data to a new frequency vector - which is the same as the FFT; do this FIRST!
  /// @param new_f e.g. the frequency vector of the FFT
  /// @return size of the new frequency vector
  size_t interpolate_master_cal(const std::vector<double> &new_f) {

    if (!new_f.size()) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << ":: new_f.size() == 0 ->";
      throw std::runtime_error(err_str.str());
    }

    if (!this->f_master.size()) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << ":: this->f_master.size() == 0 ->";
      throw std::runtime_error(err_str.str());
    }

    // use the range of this->f and create a new vector from new_f
    std::vector<double> new_xf;
    new_xf.reserve(new_f.size());
    for (auto const &v : new_f) {
      if ((v >= this->f_master.front()) && (v <= this->f_master.back()))
        new_xf.push_back(v);
    }

    // to small to interpolate; delete my data; hence that cal is attached to the time series individually
    // we don't care about other calibrations or runs
    if (new_xf.size() < min_cal_size) {
      this->f_master.clear();
      this->a_master.clear();
      this->p_master.clear();
      return 0;
    }

    std::vector<double> new_a;
    std::vector<double> new_p;
    bvec::akima_vector_double(this->f, this->a, new_xf, new_a);
    bvec::akima_vector_double(this->f, this->p, new_xf, new_p);
    if (new_a.size() != new_xf.size()) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << ":: interpolation amplitude failed ->";
      throw std::runtime_error(err_str.str());
    }
    if (new_p.size() != new_xf.size()) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << ":: interpolation phase failed ->";
      throw std::runtime_error(err_str.str());
    }

    this->f_master = new_xf;
    this->a_master = new_a;
    this->p_master = new_p;

    return this->f_master.size();
  }

  /// @brief generate a theoretical calibration for the sensor; always use the mtx format by default; decide to convert at the end!
  /// @param f_in new frequency vector OR the existing one
  /// the result is stored in f_theo, a_theo, p_theo and is ON THE FREQUENCY GRID OF f_in which is same as FFT!
  ///
  std::vector<std::complex<double>> gen_cal_sensor(const std::vector<double> &f_in, const bool no_error = false) {

    if (!f_in.size()) {
      this->f_theo = this->f;
    } else {
      this->f_theo = f_in;
    }
    if (this->sensor.empty()) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << ":: sensor is empty";
      throw std::runtime_error(err_str.str());
    }
    std::vector<std::complex<double>> trf;
    if ((this->sensor == "MFS-06") || (this->sensor == "MFS-06e")) {
      trf = gen_trf_mfs06e(this->f_theo, this->chopper);
      bvec::cplx2ap(trf, this->a_theo, this->p_theo, true);
    } else if ((this->sensor == "MFS-07")) {
      trf = gen_trf_mfs07(this->f_theo, this->chopper);
      bvec::cplx2ap(trf, this->a_theo, this->p_theo, true);
    } else if ((this->sensor == "MFS-07e")) {
      trf = gen_trf_mfs07e(this->f_theo, this->chopper);
      bvec::cplx2ap(trf, this->a_theo, this->p_theo, true);
    } else if ((this->sensor == "MFS-12e")) {
      trf = gen_trf_mfs12e(this->f_theo, this->chopper);
      bvec::cplx2ap(trf, this->a_theo, this->p_theo, true);
    } else if ((this->sensor == "FGS-02")) {
      trf = gen_trf_fgs02(this->f_theo);
      bvec::cplx2ap(trf, this->a_theo, this->p_theo, true);
    } else if ((this->sensor == "FGS-03e")) {
      trf = gen_trf_fgs03e(this->f_theo);
      bvec::cplx2ap(trf, this->a_theo, this->p_theo, true);
    } else if ((this->sensor == "FGS-04e")) {
      trf = gen_trf_fgs04e(this->f_theo);
      bvec::cplx2ap(trf, this->a_theo, this->p_theo, true);
    } else if ((this->sensor == "FGS-05e")) {
      trf = gen_trf_fgs05e(this->f_theo);
      bvec::cplx2ap(trf, this->a_theo, this->p_theo, true);
    } else if ((this->sensor == "SHFT-02e") || (this->sensor == "SHFT-02")) {
      trf = gen_trf_shft02e(this->f_theo);
      bvec::cplx2ap(trf, this->a_theo, this->p_theo, true);
    } else if (no_error) { // if no_error is true, return an empty transfer function instead of throwing an error, needed for synthetic calibration or nested calls
      trf.clear();
      return trf;
    } else {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << ":: unknown sensor ->" << this->sensor;
      throw std::runtime_error(err_str.str());
    }
    return trf;
  }

  // inline std::vector<std::complex<double>> gen_trf_ad_7771_lf(const std::vector<double> &freqs, const double &sampling_rate, const double &resistance, const double &capacity) {
  std::vector<std::complex<double>> gen_cal_board(const std::vector<double> &f_in) {
    if (!f_in.size()) {
      this->f_theo = this->f;
    } else {
      this->f_theo = f_in;
    }
    std::vector<std::complex<double>> trf;
    if (this->system == "ADU-11e") {
      const double capacitance_nF = this->C * 1.0e9;
      trf = gen_trf_ad_7771_lf(f_theo, this->sampling_rate, this->R, capacitance_nF); // only ADC up to now
    }
    return trf;
  }

  std::vector<std::complex<double>> gen_synthetic_cal(const std::vector<double> &f_in) {
    if (!f_in.size()) {
      this->f_theo = this->f;
    } else {
      this->f_theo = f_in;
    }
    std::vector<std::complex<double>> trf;
    if (!this->sensor.empty()) {
      trf = gen_cal_sensor(this->f_theo, true);
    }
    if (!this->system.empty()) {
      trf = gen_cal_board(this->f_theo);
    }
    if (trf.empty()) {
      trf = std::vector<std::complex<double>>(this->f_theo.size(), std::complex<double>(1.0, 0.0));
    } else {
      bvec::cplx2ap(trf, this->a_theo, this->p_theo, true);
    }
    return trf;
  }

  /// @brief BOTH f_theo and f must be sorted and BOTH must be on the same frequency grid (FFT); INTERPOLATE the measured FIRST!
  /// @return size of the joined vector
  size_t join_lower_theo_and_measured_interpolated() {
    if (!this->f_theo.size()) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << ":: f_theo.size() is 0";
      throw std::runtime_error(err_str.str());
    }
    if (!this->f.size()) {
      // take the theoretical frequency and use it as measured frequency
      this->f = this->f_theo;
      this->a = this->a_theo;
      this->p = this->p_theo;
      return this->f.size();
    }
    if (this->f.size() < min_cal_size) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << ":: f.size() is < min_cal_size (mt_base.h) (6)";
      throw std::runtime_error(err_str.str());
    }

    std::vector<double> new_f;
    std::vector<double> new_a;
    std::vector<double> new_p;

    // iterate over the theoretical frequency and find the first measured frequency
    // theoretical and "measured - interpolated" are on the same frequency grid
    // so first we iterate over the theoretical frequency and find the first measured - interpolated frequency
    size_t i, last_f = SIZE_MAX;
    for (i = 0; i < this->f_theo.size(); ++i) {
      if (this->f_theo[i] < this->f.front()) {
        new_f.push_back(this->f_theo[i]);
        new_a.push_back(this->a_theo[i]);
        new_p.push_back(this->p_theo[i]);
        last_f = i;
      }
    }
    // for two iterations we sum the measured - interpolated frequency and divide by 2.0
    size_t j = 0;
    // as long we are completely in the measured, we can not join the theoretical and measured - interpolated; so skip
    if (last_f < SIZE_MAX) {
      ++last_f; // next theoretical frequency
      if (last_f >= this->f_theo.size() + overlapping_cal) {
        std::ostringstream err_str(__func__, std::ios_base::ate);
        err_str << ":: last_f >= this->f_theo.size() + overlapping_cal - theoretical frequency is too short";
        throw std::runtime_error(err_str.str());
      }
      for (size_t k = 0; k < overlapping_cal; ++k) {
        new_f.push_back(this->f[j]);
        new_a.push_back((this->a_theo[last_f] + this->a[j]) / 2.0);
        new_p.push_back((this->p_theo[last_f++] + this->p[j++]) / 2.0);
      }
    }
    // continue with the measured - interpolated frequency
    for (; j < this->f.size(); ++j) {
      new_f.push_back(this->f[j]);
      new_a.push_back(this->a[j]);
      new_p.push_back(this->p[j]);
    }
    // swap the internal f,a,p vectors with new_f,new_a,new_p
    std::swap(new_f, this->f);
    std::swap(new_a, this->a);
    std::swap(new_p, this->p);

    return this->f.size();
  }

  size_t join_higher_master_and_measured_interpolated() {
    if (!this->f_master.size()) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << ":: f_master.size() is 0";
      throw std::runtime_error(err_str.str());
    }
    if (!this->f.size()) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << ":: f.size() is 0";
      throw std::runtime_error(err_str.str());
    }
    if (this->f.size() < min_cal_size) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << ":: f.size() is < min_cal_size (mt_base.h) (6)";
      throw std::runtime_error(err_str.str());
    }

    std::vector<double> new_f;
    std::vector<double> new_a;
    std::vector<double> new_p;

    // iterate over the measured frequency and find the first master frequency
    // master and "measured - interpolated" are on the same frequency grid
    size_t i, last_f = SIZE_MAX;

    for (i = 0; i < this->f.size(); ++i) {
      if (this->f[i] > this->f_master.front()) {
        new_f.push_back(this->f[i]);
        new_a.push_back(this->a[i]);
        new_p.push_back(this->p[i]);
        last_f = i;
      }
    }

    // for two iterations we sum the measured - interpolated frequency and divide by 2.0
    size_t j = 0;
    // as long we are completely in the measured, we can not join the master and measured - interpolated; so skip
    if (last_f < SIZE_MAX) {
      ++last_f; // next master frequency
      if (last_f >= this->f_master.size() + overlapping_cal) {
        std::ostringstream err_str(__func__, std::ios_base::ate);
        err_str << ":: last_f >= this->f_master.size() + overlapping_cal - master frequency is too short";
        throw std::runtime_error(err_str.str());
      }
      for (size_t k = 0; k < overlapping_cal; ++k) {
        new_f.push_back(this->f_master[j]);
        new_a.push_back((this->a_master[last_f] + this->a[j]) / 2.0);
        new_p.push_back((this->p_master[last_f++] + this->p[j++]) / 2.0);
      }
    }
    // continue with the master frequency, which has the higher frequencies here
    for (; j < this->f_master.size(); ++j) {
      new_f.push_back(this->f_master[j]);
      new_a.push_back(this->a_master[j]);
      new_p.push_back(this->p_master[j]);
    }
    // swap the internal f,a,p vectors with new_f,new_a,new_p
    std::swap(new_f, this->f);
    std::swap(new_a, this->a);
    std::swap(new_p, this->p);

    return this->f.size();
  }

  void auto_extend_for_mth5() {
    // by default we can extend to lower frequencies but not to higher frequencies
    if (this->f.size() < min_cal_size)
      return;
    double lowest = 1.0 / 100000.0; ///< 100,000s for MFS coils chopper on
    size_t steps_per_decade = 10;
    if (this->sensor.substr(0, 3) == "FGS") {
      lowest = 1.0 / 100000000.0; // 100,000,000s for FGS unlimited
      steps_per_decade = 2;
    } else if (this->sensor.substr(0, 4) == "SHFT") {
      lowest = 1.0 / 100.0; // 100s for SHFT (which is already out of range)
      steps_per_decade = 10;
    }
    double highest = this->f.at(2);
    auto v = gen_equidistant_logvector_fixed(lowest, highest);
    if (v.size() < min_cal_size)
      return;
    this->gen_cal_sensor(v);
    this->join_lower_theo_and_measured_interpolated();
  }

  size_t set_master_cal(const std::vector<double> &f, const std::vector<double> &a, const std::vector<double> &p) {
    if (!f.size()) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << ":: f.size() is 0";
      throw std::runtime_error(err_str.str());
    }
    if (f.size() != a.size()) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << ":: f.size() != a.size()";
      throw std::runtime_error(err_str.str());
    }
    if (f.size() != p.size()) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << ":: f.size() != p.size()";
      throw std::runtime_error(err_str.str());
    }
    this->f_master = f;
    this->a_master = a;
    this->p_master = p;
    return f.size();
  }

  size_t get_cplx_cal(std::vector<double> &f, std::vector<std::complex<double>> &cal) const {
    if (!this->f.size()) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << ":: f.size() is 0";
      throw std::runtime_error(err_str.str());
    }

    f = this->f;
    cal.resize(this->a.size());
    for (size_t i = 0; i < this->a.size(); ++i) {
      cal[i] = std::polar(this->a[i], this->p[i] * M_PI / 180.0);
    }
    return f.size();
  }
  void backup() {
    this->f_backup = this->f;
    this->a_backup = this->a;
    this->p_backup = this->p;
  }

  void restore() {
    this->f = this->f_backup;
    this->a = this->a_backup;
    this->p = this->p_backup;
  }

  bool is_same_sensor(const std::shared_ptr<calibration> &rhs) const {
    if (this->sensor != rhs->sensor)
      return false;
    if (this->serial != rhs->serial)
      return false;
    return true;
  }

  bool is_identical(const std::shared_ptr<calibration> &rhs) const {
    if (this->sensor != rhs->sensor)
      return false;
    if (this->serial != rhs->serial)
      return false;
    if (this->chopper != rhs->chopper)
      return false;
    if (this->ct != rhs->ct)
      return false;
    if (this->f.size() != rhs->f.size())
      return false;
    return true;
  }

  void map_to_zero(const double &deg_greater_than = 358.0) {
    for (auto &v : this->p) {
      if (std::abs(v) > deg_greater_than)
        v = 0.0;
    }
  }

private:
  void validate_arithmetic(const calibration &rhs, const char *operation) const {
    if (this->f != rhs.f) {
      throw std::runtime_error(std::string(operation) + ":: frequency vectors must be identical");
    }
    if (this->a.empty() || this->p.empty() || rhs.a.empty() || rhs.p.empty()) {
      throw std::runtime_error(std::string(operation) + ":: amplitude and phase vectors must be non-empty in both operands");
    }
    if (this->a.size() != this->f.size() || this->p.size() != this->f.size() ||
        rhs.a.size() != rhs.f.size() || rhs.p.size() != rhs.f.size()) {
      throw std::runtime_error(std::string(operation) + ":: amplitude and phase sizes must match the frequency vector in both operands");
    }
  }

  // vectors for the theoretical calibration
  std::vector<double> f_theo;
  std::vector<double> a_theo;
  std::vector<double> p_theo;

  // vectors for the calibration file as backup
  std::vector<double> f_backup;
  std::vector<double> a_backup;
  std::vector<double> p_backup;

  // vectors from a master calibration file, e.g. for HIGHER frequencies as measured
  std::vector<double> f_master;
  std::vector<double> a_master;
  std::vector<double> p_master;

}; // end calibration    ************************************************************************************************

static bool operator==(const std::shared_ptr<calibration> &lhs, const std::shared_ptr<calibration> &rhs) {

  // do I want to include the serial ... or check if just the data is the same?

  if (lhs->sensor != rhs->sensor)
    return false;
  if (lhs->serial != rhs->serial)
    return false;
  if (lhs->chopper != rhs->chopper)
    return false;
  if (lhs->units_amplitude != rhs->units_amplitude)
    return false;
  if (lhs->units_frequency != rhs->units_frequency)
    return false;
  if (lhs->units_phase != rhs->units_phase)
    return false;
  if (lhs->datetime != rhs->datetime)
    return false;
  if (lhs->Operator != rhs->Operator)
    return false;
  if (lhs->ct != rhs->ct)
    return false;

  if (lhs->f != rhs->f)
    return false;
  if (lhs->a != rhs->a)
    return false;
  if (lhs->p != rhs->p)
    return false;

  return true;
}

/// compare a sensor - ignore the chopper
inline auto compare_same_sensor = [](const std::shared_ptr<calibration> &lhs, const std::shared_ptr<calibration> &rhs) -> bool {
  if (lhs->sensor != rhs->sensor)
    return false;
  if (lhs->units_amplitude != rhs->units_amplitude)
    return false;
  if (lhs->units_frequency != rhs->units_frequency)
    return false;
  if (lhs->units_phase != rhs->units_phase)
    return false;
  if (lhs->serial != rhs->serial)
    return false;

  return true;
};

inline auto compare_sensor_and_chopper = [](const std::shared_ptr<calibration> &lhs, const std::shared_ptr<calibration> &rhs) -> bool {
  if (lhs->sensor != rhs->sensor)
    return false;
  if (lhs->serial != rhs->serial)
    return false;
  if (lhs->chopper != rhs->chopper)
    return false;
  return true;
};

/// @brief Lambda function to find the other chopper.
///
/// This lambda function compares two `std::shared_ptr<calibration>` objects and returns true lhs has a different chopper setting.
///
/// @param lhs The left-hand side `std::shared_ptr<calibration>` object.
/// @param rhs The right-hand side `std::shared_ptr<calibration>` object.
/// @return True if lhs  and rhs have the same sensor and serial number but different chopper settings.
inline auto find_other_chopper = [](const std::shared_ptr<calibration> &lhs, const std::shared_ptr<calibration> &rhs) -> bool {
  if (lhs->sensor != rhs->sensor)
    return false;
  if (lhs->serial != rhs->serial)
    return false;
  if (lhs->chopper != rhs->chopper)
    return true;
  return false;
};

/// @brief pair calibrations with the same sensor and serial number but different chopper settings
/// @param calibrations vector of shared pointers to calibrations
/// @return a paired vector of shared pointers to calibrations, where the first element of the pair has the chopper on and second element has the chopper off
inline std::vector<std::pair<std::shared_ptr<calibration>, std::shared_ptr<calibration>>> mk_on_off(
    const std::vector<std::shared_ptr<calibration>> &calibrations) {
  std::vector<std::pair<std::shared_ptr<calibration>, std::shared_ptr<calibration>>> result;

  std::list<std::shared_ptr<calibration>> calibrations_list(calibrations.begin(), calibrations.end());

  if (!calibrations.size())
    return result;

  auto it = calibrations_list.begin();
  auto it2 = std::next(it);
  bool found = false;
  // we need at least two calibrations to make a pair
  if (calibrations_list.size() > 1) {
    do {
      while (it2 != calibrations_list.end()) {
        if (find_other_chopper(*it, *it2)) {
          if ((*it)->chopper == ChopperStatus::on)
            result.push_back(std::make_pair(*it, *it2));
          else
            result.push_back(std::make_pair(*it2, *it));
          it = calibrations_list.erase(it);   // delete the first element
          it2 = calibrations_list.erase(it2); // delete the second element, which can be the next element, it is dead then
          found = true;
          break;
        } else
          ++it2;
      }
      if (!found) {
        ++it;
        it2 = std::next(it);
      } else {
        if (calibrations_list.size() <= 1)
          break;
        it = calibrations_list.begin(); // ensure we start from the beginning
        it2 = std::next(it);
        found = false;
      }

    } while (it != calibrations_list.end());
  }

  // if list is not empty, make a pair with the same calibration for the possible last elements with single calibrations
  for (auto it = calibrations_list.begin(); it != calibrations_list.end(); ++it) {
    if ((*it)->chopper == ChopperStatus::on)
      result.push_back(std::make_pair(*it, std::shared_ptr<calibration>())); // chopper on and chopper off (empty)
    else
      result.push_back(std::make_pair(std::shared_ptr<calibration>(), *it)); // chopper on (empty) and chopper off
  }
  return result;
}

#endif
