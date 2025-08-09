#ifndef ATSFILE_HPP
#define ATSFILE_HPP

/*!
 * @file atsfile.hpp reads a binary ats file which consists of a header and data (int32_t) values.
 * the old survey structure (Northern_Mining), where the file normally resides looks like this:
 * .
 *└── Northern_Mining
 *    ├── cal
 *    │   └── MFS06E0005.txt
 *    ├── config
 *    ├── db
 *    ├── doc
 *    ├── dump
 *    ├── edi
 *    ├── filters
 *    ├── jle
 *    ├── jobs
 *    ├── log
 *    ├── processings
 *    ├── shell
 *    ├── tmp
 *    └── ts
 *        ├── Kocatepe
 *        │   ├── meas_2009-08-20_13-22-00
 *        │   │   ├── 085_2009-08-20_13-22-00_2009-08-21_07-00-00_R001_128H.xml
 *        │   │   ├── 085_V01_C00_R001_TEx_BL_128H.ats
 *        │   │   ├── 085_V01_C01_R001_TEy_BL_128H.ats
 *        │   │   ├── 085_V01_C02_R001_THx_BL_128H.ats
 *        │   │   ├── 085_V01_C03_R001_THy_BL_128H.ats
 *        │   │   └── 085_V01_C04_R001_THz_BL_128H.ats
 *        │   └── meas_2009-08-21_07-01-00
 *        │       ├── 085_2009-08-21_07-01-00_2009-08-21_07-06-00_R001_2048H.xml
 *        │       ├── 085_V01_C00_R001_TEx_BL_2048H.ats
 *        │       ├── 085_V01_C01_R001_TEy_BL_2048H.ats
 *        │       ├── 085_V01_C02_R001_THx_BL_2048H.ats
 *        │       ├── 085_V01_C03_R001_THy_BL_2048H.ats
 *        │       └── 085_V01_C04_R001_THz_BL_2048H.ats
 *        └── Sarıçam
 *            ├── meas_2009-08-20_13-22-00
 *            │   ├── 084_2009-08-20_13-22-00_2009-08-21_07-00-00_R001_128H.xml
 *            │   ├── 084_V01_C00_R001_TEx_BL_128H.ats
 *            │   ├── 084_V01_C01_R001_TEy_BL_128H.ats
 *            │   ├── 084_V01_C02_R001_THx_BL_128H.ats
 *            │   ├── 084_V01_C03_R001_THy_BL_128H.ats
 *            │   └── 084_V01_C04_R001_THz_BL_128H.ats
 *            └── meas_2009-08-21_07-01-00
 *                ├── 084_2009-08-21_07-01-00_2009-08-21_07-06-00_R001_2048H.xml
 *                ├── 084_V01_C00_R001_TEx_BL_2048H.ats
 *                ├── 084_V01_C01_R001_TEy_BL_2048H.ats
 *                ├── 084_V01_C02_R001_THx_BL_2048H.ats
 *                ├── 084_V01_C03_R001_THy_BL_2048H.ats
 *                └── 084_V01_C04_R001_THz_BL_2048H.ats
 *
 * @details the file name is serial_version_channel_type_sample_rate.ats nnn_V01_Cnn_Rnnn_Tcc_B[L, H, B]_<sample_rate>[H, s].ats
 * sample rate: s indicates seconds, H Hertz, B indicates LF, HF or BB board
 *
 * The serial_start_date_time_stop_date_time_sample_rate.xml  file contains the calibration data of the sensor. Sensor and xmlfile name are in the ats file header.
 * The meas_date_time directory is a container for the ATS files. The container can hold one or more sets of xml and ats files.
 */

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <shared_mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "atsheader_struct.h"
#include "json.hpp"
#include "mt_base.hpp"
#include "strings_etc.hpp"

//
//        REMINDER
//        array of chars are not NULL terminated !
//        achChanType   [2] = "Ex" is not NULL terminated
//
//

/*!
 * \brief The ats_sys_names; in the binary header the SystemType is stored like ADU08 (without - ) and likely without e at the end.
 * \details The ats_sys_names map is used to convert the SystemType from the binary header, so case insensitive comparison so ADU08e <-> adu08 --> ADU-08e
 * \note The ats_sys_names map is used to convert the System
 */
inline std::unordered_map<std::string, std::string> ats_sys_names = {
    {"adu06", "ADU-06"},
    {"adu07", "ADU-07e"},
    {"adu08", "ADU-08e"},
    {"adu09", "ADU-09u"},
    {"adu10", "ADU-10e"},
    {"adu11", "ADU-11e"},
    {"adu12", "ADU-12e"},
    {"adu07e", "ADU-07e"},
    {"adu08e", "ADU-08e"},
    {"adu10e", "ADU-10e"},
    {"adu11e", "ADU-11e"},
    {"adu12e", "ADU-12e"}};

/*!
 * \brief The ats_sys_types, used in GMS Version
 */
inline std::unordered_map<std::string, int> ats_sys_types = {
    {"ADU-06", 0},
    {"ADU-07e", 0},
    {"ADU-08e", 1},
    {"ADU-09u", 4},
    {"ADU-10e", 4},
    {"ADU-11e", 5},
    {"ADU-12e", 6}};

/*!
 * \brief The ats_sys_family, used in GMS Version
 */
inline std::unordered_map<std::string, int> ats_sys_family = {
    {"ADU-06", 6},
    {"ADU-07e", 7},
    {"ADU-08e", 8},
    {"ADU-09u", 9},
    {"ADU-10e", 10},
    {"ADU-11e", 11},
    {"ADU-12e", 12}};

// ************************************** A T S F I L E ********************************************************************
//
//
//
//
//
/*!
 * @brief Class for handling ATS files This class will ONLY EXIST as std::shared_ptr<atsfile> !
 */
class atsfile {
public:
  atsfile() = default;                               //!< empty constructor
  atsfile(const std::filesystem::path &ats_filename) //!< constructor with filename
      :
      ats_filename(ats_filename) {
    if (!std::filesystem::exists(ats_filename)) {
      throw std::runtime_error("File does not exist: " + ats_filename.string());
    }
    this->read_atsheader();
  }

  /*!
   * @brief Copy constructor for std::shared_ptr<atsfile>
   * @details This constructor allows creating a new atsfile object from an existing one, not incrementing the reference count.
   * @param rhs The other atsfile object
   * @return
   */
  atsfile(const std::shared_ptr<atsfile> &rhs) {
    this->ats_filename = rhs->ats_filename;
    this->atsheader_bin = rhs->atsheader_bin;
    this->ats_read_count = 0;  // reset read count
    this->ats_write_count = 0; // reset write count
    this->is_ok = rhs->is_ok;  // keep the status of the header
  }

  /*!
   * @brief make sure that files are closed
   */
  ~atsfile() {
    this->ats_data_in_file.close();
    this->ats_data_out_file.close();
  }

  /*!
   * @brief calculate the least significant bit (lsb) for a virtual 32 bit ADC (analog to digital converter)
   * @param ddata vector of double values, which we may want to put into the ats format.
   * @return lsbval in mV calculated from the data vector
   */
  double calc_lsb_from_dbl_vec_mV(const std::vector<double> &ddata) {
    double lsb = 0.0;
    auto minmax = minmax_element(ddata.cbegin(), ddata.cend());
    lsb = (2 * (minmax.second - minmax.first)) / pow(2, 32);
    this->atsheader_bin.lsbval = lsb;
    this->atsheader["lsbval"] = lsb; // update the header
    return this->atsheader_bin.lsbval;
  }

  /*!
   * @brief Set the ATS filename ONLY
   * @param ats_filename The new ATS filename
   * @param exists_check Bypass file existence check, e.g. when creating a new file
   * @throws std::runtime_error if the file does not exist and exists_check is true
   * @details This function sets the ATS filename and reads the ATS header from the file
   * @note replaces old set_new_filename(...) function with exists_check = false to be called
   */
  void set_ats_filename(const std::filesystem::path &ats_filename, const bool exists_check = false) {
    if (exists_check && !std::filesystem::exists(ats_filename)) {
      throw std::runtime_error("File does not exist: " + ats_filename.string());
    }
    this->ats_read_count = 0;        // reset read count
    this->ats_write_count = 0;       // reset write count
    this->ats_data_in_file.close();  // close any open input file stream
    this->ats_data_out_file.close(); // close any open output file stream
    this->ats_filename = ats_filename;
  }

  /*!
   * @brief Prepare the ATS file for reading data
   * @details resets the global read count, opens the file for reading, and seeks to the start of the data.
   * @throws std::runtime_error if the ATS header has not been read successfully or if the file cannot be opened
   * @return true if preparation was successful, false otherwise
   */
  bool prepare_read_data() {
    if (!is_ok) {
      throw std::runtime_error("ATS header not read successfully.");
    }
    this->ats_data_in_file.open(this->ats_filename, std::ios::binary);
    if (!this->ats_data_in_file.is_open()) {
      throw std::runtime_error("Could not open file for reading: " + this->ats_filename.string());
    }
    // Seek to start of data (header size)
    this->ats_data_in_file.seekg(sizeof(ATSHeader_80), std::ios::beg);
    if (!this->ats_data_in_file.good()) {
      throw std::runtime_error("Failed to seek to start of data in file: " + this->ats_filename.string());
    }
    this->ats_read_count = 0; // reset read count
    return true;
  }

  /*!
   * @brief Go to a specific ATS sample in the file
   * @param sample_no The sample number to seek to
   * @return True if successful, false otherwise
   */
  bool go_to_ats_sample(const size_t sample_no) {
    if (!this->ats_data_in_file.is_open()) {
      throw std::runtime_error("ATS file is not open for reading.");
    }
    if (sample_no >= this->atsheader_bin.samples) {
      throw std::out_of_range("Sample number out of range: " + std::to_string(sample_no));
    }
    // Calculate the position in the file
    std::streampos pos = sizeof(ATSHeader_80) + sample_no * sizeof(int32_t);
    this->ats_data_in_file.seekg(pos, std::ios::beg);
    if (!this->ats_data_in_file.good()) {
      throw std::runtime_error("Failed to seek to sample number: " + std::to_string(sample_no));
    }
    return true;
  }

  /**
   * @brief Read a vector of int32_t or double samples from the ATS file. returns 0 if no samples were read.
   * @details so the logic is to FIRST check read is ok at the beginning of the loop
   * @param data Reference to a vector<T> to store the samples, which control the size of the read block.
   * @details a local counter is incremented to count the total number of samples read.
   * @return false if EOF is reached or fails, true otherwise.
   */
  template <typename T>
  size_t ats_read_int_doubles(std::vector<T> &ints_doubles) {
    if (!is_ok) {
      throw std::runtime_error("ATS header not read successfully.");
    }
    if (!this->ats_data_in_file.is_open()) {
      throw std::runtime_error("ATS file is not open for reading.");
    }
    if (this->ats_data_in_file.eof()) {
      return 0; // EOF reached; ints_doubles not touched
    }
    // local data buffer to read int32_t samples
    std::vector<int32_t> data;
    data.reserve(ints_doubles.size()); // we use push_back, so reserve the size; we always know the samples read
    // now be push_back the data, no read count is needed, we have data.size()
    // we read one by one, because we may approach EOF of fail read
    size_t read_count = 0;
    while (read_count < ints_doubles.size() && !this->ats_data_in_file.eof() && this->ats_data_in_file.good()) {
      int32_t sample;
      this->ats_data_in_file.read(reinterpret_cast<char *>(&sample), sizeof(int32_t));
      if (this->ats_data_in_file.eof()) {
        return 0; // EOF reached, no samples read
      }
      if (!this->ats_data_in_file.good()) {
        throw std::runtime_error("Failed to read sample from file: " + this->ats_filename.string());
      }
      // convert to double if needed
      data.push_back(sample);
      ++read_count; // increment read count
    }
    if (read_count == 0) {
      std::cerr << "No samples read from file: " << this->ats_filename << std::endl;
      return 0; // no samples read
    }
    // if data.size() != ints_doubles.size(), we resize it
    if (data.size() != ints_doubles.size()) {
      ints_doubles.resize(data.size());
    }
    // copy (and convert) the data to the output vector
    for (size_t i = 0; i < data.size(); ++i) {
      if constexpr (std::is_same_v<T, int32_t>) {
        ints_doubles[i] = data[i]; // copy int32_t directly
      } else if constexpr (std::is_same_v<T, double>) {
        ints_doubles[i] = static_cast<double>(data[i]) * this->atsheader_bin.lsbval; // convert to double
      } else {
        throw std::runtime_error("Unsupported type for ats_read_int_doubles: " + std::string(typeid(T).name()));
      }
    }
    // we do not close the file here, because we may read more data
    this->ats_read_count += read_count;
    return ints_doubles.size(); // return the number of samples read
  }

  /*!
   * @brief open the ATS file for writing data
   * @details and reset the write count
   * @throws std::runtime_error if the ATS header has not been read successfully or if
   * @return
   */
  bool prepare_write_data() {
    if (!is_ok) {
      throw std::runtime_error("ATS header not read successfully.");
    }
    this->ats_data_out_file.open(this->ats_filename, std::ios::binary | std::ios::out | std::ios::trunc);
    if (!this->ats_data_out_file.is_open()) {
      throw std::runtime_error("Could not open file for writing: " + this->ats_filename.string());
    }
    // Write the header first
    this->ats_data_out_file.write(reinterpret_cast<const char *>(&this->atsheader_bin), sizeof(ATSHeader_80));
    if (!this->ats_data_out_file.good()) {
      throw std::runtime_error("Failed to write ATS header to file: " + this->ats_filename.string());
    }
    this->ats_write_count = 0; // reset write count
    return true;
  }
  /*!
   * @brief Write the ATS data to the file
   * @tparam T
   * @param lsbval if T is double, the lsbval is used to convert the double to int32_t
   * @details if given, and same as in header, not used when writing int32_t
   * @param ints_doubles data to write, can be int32_t or double
   * @return total number of samples written (ats_write_count)
   */
  template <typename T>
  size_t ats_write_ints_doubles(const double &lsbval, const std::vector<T> &ints_doubles) {
    if (!this->ats_data_out_file.is_open()) {
      throw std::runtime_error("ATS file is not open for writing.");
    }
    if (ints_doubles.empty()) {
      throw std::runtime_error("Data vector is empty, cannot write samples.");
    }
    // Write the data
    // case one, int32_t samples to int32_t
    if constexpr (std::is_same_v<T, int32_t>) {
      if (lsbval == this->atsheader_bin.lsbval) {
        int32_t idata32;
        for (const auto &idata : ints_doubles) {
          idata32 = idata;
          this->ats_data_out_file.write(reinterpret_cast<const char *>(&idata32), sizeof(int32_t));
        }
      } else {
        int32_t idata32;
        for (const auto &idata : ints_doubles) {
          double val = idata * lsbval;
          idata32 = int32_t(val / this->atsheader_bin.lsbval);
          this->ats_data_out_file.write(reinterpret_cast<const char *>(&idata32), sizeof(int32_t));
        }
      }
    }
    // case two, double samples to int32_t
    else if constexpr (std::is_same_v<T, double>) {
      int32_t idata32;
      for (const auto &data : ints_doubles) {
        idata32 = int32_t(data / this->atsheader_bin.lsbval);
        this->ats_data_out_file.write(reinterpret_cast<const char *>(&idata32), sizeof(int32_t));
      }
    } else {
      throw std::runtime_error("Unsupported type for ats_write_ints_doubles: " + std::string(typeid(T).name()));
    }
    this->ats_write_count += ints_doubles.size();
    return this->ats_write_count;
  }

  /*!
   * @brief Write zero int32_t samples to the ATS file. Can be needed when concatenating files
   * @param n Number of samples to write
   * @param close_after_write If true, close the file after writing
   * @return Total number of samples written (ats_write_count)
   */
  size_t write_zero_ints(const size_t &n, const bool close_after_write = false) {
    if (!this->ats_data_out_file.is_open()) {
      throw std::runtime_error("ATS file is not open for writing.");
    }
    int32_t tint = 0;
    for (size_t i = 0; i < n; ++i) {
      this->ats_data_out_file.write(reinterpret_cast<const char *>(&tint), sizeof(int32_t));
    }
    if (close_after_write) {
      this->ats_data_out_file.close();
    }
    this->ats_write_count += n;
    return this->ats_write_count;
  }

  void read_atsheader() {
    std::ifstream file(this->ats_filename, std::ios::binary);
    if (!file.is_open()) {
      throw std::runtime_error("Could not open file: " + this->ats_filename.string());
    }
    // Read the header
    file.read(reinterpret_cast<char *>(&this->atsheader_bin), sizeof(this->atsheader_bin));
    if (file.gcount() != sizeof(this->atsheader_bin)) {
      throw std::runtime_error("Failed to read ATS header from file: " + this->ats_filename.string());
    }
    file.close();
    this->is_ok = true; // Mark the header as successfully read
    // Convert header data to internal representation
    this->atsheader_bin_to_atsheader();
  }

  /*!
   * @brief Write the ATS header to the file
   * @details This function writes the ATS header to the file, creating a new file if `new_file` is true.
   * @throws std::runtime_error if the file cannot be opened or written to
   * @param new_file
   */
  void write_atsheader(const bool new_file = true) {
    if (!is_ok && !new_file) {
      throw std::runtime_error("ATS header not read / created successfully.");
    }
    std::ofstream file;
    if (new_file) {
      file.open(this->ats_filename, std::ios::binary | std::ios::trunc);
    } else if (!std::filesystem::exists(this->ats_filename)) {
      throw std::runtime_error("File does not exist: " + this->ats_filename.string());
    } else {
      file.open(this->ats_filename, std::ios::binary | std::ios::in | std::ios::out);
    }
    if (!file.is_open()) {
      throw std::runtime_error("Could not open file for writing: " + this->ats_filename.string());
    }
    // Write the header
    file.write(reinterpret_cast<const char *>(&this->atsheader_bin), sizeof(this->atsheader_bin));
    if (!file) {
      throw std::runtime_error("Failed to write ATS header to file: " + this->ats_filename.string());
    }
    file.close();
    is_ok = true; // Mark the header as successfully written / read
  }

  /*!
   * @brief Write the ATS header to a JSON file, so we can keep the COMPLETE metadata
   * @param outdir_only The output directory for the JSON file, file will be named <ats_filename>.json
   */
  void write_ats_json_header(const std::filesystem::path &outdir_only) const {
    if (!is_ok) {
      throw std::runtime_error("ATS header not read successfully.");
    }
    std::filesystem::path json_file = outdir_only / (this->ats_filename.stem().string() + ".json");
    std::ofstream file(json_file);
    if (!file.is_open()) {
      throw std::runtime_error("Could not open JSON file for writing: " + json_file.string());
    }
    // we want an indented JSON output, indentation of 2 spaces
    file << std::setw(2) << this->atsheader << std::endl;
    file.close();
  }

  size_t write_ascii() {
    // ensure header is read first, e.g. use the right class constructor
    if (!is_ok) {
      throw std::runtime_error("ATS header not read successfully. Use the class constructor to read the header first.");
    }
    std::fstream afile;
    std::filesystem::path afilename;
    size_t sz = 0;
    afilename = this->ats_filename.parent_path();
    afilename /= this->ats_filename.stem();
    afilename += ".tsdata";
    afile.open(afilename, std::ios::out | std::ios::trunc);
    if (!afile.is_open()) {
      throw std::runtime_error("Could not open ASCII file for writing: " + afilename.string());
    }
    std::vector<double> ints_doubles(8192 * 64); // use big chunks
    size_t i = 0;
    while (this->ats_read_int_doubles(ints_doubles)) {
      std::cout << i++ << std::endl;
      for (const auto &d : ints_doubles) {
        afile << d << std::endl;
      }
      sz += ints_doubles.size();
    }
    afile.close();
    return sz;
  }

  /*!
   * @brief get the start date and time of the ATS file as string in ISO 8601 format
   * @return start date T time of the ATS file as a string in ISO 8601 format
   */
  std::string ats_start_date_time() const {
    if (!is_ok) {
      throw std::runtime_error("ATS header not read successfully.");
    }
    long int utc = static_cast<int64_t>(this->atsheader_bin.start);
    return mstr::iso8601_time_t(utc, 0); // 0 returns datetime with T
  }
  std::string ats_start_date() const {
    if (!is_ok) {
      throw std::runtime_error("ATS header not read successfully.");
    }
    long int utc = static_cast<int64_t>(this->atsheader_bin.start);
    return mstr::iso8601_time_t(utc, 1); // 1 returns the date only
  }
  std::string ats_start_time() const {
    if (!is_ok) {
      throw std::runtime_error("ATS header not read successfully.");
    }
    long int utc = static_cast<int64_t>(this->atsheader_bin.start);
    return mstr::iso8601_time_t(utc, 2, 0); // 2 returns the time only, no fractional seconds
  }

  /*!
   * @brief Get the start time of the ATS file as seconds since 1970
   * @return The start time as a int64_t
   * @throws std::runtime_error if the ATS header has not been read successfully
   */
  int64_t ats_start_secs_since_1970() const {
    if (!is_ok) {
      throw std::runtime_error("ATS header not read successfully.");
    }
    return static_cast<int64_t>(this->atsheader_bin.start);
  }

  /*!
   * @brief stop time in seconds since 1970, no fractional seconds; a following ats file may start at next second
   * @details The stop time is calculated as the start time plus the number of samples divided by the sample rate.
   * This gives the stop time in seconds since 1970
   * @return stop time in seconds since 1970, no fractional seconds
   * @throws std::runtime_error if the ATS header has not been read successfully
   */
  int64_t ats_stop_secs_since_1970() const {
    if (!is_ok) {
      throw std::runtime_error("ATS header not read successfully.");
    }
    long double sf_lhs = static_cast<long double>(this->atsheader_bin.samples) / static_cast<long double>(this->atsheader_bin.sample_rate);
    sf_lhs += static_cast<long double>(this->atsheader_bin.start);
    double f = 0.0;
    double intpart;
    f = modf(sf_lhs, &intpart);
    return static_cast<int64_t>(intpart);
  }

  /*!
   * @brief Get the stop date of the ATS file
   * @return The stop date as a string
   */
  std::string ats_stop_date() const {
    if (!is_ok) {
      throw std::runtime_error("ATS header not read successfully.");
    }
    long double sf_lhs = static_cast<long double>(this->atsheader_bin.samples) / static_cast<long double>(this->atsheader_bin.sample_rate);
    sf_lhs += static_cast<long double>(this->atsheader_bin.start);
    double f = 0.0;
    double intpart;
    f = modf(sf_lhs, &intpart);
    int64_t utc = static_cast<int64_t>(intpart);
    return mstr::iso8601_time_t(utc, 1); // 1 returns the date only
  }

  std::string ats_stop_time() const {
    if (!is_ok) {
      throw std::runtime_error("ATS header not read successfully.");
    }
    long double sf_lhs = static_cast<long double>(this->atsheader_bin.samples) / static_cast<long double>(this->atsheader_bin.sample_rate);
    sf_lhs += static_cast<long double>(this->atsheader_bin.start);
    double f = 0.0;
    double intpart;
    f = modf(sf_lhs, &intpart);
    int64_t utc = static_cast<int64_t>(intpart);
    return mstr::iso8601_time_t(utc, 2, 0); // 2 returns the time only, no fractional seconds
  }

  /*!
   * @brief create a default header for a given channel type; all fields are set to 0, except for the channel type specific fields
   * @param channel_type
   */
  void create_default_header(const std::string channel_type) {
    this->atsheader["header_length"] = static_cast<int64_t>(1024);
    this->atsheader["header_version"] = static_cast<int64_t>(80);
    this->atsheader["samples"] = static_cast<int64_t>(0);
    this->atsheader["lsbval"] = static_cast<int64_t>(0);
    this->atsheader["sample_rate"] = static_cast<double>(0);
    this->atsheader["start"] = static_cast<int64_t>(0);
    this->atsheader["lsbval"] = static_cast<double>(0);
    this->atsheader["GMToffset"] = static_cast<int64_t>(0);
    this->atsheader["orig_sample_rate"] = static_cast<double>(0);

    this->atsheader["x1"] = static_cast<double>(0);
    this->atsheader["y1"] = static_cast<double>(0);
    this->atsheader["z1"] = static_cast<double>(0);
    this->atsheader["x2"] = static_cast<double>(0);
    this->atsheader["y2"] = static_cast<double>(0);
    this->atsheader["z2"] = static_cast<double>(0);

    this->atsheader["serial_number"] = static_cast<int64_t>(999);
    if (channel_type == "Ex") {
      this->atsheader["serial_number_ADC_board"] = static_cast<int64_t>(1);
      this->atsheader["channel_number"] = static_cast<int64_t>(0);
      this->atsheader["sensor_type"] = "EFP06";
      this->atsheader["sensor_serial_number"] = 1;
      this->atsheader["x1"] = static_cast<double>(-500);
      this->atsheader["x2"] = static_cast<double>(500);
      this->atsheader["InputDivOn"] = static_cast<int64_t>(0);

    } else if (channel_type == "Ey") {
      this->atsheader["serial_number_ADC_board"] = static_cast<int64_t>(2);
      this->atsheader["channel_number"] = static_cast<int64_t>(1);
      this->atsheader["sensor_type"] = "EFP06";
      this->atsheader["sensor_serial_number"] = 2;
      this->atsheader["y1"] = static_cast<double>(-500);
      this->atsheader["y2"] = static_cast<double>(500);
      this->atsheader["InputDivOn"] = static_cast<int64_t>(0);

    } else if (channel_type == "Hx") {
      this->atsheader["serial_number_ADC_board"] = static_cast<int64_t>(3);
      this->atsheader["channel_number"] = static_cast<int64_t>(2);
      this->atsheader["sensor_type"] = "MFS06E";
      this->atsheader["sensor_serial_number"] = 3;
      this->atsheader["InputDivOn"] = static_cast<int64_t>(1);

    } else if (channel_type == "Hy") {
      this->atsheader["serial_number_ADC_board"] = static_cast<int64_t>(4);
      this->atsheader["channel_number"] = static_cast<int64_t>(3);
      this->atsheader["sensor_type"] = "MFS06E";
      this->atsheader["sensor_serial_number"] = 4;
      this->atsheader["InputDivOn"] = static_cast<int64_t>(1);

    } else if (channel_type == "Hz") {
      this->atsheader["serial_number_ADC_board"] = static_cast<int64_t>(5);
      this->atsheader["channel_number"] = static_cast<int64_t>(4);
      this->atsheader["sensor_type"] = "MFS06E";
      this->atsheader["sensor_serial_number"] = 5;
      this->atsheader["InputDivOn"] = static_cast<int64_t>(1);
    }
    this->atsheader["chopper"] = static_cast<int64_t>(0);
    this->atsheader["channel_type"] = channel_type;

    // pos and diplength not supported since 2004 - inconsistency removed

    this->atsheader["rho_probe_ohm"] = static_cast<double>(1);
    this->atsheader["DC_offset_voltage_mV"] = static_cast<double>(0);
    this->atsheader["gain_stage1"] = static_cast<double>(1);
    this->atsheader["gain_stage2"] = static_cast<double>(1);

    // Data from status information ?
    this->atsheader["iLat_ms"] = static_cast<int64_t>(0);
    this->atsheader["iLong_ms"] = static_cast<int64_t>(0);
    this->atsheader["iElev_cm"] = static_cast<int64_t>(0);
    this->atsheader["Lat_Long_TYPE"] = "G";
    this->atsheader["coordinate_type"] = "U";
    this->atsheader["ref_meridian"] = static_cast<int64_t>(0);

    this->atsheader["Northing"] = static_cast<double>(0);
    this->atsheader["Easting"] = static_cast<double>(0);
    this->atsheader["gps_clock_status"] = "C";
    this->atsheader["GPS_accuracy"] = "1";
    this->atsheader["offset_UTC"] = 0;

    this->atsheader["SystemType"] = "ADU-08e";
    // beside from binary header
    this->atsheader["GMSno"] = 8;
    this->atsheader["TypeNo"] = 1;
    this->atsheader["ats_data_file"] = "";
    //
    set_filter_bank("ADU-08e");

    // Data from XML-Job specification
    this->atsheader["survey_header_filename"] = "";
    this->atsheader["type_of_meas"] = "MT";

    this->atsheader["DCOffsetCorrValue"] = static_cast<double>(0);
    this->atsheader["DCOffsetCorrOn"] = static_cast<int64_t>(0);
    this->atsheader["bit_indicator"] = static_cast<int64_t>(0);
    this->atsheader["result_selftest"] = "OK";
    this->atsheader["numslices"] = static_cast<int64_t>(0);

    this->atsheader["cal_freqs"] = static_cast<int64_t>(0);
    this->atsheader["cal_entry_length"] = static_cast<int64_t>(0);
    this->atsheader["cal_version"] = static_cast<int64_t>(0);
    this->atsheader["cal_start_address"] = static_cast<int64_t>(0);

    // bitfield; filterbank was set above
    this->atsheader["LF_filters"] = "";
    this->atsheader["UTMZone"] = "";
    this->atsheader["system_cal_datetime"] = static_cast<int64_t>(0);
    this->atsheader["sensor_cal_filename"] = "SENSOR.CAL";
    this->atsheader["sensor_cal_datetime"] = static_cast<int64_t>(0);

    this->atsheader["powerline1"] = static_cast<double>(0.0);
    this->atsheader["powerline2"] = static_cast<double>(0.0);

    // bitfield; filterbank was set above
    this->atsheader["HF_filters"] = "";
    this->atsheader["external_gain"] = static_cast<double>(0);
    this->atsheader["ADB_board_type"] = "LF";

    this->atsheader["Client"] = "";
    this->atsheader["Contractor"] = "";
    this->atsheader["Area"] = "";
    this->atsheader["SurveyID"] = "";
    this->atsheader["Operator"] = "";
    this->atsheader["SiteName"] = "";
    this->atsheader["XmlHeader"] = "";

    this->atsheader["Comments"] = "from ASCII";
    this->atsheader["SiteNameRR"] = "";
    this->atsheader["SiteNameEMAP"] = "";
  }

  /*!
   * @brief XmlHeader is stored in the header
   * @return xmlfilename
   */
  std::filesystem::path gen_xmlfilename() const {
    if (!is_ok) {
      throw std::runtime_error("ATS header not read successfully.");
    }
    // generate the preliminary XML filename from the ats_filename path excluding the filename
    std::filesystem::path xmlfilepath = this->ats_filename.parent_path();
    std::string xmlfile;
    std::string sad = this->ats_start_date();
    std::string sat = this->ats_start_time();
    std::string sod = this->ats_stop_date();
    std::string sot = this->ats_stop_time();
    std::string samp = mstr::sample_rate_to_str_simple(this->atsheader_bin.sample_rate);
    int irun = 0;
    auto tokens = mstr::split(this->ats_filename.filename().string(), '_');
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
    std::string run = mstr::zero_fill_field(irun, 3);
    std::string serial = mstr::zero_fill_field(this->atsheader_bin.serial_number, 3);
    std::replace(sat.begin(), sat.end(), ':', '-'); // replace all ':' to '-'
    std::replace(sot.begin(), sot.end(), ':', '-'); // replace all ':' to '-'

    xmlfile += serial;
    xmlfile += "_";
    xmlfile += sad;
    xmlfile += "_";
    xmlfile += sat;
    xmlfile += "_";
    xmlfile += sod;
    xmlfile += "_";
    xmlfile += sot;
    xmlfile += "_R";
    xmlfile += run;
    xmlfile += "_";
    xmlfile += samp;
    xmlfile += ".xml";
    // return the full path
    return xmlfilepath / xmlfile;
  }
  /*!
   * @brief Generate the XML filename as a string, using gen_xmlfilename
   * @param name_only If true, return only the filename, otherwise return the full path
   * @return The XML filename as a std::string
   * @throws std::runtime_error if the ATS header has not been read successfully
   */
  std::string gen_xmlfilename_str(bool name_only = true) const {
    if (!is_ok) {
      throw std::runtime_error("ATS header not read successfully.");
    }
    if (!name_only) {
      return gen_xmlfilename().string(); // return the full path
    }
    return gen_xmlfilename().filename().string(); // return only the filename
  }

  /*!
   * @brief Get the XML path from the ATS header, so we can use it for reading (calibration) data
   * @return The XML path as a std::filesystem::path
   * @throws std::runtime_error if the ATS header has not been read successfully
   */
  std::filesystem::path get_xmlpath() const {
    if (!is_ok) {
      throw std::runtime_error("ATS header not read successfully.");
    }
    // get the canonical path of the ATS file
    std::filesystem::path ats_path = std::filesystem::canonical(this->ats_filename).parent_path();
    return ats_path / this->atsheader["XmlHeader"].get<std::filesystem::path>();
  }

  /*!
   * @brief return the ATS filename as a std::filesystem::path; member is private^
   * @return file path of the ATS file
   */
  std::filesystem::path get_ats_path() const {
    return std::filesystem::canonical(this->ats_filename);
  }

  void change_ats_dir(const std::filesystem::path &new_dir) {
    std::filesystem::path fname = this->ats_filename.filename();
    this->ats_filename.clear();
    this->ats_filename = new_dir;
    this->ats_filename /= fname;
  }

  /*!
   * @brief Get the site name from the ATS filename; file is in ts/site_name/meas_date_time/xxx.ats
   * @return The site_name as a std::string
   */
  std::string site_name_ats() const {
    return this->ats_filename.parent_path().parent_path().filename().string();
  }

  std::string get_ats_filename(const size_t &run = 99) const {
    // 084_V01_C00_R001_TEx_BL_8S.ats
    std::string atsfile = mstr::zero_fill_field(this->atsheader_bin.serial_number, 3);
    atsfile += "_V01_C";
    atsfile += mstr::zero_fill_field(this->atsheader_bin.channel_number, 2);
    atsfile += "_R";
    atsfile += mstr::zero_fill_field(run, 3);
    atsfile += "_T";
    std::string ct;

    if (sizeof(this->atsheader_bin.channel_type) > 1) {
      atsfile += this->atsheader_bin.channel_type[0];
      atsfile += this->atsheader_bin.channel_type[1];
    }

    if (this->atsheader_bin.sample_rate < 4096.5)
      atsfile += "_BL_";
    else
      atsfile += "_BH_";

    atsfile += mstr::sample_rate_to_str_simple(this->atsheader_bin.sample_rate);
    atsfile += ".ats";
    return atsfile;
  }

  /*!
   * @brief Get the ATS header as a JSON object
   * @return The ATS header as a nlohmann::ordered_json object
   */
  nlohmann::ordered_json get_ats_header() const {
    if (!is_ok) {
      throw std::runtime_error("ATS header not read successfully.");
    }
    return this->atsheader;
  }

  // some getters and setters for the ATS header
  double get_lat() const {
    if (!is_ok) {
      throw std::runtime_error("ATS header not read successfully.");
    }
    return (this->atsheader_bin.iLat_ms / 1000.) / 3600.;
  }

  /*!
   * @brief set_double_lat
   * @param d ISO 6709, North latitude is positive, decimal fractions
   */
  void set_lat(const double &d) {
    if (!is_ok) {
      throw std::runtime_error("ATS header not read successfully.");
    }
    this->atsheader_bin.iLat_ms = static_cast<int32_t>(d * 3600000.);
  }

  double get_lon() const {
    if (!is_ok) {
      throw std::runtime_error("ATS header not read successfully.");
    }
    return (this->atsheader_bin.iLong_ms / 1000.) / 3600.;
  }

  /*!
   * @brief set_double_lon
   * @param d ISO 6709, East longitude is positive, decimal fractions
   */
  void set_lon(const double &d) {
    if (!is_ok) {
      throw std::runtime_error("ATS header not read successfully.");
    }
    this->atsheader_bin.iLong_ms = static_cast<int32_t>(d * 3600000.);
  }

  /*!
   * @brief get_elev elevation in meter
   * @return
   */
  double get_elev() const {
    if (!is_ok) {
      throw std::runtime_error("ATS header not read successfully.");
    }
    return (this->atsheader_bin.iElev_cm / 100.);
  }

  double get_sample_rate() const {
    if (!is_ok) {
      throw std::runtime_error("ATS header not read successfully.");
    }
    return this->atsheader_bin.sample_rate;
  }

  void set_elev(const double &d) {
    if (!is_ok) {
      throw std::runtime_error("ATS header not read successfully.");
    }
    this->atsheader_bin.iElev_cm = static_cast<int32_t>(d * 100.);
  }

  /*!
   * @brief pos2length calculate dipole length
   * @return
   */
  double pos2length() const {
    if (!is_ok) {
      throw std::runtime_error("ATS header not read successfully.");
    }
    double tx, ty, tz;
    tx = double(this->atsheader_bin.x2 - this->atsheader_bin.x1);
    ty = double(this->atsheader_bin.y2 - this->atsheader_bin.y1);
    tz = double(this->atsheader_bin.z2 - this->atsheader_bin.z1);

    double diplength = sqrt(tx * tx + ty * ty + tz * tz);
    if (diplength < 0.001)
      diplength = 0; // avoid rounding errors
    return diplength;
  }

  /*!
   * @brief pos2angle calculate angle for North to East
   * @return
   */
  double pos2angle() const {
    if (!is_ok) {
      throw std::runtime_error("ATS header not read successfully.");
    }
    if (!this->atsheader.contains("channel_type")) {
      throw std::runtime_error("atsheader channel_type entry not existing; read file and use get_ats_header()");
    }
    double tx, ty;
    tx = double(this->atsheader_bin.x2 - this->atsheader_bin.x1);
    ty = double(this->atsheader_bin.y2 - this->atsheader_bin.y1);

    double diplength = this->pos2length();

    // avoid calculation noise
    if (std::abs(tx) < 0.001)
      tx = 0.0;
    if (std::abs(ty) < 0.001)
      ty = 0.0;

    // many user do not set coordiantes for the coils
    // Hx
    if ((diplength == 0) && (this->atsheader["channel_type"].get<std::string>() == "Hx")) {
      return 0.; // NORTH
    }
    // Hy
    if ((diplength == 0) && (this->atsheader["channel_type"].get<std::string>() == "Hy")) {
      return 90.; // EAST
    }
    // Hz
    if ((diplength == 0) && (this->atsheader["channel_type"].get<std::string>() == "Hz")) {
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
   * \param init like "ADB" - we may other board names in future, use it together with system type
   * \return comma separated string,like ADB-LF,LF-RF-2; so the board type and the filters
   */
  std::string get_ats_filter(const std::string init) const {
    if (!is_ok) {
      throw std::runtime_error("ATS header not read successfully.");
    }
    std::string filter(init);
    filter += this->atsheader["ADB_board_type"];
    if (this->atsheader["LF_filters"].get<std::string>().size()) {
      filter += ",";
      filter += this->atsheader["LF_filters"];
    }
    if (this->atsheader["HF_filters"].get<std::string>().size()) {
      filter += ",";
      filter += this->atsheader["HF_filters"];
    }
    // check for
    // append if we have dividers
    if (this->atsheader["InputDivOn"] == 1)
      filter += ",div_8";
    else
      filter += ",div_1";
    // append gain_stage1, this is float, so we make a string gain_1_ + gain as int
    filter += ",gain_1_";
    // uint8_t fits always and is member of ADU
    filter += std::to_string(uint8_t(static_cast<int>(this->atsheader["gain_stage1"].get<float>())));
    // same for gain_stage2
    filter += ",gain_2_";
    filter += std::to_string(uint8_t(static_cast<int>(this->atsheader["gain_stage2"].get<float>())));

    // finally we check for direct mode
    double tmp_lsb = this->atsheader["lsbval"].get<double>();
    // if ... filter += ",direct_mode";

    return filter;
  }

  /*!
   * @brief electric channels can scale; I want mV/km
   * @details we however can NOT scale E[xyz] if length is 0! that would give us a division by zero
   * @return
   */
  bool can_and_want_scale() const {
    if (!is_ok) {
      throw std::runtime_error("ATS header not read successfully.");
    }
    if (!this->atsheader.contains("channel_type")) {
      throw std::runtime_error("atsheader not existing, channel_type entry missing; read file and use get_ats_header()");
    }
    if (this->pos2length() == 0.0)
      return false;
    std::vector<std::string> types;
    types.emplace_back("Ex");
    types.emplace_back("Ey");
    types.emplace_back("Ez");

    std::string mytpe = this->atsheader["channel_type"];
    for (const auto &type : types) {
      if (type == mytpe)
        return true;
    }

    return false;
  }

  /*!
   * @brief pos2tilt calculate tilt angle from position in 3D space
   * @return tilt angle; 90 = positive downwards, 0 = horizontal
   */
  double pos2tilt() const {
    if (!is_ok) {
      throw std::runtime_error("ATS header not read successfully.");
    }
    double tz = double(this->atsheader_bin.z2 - this->atsheader_bin.z1);

    double diplength = this->pos2length();
    // no coordinates given for Hz
    if ((diplength == 0) && (this->atsheader["channel_type"].get<std::string>() == "Hz")) {
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

  /*!
   * @brief convert dipole angle and length to position
   * @param length
   * @param angle_north_to_east in degrees, 0 = North, 90 = East, 180 = South, 270 = West
   */
  void dip2pos(const double &length, const double &angle_north_to_east) {
    if (!is_ok) {
      throw std::runtime_error("ATS header not read successfully.");
    }
    double tx = 0.5 * length * cos(angle_north_to_east * M_PI / 180.0); // North
    double ty = 0.5 * length * sin(angle_north_to_east * M_PI / 180.0); // East

    this->atsheader["x1"] = -tx;
    this->atsheader["y1"] = -ty;
    this->atsheader["x2"] = tx;
    this->atsheader["y2"] = ty;
  }

  /*!
   * @brief convert dipole length and tilt angle to z position
   * @details tilt angle is the angle from horizontal to vertical, so 90 degrees is vertical downwards
   * @param length
   * @param tilt in degrees, 90 = vertical downwards, 0 = horizontal
   */
  void dip2z(const double &length, const double &tilt) {
    if (!is_ok) {
      throw std::runtime_error("ATS header not read successfully.");
    }
    double tz = length * sin(tilt * M_PI / 180.0);
    this->atsheader["z1"] = 0.0;
    this->atsheader["z2"] = tz;
  }

  /*!
   * @brief needed when we want to write into subdirectory, where this file wants to be written
   * @details the measdir is the directory where the ATS file is stored, and has the start meas_date_time as name
   * @return
   */
  std::string measdir() const {
    if (!is_ok) {
      throw std::runtime_error("ATS header not read successfully.");
    }
    return mstr::measdir_time(static_cast<int64_t>(this->atsheader_bin.start));
  }

  /*!
   * @brief Get the chopper status from the ATS header, on is LF mode, off is HF mode
   * @return ChopperStatus indicating the current chopper status
   */
  ChopperStatus get_ats_chopper() const {
    if (!is_ok) {
      throw std::runtime_error("ATS header not read successfully.");
    }
    if (this->atsheader["chopper"] == 1)
      return ChopperStatus::on;
    return ChopperStatus::off;
  }

  /*!
   * @brief Get the run number from the ATS filename
   * @return int64_t indicating the run number
   */
  int64_t get_ats_run() const {
    if (!this->ats_filename.string().size())
      return 0;
    std::string base = this->ats_filename.stem().string();

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

  /*!
   * @brief Convert LF filter settings to a vector of integers
   * @return Vector of integers representing the LF filter settings
   */
  std::vector<uint8_t> LFFilter_to_ints() const {
    if (!is_ok) {
      throw std::runtime_error("ATS header not read successfully.");
    }
    std::vector<uint8_t> filters(8, 0);
    size_t i = 0;
    for (i = 0; i < sizeof(this->atsheader_bin.LF_filters); ++i) {
      filters[i] = static_cast<int8_t>(this->atsheader_bin.LF_filters[i]);
    }
    return filters;
  }

  /*!
   * @brief Convert HF filter settings to a vector of integers
   * @return Vector of integers representing the HF filter settings
   */
  std::vector<uint8_t> HFFilter_to_ints() const {
    if (!is_ok) {
      throw std::runtime_error("ATS header not read successfully.");
    }
    std::vector<uint8_t> filters(8, 0);
    size_t i = 0;
    for (i = 0; i < sizeof(this->atsheader_bin.HF_filters); ++i) {
      filters[i] = static_cast<int8_t>(this->atsheader_bin.HF_filters[i]);
    }
    return filters;
  }

  /*!
   * @brief Set the filter bank object
   * @details This function sets the filter bank for the ATS file based on the ADU.
   * @param ADUin The ADU string, e.g. "ADU-07e" or "ADU-08e"
   * @return size_t successfully created filters
   */

  size_t set_filter_bank(const std::string &ADUin) {
    std::string ADU(ADUin);
    if (ADUin == "ADU-06")
      ADU = "ADU-07e";
    if (ADUin == "ADU-07")
      ADU = "ADU-07e";
    if (ADUin == "ADU-08")
      ADU = "ADU-08e";
    // clear the filter maps
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
    // 1 Hz has been dropped for 08, was default for 07
    if ((ADU == "ADU-07e"))
      this->HF_Filters["HF-HP-1Hz"] = ADU::HF_HP_1Hz; //! 0x01 ADU07   HF-HP-1Hz 1Hz filter enable for HF board
    // 500Hz is the HP for 08 default
    if ((ADU == "ADU-08e"))
      this->HF_Filters["HF-HP-500Hz"] = ADU::HF_HP_500Hz; //! 0x02 ADU08   HF-HP-500Hz 500Hz filter enable for HF board

    return this->LF_Filters.size() + this->HF_Filters.size();

    // for LF filter may visible above 800 Hz
    // for HF filter may be visible above 8000 Hz
  }

  /*!
   * @brief return the LF filter strings as a comma separated string
   * @return comma separated string of LF filter names
   */
  std::string get_lf_filter_strings() const {
    std::string sfilter;
    std::vector<uint8_t> filters(this->LFFilter_to_ints());

    // the atsheader uses only the FIRST int for up to ADU-08e
    //!< @todo check for ADU-10e,11e,12e
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

  /*!
   * @brief return the HF filter strings as a comma separated string
   * @return comma separated string of HF filter names
   */
  std::string get_hf_filter_strings() const {
    std::string sfilter;
    std::vector<uint8_t> filters(this->HFFilter_to_ints());

    // the atsheader uses only the FIRST int
    //!< @todo check for ADU-10e,11e,12e
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
   * \brief set_hf_filter_int, so we get the ATS header binary representation
   * \details This function sets the HF filters in the ATS header binary representation.
   * \param cs_string comma separated list of filters
   */
  void set_hf_filter_int(const std::string &cs_string) {
    if (cs_string.empty()) {
      throw std::runtime_error("set_hf_filter_int: cs_string is empty.");
    }
    if (!is_ok) {
      throw std::runtime_error("set_hf_filter_int: ATS header not read successfully.");
    }
    std::vector<uint8_t> filters(8, 0);
    for (const auto &it : this->HF_Filters) {
      if (mstr::contains(cs_string, it.first))
        filters[0] += uint8_t(it.second);
    }

    for (size_t i = 0; i < sizeof(this->atsheader_bin.HF_filters); ++i) {
      this->atsheader_bin.HF_filters[i] = static_cast<char>(filters[i]);
    }
  }

  /*!
   * @brief set_lf_filter_int, so we get the ATS header binary representation
   * @details This function sets the LF filters in the ATS header binary representation.
   * @details The LF filters are set as a bitfield in the ATS header binary representation
   * @param cs_string comma separated list of filters, e.g. "LF-RF-1,LF-RF-2,LF-LP-4Hz"
   */
  void set_lf_filter_int(const std::string &cs_string) {
    if (cs_string.empty()) {
      throw std::runtime_error("set_lf_filter_int: cs_string is empty.");
    }
    if (!is_ok) {
      throw std::runtime_error("set_lf_filter_int: ATS header not read successfully.");
    }
    std::vector<uint8_t> filters(8, 0);

    for (const auto &it : this->LF_Filters) {
      if (mstr::contains(cs_string, it.first))
        filters[0] += uint8_t(it.second);
    }

    for (size_t i = 0; i < sizeof(this->atsheader_bin.LF_filters); ++i) {
      this->atsheader_bin.LF_filters[i] = static_cast<char>(filters[i]);
    }
  }
  // keep public becaus of lambda comparison functions
  nlohmann::ordered_json atsheader; //!< JSON representation of the header, keeps insertion order; this the working header, so I call it atsheader
  ATSHeader_80 atsheader_bin;       //!< ATS header structure, e.g. 1024 bytes

private:
  std::filesystem::path ats_filename;
  size_t ats_write_count = 0;      //!< count samples written to file, needed when finally re-write the header
  size_t ats_read_count = 0;       //!< count samples read from file, needed when finally re-write the header
  std::ifstream ats_data_in_file;  //!< input file stream for reading data
  std::ofstream ats_data_out_file; //!< output file stream for writing data
  bool is_ok = false;              //!< indicates that the header was read successfully
  std::unordered_map<std::string, ADU> LF_Filters;
  std::unordered_map<std::string, ADU> HF_Filters;

  /*!
   * @brief converts the internal ATS header binary representation to a JSON object
   * @details some values will be corrected, like ADU08 to ADU-08e; we use official names like in the manuals
   */
  void atsheader_bin_to_atsheader() {
    this->atsheader["header_length"] = static_cast<int64_t>(this->atsheader_bin.header_length);
    this->atsheader["header_version"] = static_cast<int64_t>(this->atsheader_bin.header_version);

    this->atsheader["samples"] = static_cast<int64_t>(this->atsheader_bin.samples);
    this->atsheader["sample_rate"] = static_cast<double>(this->atsheader_bin.sample_rate);
    this->atsheader["start"] = static_cast<int64_t>(this->atsheader_bin.start);
    this->atsheader["lsbval"] = static_cast<double>(this->atsheader_bin.lsbval);
    this->atsheader["GMToffset"] = static_cast<int64_t>(this->atsheader_bin.GMToffset);
    this->atsheader["orig_sample_rate"] = static_cast<double>(this->atsheader_bin.orig_sample_rate);

    this->atsheader["serial_number"] = static_cast<int64_t>(this->atsheader_bin.serial_number);
    this->atsheader["serial_number_ADC_board"] = static_cast<int64_t>(this->atsheader_bin.serial_number_ADC_board);
    this->atsheader["channel_number"] = static_cast<int64_t>(this->atsheader_bin.channel_number);
    this->atsheader["chopper"] = static_cast<int64_t>(this->atsheader_bin.chopper);

    // make sure that we have Ex .. Hy in camel case
    std::string tch = mstr::clean_bc_str(this->atsheader_bin.channel_type, 2);
    // x,y,z,T,t
    if (tch.size() == 1)
      this->atsheader["channel_type"] = tch;
    else {
      std::transform(tch.begin(), tch.begin() + 1, tch.begin(), ::toupper);
      std::transform(tch.begin() + 1, tch.end(), tch.begin() + 1, ::tolower);
      this->atsheader["channel_type"] = tch;
    }
    this->atsheader["sensor_type"] = mstr::clean_bc_str(this->atsheader_bin.sensor_type, 6);
    this->atsheader["sensor_serial_number"] = static_cast<int64_t>(this->atsheader_bin.sensor_serial_number);

    this->atsheader["x1"] = static_cast<double>(this->atsheader_bin.x1);
    this->atsheader["y1"] = static_cast<double>(this->atsheader_bin.y1);
    this->atsheader["z1"] = static_cast<double>(this->atsheader_bin.z1);
    this->atsheader["x2"] = static_cast<double>(this->atsheader_bin.x2);
    this->atsheader["y2"] = static_cast<double>(this->atsheader_bin.y2);
    this->atsheader["z2"] = static_cast<double>(this->atsheader_bin.z2);

    // pos and diplength not supported since 2004 - inconsistency removed

    this->atsheader["rho_probe_ohm"] = static_cast<double>(this->atsheader_bin.rho_probe_ohm);
    this->atsheader["DC_offset_voltage_mV"] = static_cast<double>(this->atsheader_bin.DC_offset_voltage_mV);
    this->atsheader["gain_stage1"] = static_cast<double>(this->atsheader_bin.gain_stage1);
    this->atsheader["gain_stage2"] = static_cast<double>(this->atsheader_bin.gain_stage2);

    // Data from status information ?
    this->atsheader["iLat_ms"] = static_cast<int64_t>(this->atsheader_bin.iLat_ms);
    this->atsheader["iLong_ms"] = static_cast<int64_t>(this->atsheader_bin.iLong_ms);
    this->atsheader["iElev_cm"] = static_cast<int64_t>(this->atsheader_bin.iElev_cm);
    this->atsheader["Lat_Long_TYPE"] = std::string(1, this->atsheader_bin.Lat_Long_TYPE);
    this->atsheader["coordinate_type"] = std::string(1, this->atsheader_bin.coordinate_type);
    this->atsheader["ref_meridian"] = static_cast<int64_t>(this->atsheader_bin.ref_meridian);

    this->atsheader["Northing"] = static_cast<double>(this->atsheader_bin.Northing);
    this->atsheader["Easting"] = static_cast<double>(this->atsheader_bin.Easting);
    this->atsheader["gps_clock_status"] = std::string(1, this->atsheader_bin.gps_clock_status);
    this->atsheader["GPS_accuracy"] = std::string(1, this->atsheader_bin.GPS_accuracy);
    this->atsheader["offset_UTC"] = static_cast<int64_t>(this->atsheader_bin.offset_UTC);

    int TypeNo = 0;
    int GMSno = 0;
    std::string Name("unknown");

    auto str1 = mstr::clean_bc_str(this->atsheader_bin.SystemType, 12);
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

    this->atsheader["SystemType"] = Name;
    // beside from binary atsheader
    this->atsheader["GMSno"] = GMSno;
    this->atsheader["TypeNo"] = TypeNo;
    this->atsheader["ats_data_file"] = this->ats_filename.filename().string();
    //
    set_filter_bank(Name);

    // Data from XML-Job specification
    this->atsheader["survey_header_filename"] = mstr::clean_bc_str(this->atsheader_bin.survey_header_filename, 12);
    this->atsheader["type_of_meas"] = mstr::clean_bc_str(this->atsheader_bin.type_of_meas, 4);

    this->atsheader["DCOffsetCorrValue"] = static_cast<double>(this->atsheader_bin.DCOffsetCorrValue);
    this->atsheader["DCOffsetCorrOn"] = static_cast<int64_t>(this->atsheader_bin.DCOffsetCorrOn);
    this->atsheader["InputDivOn"] = static_cast<int64_t>(this->atsheader_bin.InputDivOn);
    this->atsheader["bit_indicator"] = static_cast<int64_t>(this->atsheader_bin.bit_indicator);
    this->atsheader["result_selftest"] = mstr::clean_bc_str(this->atsheader_bin.result_selftest, 2);
    this->atsheader["numslices"] = static_cast<int64_t>(this->atsheader_bin.numslices);

    this->atsheader["cal_freqs"] = static_cast<int64_t>(this->atsheader_bin.cal_freqs);
    this->atsheader["cal_entry_length"] = static_cast<int64_t>(this->atsheader_bin.cal_entry_length);
    this->atsheader["cal_version"] = static_cast<int64_t>(this->atsheader_bin.cal_version);
    this->atsheader["cal_start_address"] = static_cast<int64_t>(this->atsheader_bin.cal_start_address);

    // bitfield; filterbank was set above
    this->atsheader["LF_filters"] = get_lf_filter_strings();
    this->atsheader["UTMZone"] = mstr::clean_bc_str(this->atsheader_bin.UTMZone, 12);
    this->atsheader["system_cal_datetime"] = static_cast<int64_t>(this->atsheader_bin.system_cal_datetime);
    this->atsheader["sensor_cal_filename"] = mstr::clean_bc_str(this->atsheader_bin.sensor_cal_filename, 12);
    this->atsheader["sensor_cal_datetime"] = static_cast<int64_t>(this->atsheader_bin.sensor_cal_datetime);

    this->atsheader["powerline1"] = static_cast<double>(this->atsheader_bin.powerline1);
    this->atsheader["powerline2"] = static_cast<double>(this->atsheader_bin.powerline2);

    // bitfield; filterbank was set above
    this->atsheader["HF_filters"] = get_hf_filter_strings();
    this->atsheader["external_gain"] = static_cast<double>(this->atsheader_bin.external_gain);
    this->atsheader["ADB_board_type"] = mstr::clean_bc_str(this->atsheader_bin.ADB_board_type, 4);

    this->atsheader["Client"] = mstr::clean_bc_str(this->atsheader_bin.comments.Client, 16);
    this->atsheader["Contractor"] = mstr::clean_bc_str(this->atsheader_bin.comments.Contractor, 16);
    this->atsheader["Area"] = mstr::clean_bc_str(this->atsheader_bin.comments.Area, 16);
    this->atsheader["SurveyID"] = mstr::clean_bc_str(this->atsheader_bin.comments.SurveyID, 16);
    this->atsheader["Operator"] = mstr::clean_bc_str(this->atsheader_bin.comments.Operator, 16);
    this->atsheader["SiteName"] = mstr::clean_bc_str(this->atsheader_bin.comments.SiteName, 112);
    this->atsheader["XmlHeader"] = mstr::clean_bc_str(this->atsheader_bin.comments.XmlHeader, 64);

    // remove the useless comment weather:
    std::string wstr = mstr::clean_bc_str(this->atsheader_bin.comments.Comments, 288);
    if (wstr == "weather:")
      wstr.clear();
    this->atsheader["Comments"] = wstr;
    this->atsheader["SiteNameRR"] = mstr::clean_bc_str(this->atsheader_bin.comments.SiteNameRR, 112);
    this->atsheader["SiteNameEMAP"] = mstr::clean_bc_str(this->atsheader_bin.comments.SiteNameEMAP, 112);
  }

  /*!
   * @brief converts the internal ATS header binary representation to a JSON object
   * @details  some values will be converted, like filter names, sensor type, etc.
   */
  void atsheader_to_atsheader_bin() {
    // make a clean & empty struct
    memset(&this->atsheader_bin, 0, sizeof(this->atsheader_bin));

    this->atsheader_bin.header_length = static_cast<uint16_t>(this->atsheader["header_length"]);
    this->atsheader_bin.header_version = static_cast<int16_t>(this->atsheader["header_version"]);

    this->atsheader_bin.samples = static_cast<uint32_t>(this->atsheader["samples"]);
    this->atsheader_bin.sample_rate = static_cast<float>(this->atsheader["sample_rate"]);
    this->atsheader_bin.start = static_cast<uint32_t>(this->atsheader["start"]);
    this->atsheader_bin.lsbval = static_cast<double>(this->atsheader["lsbval"]);
    this->atsheader_bin.GMToffset = static_cast<int32_t>(this->atsheader["GMToffset"]);
    this->atsheader_bin.orig_sample_rate = static_cast<float>(this->atsheader["orig_sample_rate"]);

    this->atsheader_bin.serial_number = static_cast<uint16_t>(this->atsheader["serial_number"]);
    this->atsheader_bin.serial_number_ADC_board = static_cast<uint8_t>(this->atsheader["serial_number_ADC_board"]);
    this->atsheader_bin.channel_number = static_cast<uint8_t>(this->atsheader["channel_number"]);
    this->atsheader_bin.chopper = static_cast<uint8_t>(this->atsheader["chopper"]);
    strncpy(this->atsheader_bin.channel_type, this->atsheader["channel_type"].get<std::string>().c_str(), sizeof(this->atsheader_bin.channel_type));

    // the atsheader can onöy hold 6 chars here and MFS-06e is stored as MFS06e - so check it
    std::string tmp_sensor = this->atsheader["sensor_type"].get<std::string>();
    if ((mstr::contains(tmp_sensor, "MFS")) || (mstr::contains(tmp_sensor, "SHFT")) || (mstr::contains(tmp_sensor, "FGS"))) {
      if (mstr::contains(tmp_sensor, "-"))
        tmp_sensor.erase(remove(tmp_sensor.begin(), tmp_sensor.end(), '-'), tmp_sensor.end());
    }
    if ((tmp_sensor.size() > 6) && (mstr::contains(tmp_sensor, "-")))
      tmp_sensor.erase(remove(tmp_sensor.begin(), tmp_sensor.end(), '-'), tmp_sensor.end());

    strncpy(this->atsheader_bin.sensor_type, tmp_sensor.c_str(), sizeof(this->atsheader_bin.sensor_type));
    this->atsheader_bin.sensor_serial_number = static_cast<int16_t>(this->atsheader["sensor_serial_number"]);

    this->atsheader_bin.x1 = static_cast<float>(this->atsheader["x1"]);
    this->atsheader_bin.y1 = static_cast<float>(this->atsheader["y1"]);
    this->atsheader_bin.z1 = static_cast<float>(this->atsheader["z1"]);
    this->atsheader_bin.x2 = static_cast<float>(this->atsheader["x2"]);
    this->atsheader_bin.y2 = static_cast<float>(this->atsheader["y2"]);
    this->atsheader_bin.z2 = static_cast<float>(this->atsheader["z2"]);

    this->atsheader_bin.rho_probe_ohm = static_cast<float>(this->atsheader["rho_probe_ohm"]);
    this->atsheader_bin.DC_offset_voltage_mV = static_cast<float>(this->atsheader["DC_offset_voltage_mV"]);
    this->atsheader_bin.gain_stage1 = static_cast<float>(this->atsheader["gain_stage1"]);
    this->atsheader_bin.gain_stage2 = static_cast<float>(this->atsheader["gain_stage2"]);

    this->atsheader_bin.iLat_ms = static_cast<int32_t>(this->atsheader["iLat_ms"]);
    this->atsheader_bin.iLong_ms = static_cast<int32_t>(this->atsheader["iLong_ms"]);
    this->atsheader_bin.iElev_cm = static_cast<int32_t>(this->atsheader["iElev_cm"]);

    this->atsheader_bin.Lat_Long_TYPE = this->atsheader["Lat_Long_TYPE"].get<std::string>().at(0);
    this->atsheader_bin.coordinate_type = this->atsheader["coordinate_type"].get<std::string>().at(0);

    this->atsheader_bin.ref_meridian = static_cast<int16_t>(this->atsheader["ref_meridian"]);

    this->atsheader_bin.Northing = static_cast<double>(this->atsheader["Northing"]);
    this->atsheader_bin.Easting = static_cast<double>(this->atsheader["Easting"]);
    this->atsheader_bin.gps_clock_status = this->atsheader["gps_clock_status"].get<std::string>().at(0);
    this->atsheader_bin.GPS_accuracy = this->atsheader["GPS_accuracy"].get<std::string>().at(0);
    this->atsheader_bin.offset_UTC = static_cast<int16_t>(this->atsheader["offset_UTC"]);

    std::string official_name = this->atsheader["SystemType"].get<std::string>(); // like ADU-08e
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
    this->set_filter_bank(this->atsheader["SystemType"]);
    strncpy(this->atsheader_bin.SystemType, header_name.c_str(), sizeof(this->atsheader_bin.SystemType));

    strncpy(this->atsheader_bin.survey_header_filename, this->atsheader["survey_header_filename"].get<std::string>().c_str(), sizeof(this->atsheader_bin.survey_header_filename));
    strncpy(this->atsheader_bin.type_of_meas, this->atsheader["type_of_meas"].get<std::string>().c_str(), sizeof(this->atsheader_bin.type_of_meas));
    this->atsheader_bin.DCOffsetCorrValue = static_cast<double>(this->atsheader["DCOffsetCorrValue"]);
    this->atsheader_bin.DCOffsetCorrOn = static_cast<int8_t>(this->atsheader["DCOffsetCorrOn"]);
    this->atsheader_bin.InputDivOn = static_cast<int8_t>(this->atsheader["InputDivOn"]);
    this->atsheader_bin.bit_indicator = static_cast<int16_t>(this->atsheader["bit_indicator"]);
    strncpy(this->atsheader_bin.result_selftest, this->atsheader["result_selftest"].get<std::string>().c_str(), sizeof(this->atsheader_bin.result_selftest));
    this->atsheader_bin.numslices = static_cast<uint16_t>(this->atsheader["numslices"]);
    this->atsheader_bin.cal_freqs = static_cast<int16_t>(this->atsheader["cal_freqs"]);
    this->atsheader_bin.cal_entry_length = static_cast<int16_t>(this->atsheader["cal_entry_length"]);
    this->atsheader_bin.cal_version = static_cast<int16_t>(this->atsheader["cal_version"]);
    this->atsheader_bin.cal_start_address = static_cast<int16_t>(this->atsheader["cal_start_address"]);

    this->set_lf_filter_int(this->atsheader["LF_filters"]);

    strncpy(this->atsheader_bin.UTMZone, this->atsheader["UTMZone"].get<std::string>().c_str(), sizeof(this->atsheader_bin.UTMZone));
    this->atsheader_bin.system_cal_datetime = static_cast<uint32_t>(this->atsheader["system_cal_datetime"]);
    strncpy(this->atsheader_bin.sensor_cal_filename, this->atsheader["sensor_cal_filename"].get<std::string>().c_str(), sizeof(this->atsheader_bin.sensor_cal_filename));

    this->atsheader_bin.sensor_cal_datetime = static_cast<uint32_t>(this->atsheader["sensor_cal_datetime"]);
    this->atsheader_bin.powerline1 = static_cast<float>(this->atsheader["powerline1"]);
    this->atsheader_bin.powerline2 = static_cast<float>(this->atsheader["powerline2"]);

    this->set_hf_filter_int(this->atsheader["HF_filters"]);

    this->atsheader_bin.external_gain = static_cast<float>(this->atsheader["external_gain"]);
    strncpy(this->atsheader_bin.ADB_board_type, this->atsheader["ADB_board_type"].get<std::string>().c_str(), sizeof(this->atsheader_bin.ADB_board_type));

    if (conv_from_06) {
      std::string s = this->atsheader["Comments"];
      this->atsheader["Comments"] = "converted from ADU-06";
      if (s.size()) {
        this->atsheader["Comments"] += "; " + s;
      }
    }

    strncpy(this->atsheader_bin.comments.Client, this->atsheader["Client"].get<std::string>().c_str(), sizeof(this->atsheader_bin.comments.Client));
    strncpy(this->atsheader_bin.comments.Contractor, this->atsheader["Contractor"].get<std::string>().c_str(), sizeof(this->atsheader_bin.comments.Contractor));
    strncpy(this->atsheader_bin.comments.Area, this->atsheader["Area"].get<std::string>().c_str(), sizeof(this->atsheader_bin.comments.Area));
    strncpy(this->atsheader_bin.comments.SurveyID, this->atsheader["SurveyID"].get<std::string>().c_str(), sizeof(this->atsheader_bin.comments.SurveyID));
    strncpy(this->atsheader_bin.comments.Operator, this->atsheader["Operator"].get<std::string>().c_str(), sizeof(this->atsheader_bin.comments.Operator));
    strncpy(this->atsheader_bin.comments.SiteName, this->atsheader["SiteName"].get<std::string>().c_str(), sizeof(this->atsheader_bin.comments.SiteName));
    strncpy(this->atsheader_bin.comments.XmlHeader, this->atsheader["XmlHeader"].get<std::string>().c_str(), sizeof(this->atsheader_bin.comments.XmlHeader));
    strncpy(this->atsheader_bin.comments.Comments, this->atsheader["Comments"].get<std::string>().c_str(), sizeof(this->atsheader_bin.comments.Comments));
    strncpy(this->atsheader_bin.comments.SiteNameRR, this->atsheader["SiteNameRR"].get<std::string>().c_str(), sizeof(this->atsheader_bin.comments.SiteNameRR));
    strncpy(this->atsheader_bin.comments.SiteNameEMAP, this->atsheader["SiteNameEMAP"].get<std::string>().c_str(), sizeof(this->atsheader_bin.comments.SiteNameEMAP));
  }

}; // end of class atsfile
// *****************
// ***************************************** C O M P A R I S O N S *****************************************

/*!
 * @brief compare the channel types of two atsfile shared_ptrs; like both having "Ex" or "Ey"
 * @param lhs first shared_ptr<atsfile>
 * @param rhs second shared_ptr<atsfile>
 * @return true if channel types are equal, false otherwise
 * @example std::find_if(atsfiles.begin(), atsfiles.end(), compare_ats_channel_type(lhs, rhs));
 * @example std::sort(atsfiles.begin(), atsfiles.end(), compare_ats_channel_type); (alphabetical order by channel type)
 * @example std::copy_if(atsfiles.begin(), atsfiles.end(), std::back_inserter(filtered), compare_ats_channel_type(lhs, rhs)); will copy all atsfiles with the same channel type into filtered
 */
auto compare_ats_channel_type = [](const std::shared_ptr<atsfile> &lhs, const std::shared_ptr<atsfile> &rhs) -> bool {
  return (std::string(lhs->atsheader_bin.channel_type, sizeof(lhs->atsheader_bin.channel_type)) == std::string(rhs->atsheader_bin.channel_type, sizeof(rhs->atsheader_bin.channel_type)));
};

/*!
 * @brief compare position of the sensors in two atsfile shared_ptrs
 * @param lhs first shared_ptr<atsfile>
 * @param rhs second shared_ptr<atsfile>
 * @return true if positions are equal, false otherwise
 */
auto compare_ats_pos = [](const std::shared_ptr<atsfile> &lhs, const std::shared_ptr<atsfile> &rhs) -> bool {
  float limit = 0.05; // that 5 cm is the limit for comparison
  if (std::abs(lhs->atsheader_bin.x1 - rhs->atsheader_bin.x1) > limit)
    return false;
  if (std::abs(lhs->atsheader_bin.y1 - rhs->atsheader_bin.y1) > limit)
    return false;
  if (std::abs(lhs->atsheader_bin.z1 - rhs->atsheader_bin.z1) > limit)
    return false;
  if (std::abs(lhs->atsheader_bin.x2 - rhs->atsheader_bin.x2) > limit)
    return false;
  if (std::abs(lhs->atsheader_bin.y2 - rhs->atsheader_bin.y2) > limit)
    return false;
  if (std::abs(lhs->atsheader_bin.z2 - rhs->atsheader_bin.z2) > limit)
    return false;
  return true;
};

/*!
 * @brief compare the least significant bits (LSB) of two atsfile shared_ptrs. If same, we can easily concatenate the files
 * @param lhs first shared_ptr<atsfile>
 * @param rhs second shared_ptr<atsfile>
 * @return true if LSBs are equal, false otherwise
 */
auto compare_ats_lsb = [](const std::shared_ptr<atsfile> &lhs, const std::shared_ptr<atsfile> &rhs) -> bool {
  double lsb_lhs = lhs->atsheader["lsbval"];
  double lsb_rhs = rhs->atsheader["lsbval"];
  if (lsb_lhs < 0.0 || lsb_rhs < 0.0)
    return false;
  double d = lsb_lhs / lsb_rhs;
  if (d < 1.0)
    d = 1.0 / d;
  return d <= (1.0 + 1e-5);
};

/*!
 * @brief compare the sample rates of two atsfile shared_ptrs
 * @param lhs first shared_ptr<atsfile>
 * @param rhs second shared_ptr<atsfile>
 * @return true if sample rates are equal, false otherwise
 */
auto compare_ats_sample_rate = [](const std::shared_ptr<atsfile> &lhs, const std::shared_ptr<atsfile> &rhs) -> bool {
  return (lhs->atsheader_bin.sample_rate == rhs->atsheader_bin.sample_rate);
};

/*!
 * @brief compare the sensor types of two atsfile shared_ptrs, so for example both having "MFS-06e"?
 * @param lhs first shared_ptr<atsfile>
 * @param rhs second shared_ptr<atsfile>
 * @return true if sensor types are equal, false otherwise
 */
auto compare_ats_sensor_type = [](const std::shared_ptr<atsfile> &lhs, const std::shared_ptr<atsfile> &rhs) -> bool {
  return (std::string(lhs->atsheader_bin.sensor_type) == std::string(rhs->atsheader_bin.sensor_type));
};

/*!
 * @brief compare the serial numbers of two atsfile shared_ptrs; hence that will easily fail for Ex, Ey because the are manually set
 * @param lhs first shared_ptr<atsfile>
 * @param rhs second shared_ptr<atsfile>
 * @return true if serial numbers are equal, false otherwise
 */
auto compare_ats_serial_number = [](const std::shared_ptr<atsfile> &lhs, const std::shared_ptr<atsfile> &rhs) -> bool {
  return (lhs->atsheader_bin.serial_number == rhs->atsheader_bin.serial_number);
};

/*!
 * @brief compare the chopper settings of two atsfile shared_ptrs
 * @param lhs first shared_ptr<atsfile>
 * @param rhs second shared_ptr<atsfile>
 * @return true if chopper settings are equal, false otherwise
 */
auto compare_ats_chopper = [](const std::shared_ptr<atsfile> &lhs, const std::shared_ptr<atsfile> &rhs) -> bool {
  return (lhs->atsheader_bin.chopper == rhs->atsheader_bin.chopper);
};

/*!
 * @brief compare the start times of two atsfile shared_ptrs
 * @param lhs first shared_ptr<atsfile>
 * @param rhs second shared_ptr<atsfile>
 * @return true if start times are equal, false otherwise
 */
auto compare_ats_start = [](const std::shared_ptr<atsfile> &lhs, const std::shared_ptr<atsfile> &rhs) -> bool {
  return lhs->atsheader_bin.start < rhs->atsheader_bin.start;
};

/*!
 * @brief compare the similar properties of two atsfile shared_ptrs
 * @details This function checks if two atsfile shared_ptrs can be concatenated based on their properties.
 * position, sample rate, sensor type, chopper settings, and channel type must match.
 * the serial number may hae changed (replacement of a sensor), so we do not compare it
 * the LSB may have (slightly) changed after reboot, but concat will check that
 * BUT we can NOT change the position, because that is in the xml file.
 * @param lhs first shared_ptr<atsfile>
 * @param rhs second shared_ptr<atsfile>
 * @return true if the two atsfile shared_ptrs are similar enough to be concatenated, false otherwise
 */
auto compare_ats_similar = [](const std::shared_ptr<atsfile> &lhs, const std::shared_ptr<atsfile> &rhs) -> bool {
  return (compare_ats_pos(lhs, rhs) && compare_ats_sample_rate(lhs, rhs) &&
          compare_ats_sensor_type(lhs, rhs) &&
          compare_ats_chopper(lhs, rhs) && compare_ats_channel_type(lhs, rhs));
};

/*!
 * @brief compare the stop times of two atsfile shared_ptrs
 * @details This function compares the stop times of two atsfile shared_ptrs.
 * @return false if not equal, true if equal
 */
auto compare_ats_stop = [](const std::shared_ptr<atsfile> &lhs, const std::shared_ptr<atsfile> &rhs) -> bool {
  long double sf_lhs = static_cast<long double>(lhs->atsheader_bin.samples) / static_cast<long double>(lhs->atsheader_bin.sample_rate);
  long double sf_rhs = static_cast<long double>(rhs->atsheader_bin.samples) / static_cast<long double>(rhs->atsheader_bin.sample_rate);
  return ((static_cast<long double>(lhs->atsheader_bin.start)) + sf_lhs) != (static_cast<long double>(rhs->atsheader_bin.start) + sf_rhs);
};

/*!
 * @brief check if two atsfile shared_ptrs can be concatenated
 * @details This function checks if two atsfile shared_ptrs can be concatenated based on their properties.
 * It checks if the two files are similar enough to be concatenated, and if the stop time of the first file is before the start time of the second file.
 * The properties checked are:
 * - position, sample rate, sensor type, chopper settings, and channel type must match
 */
auto can_ats_concatenate = [](const std::shared_ptr<atsfile> &lhs, const std::shared_ptr<atsfile> &rhs) -> bool {
  if (!compare_ats_similar(lhs, rhs))
    return false;
  // check if the stop time of lhs is before the start time of rhs, use stop_secs_since_1970
  long double lhs_stop = lhs->ats_stop_secs_since_1970();
  long double rhs_start = rhs->ats_start_secs_since_1970();
  return lhs_stop < rhs_start;
};

/*!
 * @brief check if two atsfile shared_ptrs are identical
 */
auto identical_ats_files = [](const std::shared_ptr<atsfile> &lhs, const std::shared_ptr<atsfile> &rhs) -> bool {
  // we simply compare the atsheaders
  // we do not use the binary ATSHeader_80, because binary ATSHeader_80 may have different padding or "empty bytes" in the header
  // we have the samples in the header.
  return (lhs->atsheader == rhs->atsheader);
};

/*!
 * @brief calculate the difference in time between the stop time of the left-hand side and the start time of the right-hand side
 * @details we need long double; the "real" stop my not always be at a full second, so we calculate the difference in samples
 * @param lhs first shared_ptr<atsfile> (stop time)
 * @param rhs second shared_ptr<atsfile> (start time)
 */
auto diff_time_ats_stop_start = [](const std::shared_ptr<atsfile> &lhs, const std::shared_ptr<atsfile> &rhs) -> long double {
  long double sf_lhs = static_cast<long double>(lhs->atsheader_bin.samples) / static_cast<long double>(lhs->atsheader_bin.sample_rate);
  long double sf_rhs = static_cast<long double>(rhs->atsheader_bin.start);
  sf_lhs = (static_cast<long double>(lhs->atsheader_bin.start)) + sf_lhs;
  return int64_t((sf_rhs - sf_lhs) * static_cast<long double>(lhs->atsheader_bin.sample_rate));
};

/*!
 * @brief compares comparable values of the json header of an atsfile
 * @details This class is used to compare the values of the atsheader in a shared_ptr
 * @example std::find_if(atsfiles.begin(), atsfiles.end(), ats_Greater(100.0, "lsbval")); this will find the first atsfile with a lsbval greater
 * @example to find all atsfiles with a lsbval greater than 100.0, use std::copy_if. which returns a vector of shared_ptr<atsfile>
 */
class ats_Greater {
  double _than;
  std::string what;

public:
  ats_Greater(double th, std::string wh) :
      _than(th), what(wh) {
  }
  bool operator()(std::shared_ptr<atsfile> &rhs) const {
    return rhs->atsheader_bin.lsbval > _than;
  }
};

/*!
 * @brief compares comparable values of the json header of an atsfile
 * @details This class is used to compare the values of the atsheader in a shared_ptr
 * @example std::find_if(atsfiles.begin(), atsfiles.end(), ats_Less(100.0, "lsbval"));
 */
class ats_Less {
  double _than;
  std::string what;

public:
  ats_Less(double th, std::string wh) :
      _than(th), what(wh) {
  }
  bool operator()(std::shared_ptr<atsfile> &rhs) const {
    return rhs->atsheader_bin.lsbval < _than;
  }
};

// we sort a vector of atsfile shared_ptrs by available_channel_types (defined in base_constants.h)
void sort_atsfiles_by_channel_type(std::vector<std::shared_ptr<atsfile>> &atsfiles) {
  // use available_channel_types from base_constants.h as ORDER; so we start with Ex
  // the put all Ex into the front, then all Ey, and do on until the end of available_channel_types is reached
  std::sort(atsfiles.begin(), atsfiles.end(), [](const std::shared_ptr<atsfile> &lhs, const std::shared_ptr<atsfile> &rhs) {
    auto it_lhs = std::find(available_channel_types.begin(), available_channel_types.end(), lhs->atsheader_bin.channel_type);
    auto it_rhs = std::find(available_channel_types.begin(), available_channel_types.end(), rhs->atsheader_bin.channel_type);
    if (it_lhs != available_channel_types.end() && it_rhs != available_channel_types.end()) {
      return std::distance(available_channel_types.begin(), it_lhs) < std::distance(available_channel_types.begin(), it_rhs);
    }
    return false; // if not found, do not change the order
  });
}
#endif // ATSFILE_HPP

/*
https://www.codingame.com/playgrounds/5659/c17-filesystem

cout << "exists() = " << fs::exists(pathToShow) << "\n"
     << "root_name() = " << pathToShow.root_name() << "\n"
     << "root_path() = " << pathToShow.root_path() << "\n"
     << "relative_path() = " << pathToShow.relative_path() << "\n"
     << "parent_path() = " << pathToShow.parent_path() << "\n"
     << "filename() = " << pathToShow.filename().string() << "\n"
     << "stem() = " << pathToShow.stem() << "\n"
     << "extension() = " << pathToShow.extension() << "\n";

Here's an output for a file path like "C:\Windows\system.ini":

    exists() = 1
    root_name() = C:
    root_path() = C:\
    relative_path() = Windows\system.ini
    parent_path() = C:\Windows
    filename() = system.ini
    stem() = system
    extension() = .ini

     int i = 0;
for (const auto& part : pathToShow)
    cout << "path part: " << i++ << " = " << part << "\n";
*/