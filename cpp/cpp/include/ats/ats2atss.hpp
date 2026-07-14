#ifndef ATS2ATSS_HPP
#define ATS2ATSS_HPP

#include "BS_thread_pool.hpp"
#include "atsfile.hpp"
#include "cal/read_cal/read_cal_xml.h"
#include "cal_base.hpp"
#include "channel.hpp"
#include "strings_etc.hpp"
#include "survey_tree.hpp"
#include <filesystem>
#include <iostream>
#include <memory>
#include <regex>
#include <shared_mutex>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

class ats2atss {

public:
  ats2atss() = default;
  ats2atss(const std::filesystem::path &ats_filename, const std::shared_ptr<channel> &chan) :
      ats_file(std::make_shared<atsfile>(ats_filename)), chan(chan) {
    this->ats_file = std::make_shared<atsfile>(ats_filename);
    this->ats_file->read_atsheader();
    this->can_convert_data = true; // we can convert data
  }
  ats2atss(const std::filesystem::path &ats_filename, const int64_t &shift_start_time = 0) :
      ats_file(std::make_shared<atsfile>(ats_filename)) {
    this->ats_file->read_atsheader();
    this->chan = std::make_shared<channel>();
    this->chan->from_ats(ats_file->atsheader["channel_type"], ats_file->get_sample_rate(), ats_file->ats_start_secs_since_1970() + shift_start_time, 0.0);
    // same as set_base_file
    this->chan->set_serial(ats_file->atsheader["serial_number"].get<size_t>());
    this->chan->set_system(ats_file->atsheader["SystemType"].get<std::string>());
    this->chan->set_channel_no(ats_file->atsheader["channel_number"].get<size_t>());
    chan->set_lat_lon_elev(ats_file->get_lat(), ats_file->get_lon(), ats_file->get_elev());
    chan->angle = ats_file->pos2angle();
    chan->tilt = ats_file->pos2tilt();
    chan->resistance = ats_file->atsheader["rho_probe_ohm"]; // contact resistance
    chan->filter = ats_file->get_ats_filter("ADB-");
    chan->set_filter(true); // check the filter settings in a heuristic way; e.g. 500Hz HP will be added for ADU-08e, HF board
    this->measdocxml = ats_file->get_xmlpath();
  }
  std::shared_ptr<channel> convert_header() {
    std::vector<std::shared_ptr<calibration>> cals; ///< vector of calibration objects, normally 3 which are in the measdoc.xml file
    auto rcal = std::make_shared<read_cal>();
    auto cal = std::make_shared<calibration>();
    cal->set_format(CalibrationType::mtx, false);
    cal->units_amplitude = "mV"; // may be changed later when try to scale in case E is assumed
    cal->sensor = rcal->get_sensor_name(ats_file->atsheader["sensor_type"]);
    if (cal->sensor == "")
      cal->sensor = ats_file->atsheader["sensor_type"];
    cal->serial = ats_file->atsheader["sensor_serial_number"];
    cal->chopper = ats_file->get_ats_chopper();
    std::string ssh = std::to_string(ats_file->atsheader["channel_number"].get<size_t>());

    // read XML calibration xmlcal
    std::string messages;
    try {
      cals = rcal->read_std_xml(ats_file->get_xmlpath(), messages);
    } catch (const std::runtime_error &error) {
      std::cerr << error.what() << std::endl;
      std::cerr << "--> continued" << std::endl;
      cals.clear();
    }
    // make sure to have correct sensor names
    for (auto &ncal : cals) {
      ncal->sensor = rcal->get_sensor_name(ncal->sensor);
    }
    double lsb = ats_file->atsheader["lsbval"];
    if (ats_file->can_and_want_scale()) {
      lsb *= 1000. / ats_file->pos2length();
      chan->units = "mV/km";
      chan->tmp_lsb = lsb;
    } else {
      chan->tmp_lsb = lsb;
      chan->units = "mV";
    } // no scaling
    bool has_cal = false;
    for (auto &ncal : cals) {
      if (compare_sensor_and_chopper(cal, ncal)) {
        ncal->old_to_newformat();
        chan->set_cal(ncal);
        has_cal = true;
        auto all_messages = mstr::split(messages, ';');
        for (const auto &str : all_messages) {
          if (mstr::contains(str, cal->sensor, false) && mstr::contains(str, cal->chopper2string(), false) && mstr::contains(str, cal->serial2string(), false) && mstr::contains(str, ssh, false)) {
            std::cout << str << std::endl;
            break;
          }
        }
      }
    }
    if (!has_cal) {
      chan->set_cal(cal);
      auto all_messages = mstr::split(messages, ';');
      for (const auto &str : all_messages) {
        if (mstr::contains(str, cal->sensor, false) && mstr::contains(str, ssh, false)) {
          std::cout << str << std::endl;
          break;
        }
      }
    }
    // Extract station name safely from path hierarchy
    auto path = ats_file->get_ats_path();
    if (path.has_parent_path() && path.parent_path().has_parent_path()) {
      this->chan->tmp_station = path.parent_path().parent_path().filename().string();
      this->chan->tmp_origin = path.parent_path();
    } else {
      // Fallback if path hierarchy is insufficient
      this->chan->tmp_station = "unknown_station";
      this->chan->tmp_origin = path.has_parent_path() ? path.parent_path() : path;
    }
    this->chan->tmp_xml = ats_file->get_xmlpath();
    return this->chan;
  }

  void set_can_convert_data(const bool can_convert) {
    this->can_convert_data = can_convert;
  }

  void convert_data(const bool no_e = false) {
    if (!this->can_convert_data) {
      std::cerr << "Cannot convert data, no files available." << std::endl;
      return;
    }
    if (no_e && chan->is_type("E")) {
      return; // skip E channels if no_e is true and channel is E type
    }
    // now read the data, convert to double and scale E in case
    size_t samples = static_cast<uint64_t>(ats_file->atsheader_bin.samples);
    double lsb = this->chan->tmp_lsb; // lsb value from ats header
    if (lsb == 0.0) {
      throw std::runtime_error("No lsb value found in ATS header, cannot convert data.");
    }
    size_t chunk_size = 524288;
    std::vector<int32_t> ints;
    std::vector<double> dbls;
    if (samples < chunk_size) {
      dbls.resize(samples);
      ints.resize(samples);
    } else {
      dbls.resize(chunk_size);
      ints.resize(chunk_size);
    }

    size_t samples_read = 0;
    bool check = false;

    try {
      this->ats_file->prepare_read_data(); // opens the file, skips the header and seeks to the start of the data
      do {
        if (!samples_read) {
          std::cout << "converted " << chan->get_atss_filepath() << " old LSB : " << lsb << std::endl;
        }
        dbls.resize(ats_file->ats_read_int_doubles(ints));
        // break if 0 is returned, that we have if EOF reached
        if (dbls.empty()) {
          // std::cout << "No more data to read, samples_read: " << samples_read << std::endl;
          break; // no more data to read
        }
        samples_read += dbls.size();
        std::transform(ints.begin(), ints.end(), dbls.begin(), [lsb](double v) { return (lsb * v); });
        if (check) {
          check = false;
          for (size_t nx = 0; nx < 5; nx++) {
            std::cout << ints[nx] << " <-> " << dbls[nx] << " " << "lsb:" << lsb << std::endl;
          }
        }

        chan->write_data(dbls);
      } while (dbls.size() && chan->outfile_is_good());
      chan->close_outfile();
      auto outfilename = chan->get_atss_filepath();
      size_t outfilesize = std::filesystem::file_size(outfilename);
      std::cout << chan->filename(".json") << "  " << samples_read << " <-> " << chan->samples() << std::endl;
    } catch (const std::runtime_error &error) {
      std::cerr << error.what() << std::endl;
      return;
    }
  }
  ~ats2atss() = default;

private:
  std::shared_ptr<channel> chan;     ///< channel object to write atss data (new ATSS files)
  std::shared_ptr<atsfile> ats_file; ///< atsfile object to read ATS data (old ATS files)
  std::filesystem::path out_dir;     ///< output directory for atss files
  std::filesystem::path measdocxml;  ///< contains the calibration data (old cal, normalized)
  bool can_convert_data = false;     ///< can we convert the data? (e.g. if no calibration is available, we cannot convert)
  const bool verbose = false;        ///< verbose output
};

// classes for iterating for iterating over OLD survey directories
// we do a simple iteration over the directories, not a tree.

/// @brief scans a measdoc.xml file for channels; all channels in xml file ARE A RUN!
/// @details scan converts on the fly; we keep both files. After writing the headers, we convert the data.
class ats_run_ats {
public:
  ats_run_ats(const std::filesystem::path &measdocxml) :
      measdocxml(measdocxml) {
    if (!std::filesystem::exists(this->measdocxml)) {
      throw std::runtime_error("measdocxml path does not exist: " + this->measdocxml.string());
    }
    this->measdocxml = std::filesystem::canonical(this->measdocxml);
    if (!std::filesystem::is_regular_file(this->measdocxml)) {
      throw std::runtime_error("measdocxml path is not a regular file: " + this->measdocxml.string());
    }
  }
  std::vector<std::filesystem::path> extract_ats_files() {
    std::shared_lock lock(mutex_);
    std::vector<std::filesystem::path> files;
    std::ifstream in(this->measdocxml);
    std::string line;
    if (!in.is_open()) {
      throw std::runtime_error("Failed to open measdoc.xml: " + this->measdocxml.string());
    }
    // this regex matches the <ats_data_file>...</ats_data_file> tags, ([^<]+) means any characters except '<'
    std::regex re("<ats_data_file>([^<]+)</ats_data_file>");
    while (std::getline(in, line)) {
      std::smatch match;
      if (std::regex_search(line, match, re)) {
        std::string ats_file = match[1].str();
        files.push_back(this->measdocxml.parent_path() / ats_file);
        if (files.size() > max_runs) {
          std::cerr << "Warning: More than " << max_runs << " ATS files found in measdoc.xml, only the first " << max_runs << " will be processed." << std::endl;
          break; // limit to max_runs
        }
      }
      // stop at </recording>
      if (line.find("</recording>") != std::string::npos) {
        break;
      }
    }
    return files;
  }
  void scan() {
    std::shared_lock lock(mutex_);
    channels.clear();        // clear the old files
    this->ats_files.clear(); // clear the old files
    auto ats_files_tmp = this->extract_ats_files();
    if (ats_files_tmp.empty()) {
      std::cerr << "No ATS files found in measdoc.xml: " << this->measdocxml << std::endl;
      return; // no ATS files, nothing to do
    }
    for (const auto &ats_file : ats_files_tmp) {
      auto ats_conv = std::make_shared<ats2atss>(ats_file);
      auto chan = ats_conv->convert_header();
      if (chan == nullptr) {
        std::cerr << "Failed to convert ATS file: " << ats_file << std::endl;
        continue; // skip this file
      } else {
        ats_files.push_back(ats_file);
        channels.push_back(chan);
      }
      // read the measdoc.xml file and create channels
    }
  }

  void ls() const {
    std::shared_lock lock(mutex_);
    for (const auto &chan : channels) {
      if (chan) {
        std::cout << "Channel: " << chan->get_channel_no() << ": " << chan->get_channel_type() << " " << chan->get_sample_rate_str(true) << "\n";
      }
    }
  }

  void set_output_dir(const std::filesystem::path &out_dir) {
    std::shared_lock lock(mutex_);

    if (!std::filesystem::exists(out_dir)) {
      throw std::runtime_error("Output directory does not exist: " + out_dir.string());
    }
    for (auto &chan : channels) {
      if (chan) {
        chan->set_dir(out_dir);
      }
    }
  }

  void write_headers(const bool no_e = false) const {
    std::shared_lock lock(mutex_);
    for (const auto &chan : channels) {
      if (chan) {
        if (no_e && chan->is_type("E")) {
          continue; // skip E channels if no_e is true and channel is E type
        }
        chan->write_header();
      }
    }
  }

  // ok, we try to fire up the conversion into a thread pool
  void convert_data(const std::shared_ptr<BS::thread_pool<BS::tp::none>> &pool, const bool no_e = false) {
    if (channels.empty()) {
      std::cerr << "No channels to convert data for." << std::endl;
      return;
    }
    if (ats_files.empty()) {
      std::cerr << "No ATS files to convert data for." << std::endl;
      return;
    }
    if (channels.size() != ats_files.size()) {
      std::cerr << "Mismatch between number of channels and ATS files." << std::endl;
      return;
    }
    if (!pool) {
      std::cerr << "No thread pool provided for data conversion." << std::endl;
      return;
    }
    // create the conversion class for each channel and submit to the thread pool
    std::vector<std::shared_ptr<ats2atss>> ats_convs;
    size_t idx = 0;
    for (auto &chan : channels) {
      if (chan) {
        if (no_e && chan->is_type("E")) {
          continue; // skip E channels if no_e is true and channel is E type
        }
        auto ats_conv = std::make_shared<ats2atss>(ats_files[idx], chan);
        ats_convs.push_back(ats_conv);
      }
      ++idx;
    }
    // now we should be able to detach the convert_data method to the thread pool
    for (const auto &ats_conv : ats_convs) {
      if (ats_conv) {
        pool->detach_task([ats_conv]() {
          ats_conv->convert_data();
        });
      } else {
        std::cerr << "Failed to create ats2atss object for conversion." << std::endl;
      }
    }
  }

  std::filesystem::path measdocxml;               ///< /survey/Northern_Mining/ts/Sarıçam/meas_2009-08-20_13-22-00/084_2009-08-20_13-22-00_2009-08-21_07-00-00_R001_128H.xml
  std::vector<std::shared_ptr<channel>> channels; ///< vector of channels, one for each ats file
  std::vector<std::filesystem::path> ats_files;   ///< vector of ats files, one for each channel, we keep for data conversion
  mutable std::shared_mutex mutex_;               ///< mutex for thread-safe access to ats_files
  const bool verbose = false;                     ///< verbose output
};

class station_ats {
public:
  station_ats(const std::filesystem::path &basePath) :
      basePath(basePath) {
    if (!std::filesystem::exists(this->basePath)) {
      throw std::runtime_error("station path does not exist: " + this->basePath.string());
    }
    this->basePath = std::filesystem::canonical(this->basePath);
    if (!std::filesystem::is_directory(this->basePath)) {
      throw std::runtime_error("station path is not a directory: " + this->basePath.string());
    }
  }

  void scan() {
    std::shared_lock lock(mutex_);
    for (const auto &entry : std::filesystem::directory_iterator(basePath)) {
      if (entry.is_directory()) {
        // enter the directory and look for *xml files; a run is described by a measdoc.xml file
        std::vector<std::filesystem::path> xml_files;
        for (const auto &file : std::filesystem::directory_iterator(entry.path())) {
          if (file.is_regular_file() && file.path().extension() == ".xml") {
            xml_files.push_back(file.path());
          }
        }
        if (xml_files.empty()) {
          //  std::cerr << "No measdoc.xml found in " << entry.path() << std::endl;
          continue; // no measdoc.xml, no run
        }
        // we have one or more measdoc.xml files, so we can create run objects
        // std::cout << "Found xml files in " << entry.path() << std::endl;
        for (const auto &xml_file : xml_files) {
          auto run = std::make_shared<ats_run_ats>(xml_file);
          runs[xml_file] = run; // store the run in the map
          run->scan();          // scan the run for channels
        }
        std::cout << "Found " << xml_files.size() << " measdoc.xml files in " << entry.path() << std::endl;
      }
    }
  }

  void ls() const {
    std::shared_lock lock(mutex_);
    for (const auto &run : runs) {
      std::cout << "Run: " << run.first.filename() << "\n";
      run.second->ls(); // list the channels in the run
    }
  }
  std::filesystem::path basePath; ///< base path for the survey, e.g. /survey/Northern_Mining/ts/Sample_Station
  std::map<std::filesystem::path, std::shared_ptr<ats_run_ats>> runs;
  mutable std::shared_mutex mutex_;
  const bool verbose = false; ///< verbose output
};
// "/survey/Northern_Mining/"

class survey_ats {
public:
  survey_ats(const std::filesystem::path &basePath) :
      basePath(basePath) {
    if (!std::filesystem::exists(basePath)) {
      throw std::runtime_error("survey path does not exist: " + basePath.string());
    }
    this->basePath = std::filesystem::canonical(this->basePath);
    if (!std::filesystem::is_directory(this->basePath)) {
      throw std::runtime_error("survey path is not a directory: " + this->basePath.string());
    }
    this->basePath = this->basePath / "ts";
    if (!std::filesystem::exists(this->basePath)) {
      throw std::runtime_error("survey path does not contain 'ts' directory: " + this->basePath.string());
    }
    if (!std::filesystem::is_directory(this->basePath)) {
      throw std::runtime_error("survey path 'ts' is not a directory: " + this->basePath.string());
    }
  }

  void scan() {
    std::shared_lock lock(mutex_);
    for (const auto &entry : std::filesystem::directory_iterator(basePath)) {
      if (entry.is_directory()) {
        auto station = std::make_shared<station_ats>(entry.path());
        stations[entry.path().filename().string()] = station;
        station->scan(); // scan the station for runs
      }
    }
  }

  void ls() const {
    std::shared_lock lock(mutex_);
    for (const auto &[station_name, station] : stations) {
      std::cout << "Station: " << station_name << " in " << station->basePath << "\n";
      station->ls(); // list runs in the station
    }
  }

  std::filesystem::path basePath;                               ///< base path for the survey, e.g. /survey/Northern_Mining/ts
  std::map<std::string, std::shared_ptr<station_ats>> stations; ///< map of station names to station objects
  mutable std::shared_mutex mutex_;
  const bool verbose = false; ///< verbose output
};

class command_line_options_ats2atss {
public:
  command_line_options_ats2atss(int argc, char **argv) {
    std::string myexecutable(argv[0]);
    unsigned l = 1;
    while (argc > 1 && (l < unsigned(argc))) {
      std::string marg(argv[l]);
      if (marg == "-s") {
        if (++l < unsigned(argc)) {
          survey_path_ats = argv[l];
        }
      } else if (marg == "-u") {
        if (++l < unsigned(argc)) {
          survey_tree_path = argv[l];
        }
      } else if (marg == "-i") {
        if (++l < unsigned(argc)) {
          in_directory = argv[l];
        }
      } else if (marg == "-o") {
        if (++l < unsigned(argc)) {
          out_directory = argv[l];
        }
      } else if (marg == "-v") {
        verbose = true;
      } else if (marg == "-no_e") {
        no_e = true; // disable E channel processing
      } else if (marg == "-h" || marg == "--help") {
        help = true;
      } else if (marg == "-r") {
        if (++l < unsigned(argc)) {
          try {
            start_run = std::stoul(argv[l]);
          } catch (const std::invalid_argument &) {
            std::cerr << "Invalid run number: " << argv[l] << std::endl;
          }
          // 0 does nothing;
          // 1 is anyhow the default, so we do not need to set it
        }
      } else {
        stations.push_back(marg); // collect station names
      }
      ++l; // move to the next argument
    }
    // Check that we have either survey paths OR directory paths, but not both
    bool has_survey_paths = !survey_path_ats.empty() && !survey_tree_path.empty();
    bool has_directory_paths = !in_directory.empty() && !out_directory.empty();

    if (!has_survey_paths && !has_directory_paths) {
      std::cerr << "Error: You must provide either survey paths (-s and -u) OR directory paths (-i and -o)." << std::endl;
      std::cerr << "Use -s <survey_path> and -u <survey_tree_path> for survey processing," << std::endl;
      std::cerr << "OR use -i <in_directory> and -o <out_directory> for simple directory conversion." << std::endl;
      help = true;
    }

    if (has_survey_paths && has_directory_paths) {
      std::cerr << "Error: Cannot use both survey paths (-s, -u) and directory paths (-i, -o) simultaneously." << std::endl;
      std::cerr << "Choose either survey processing or simple directory conversion." << std::endl;
      help = true;
    }

    if (has_survey_paths) {
      if (survey_path_ats.empty()) {
        std::cerr << "Error: No survey path provided. Use -s <survey_path> to specify the input ATS survey path." << std::endl;
        help = true;
      }
      if (survey_tree_path.empty()) {
        std::cerr << "Error: No survey tree path provided. Use -u <survey_tree_path> to specify the output survey tree path." << std::endl;
        std::cerr << "The output survey tree path will be CREATED if it does not exist." << std::endl;
        help = true;
      }
    }

    if (has_directory_paths) {
      if (in_directory.empty()) {
        std::cerr << "Error: No input directory provided. Use -i <in_directory> to specify the input directory." << std::endl;
        help = true;
      }
      if (out_directory.empty()) {
        std::cerr << "Error: No output directory provided. Use -o <out_directory> to specify the output directory." << std::endl;
        help = true;
      }
    }
    if (help) {
      std::cout << "Usage: " << myexecutable << " -s <survey_path> -u <survey_tree_path> [-v] [-e] [-h] [-r <start_run>] [station_names...]\n";
      std::cout << "   OR: " << myexecutable << " -i <in_directory> -o <out_directory> [-v] [-e] [-h]\n\n";
      std::cout << "Options:\n";
      std::cout << "  -s <survey_path>       Path to the input ATS survey directory.\n";
      std::cout << "  -u <survey_tree_path>  Path to the output survey tree directory.\n";
      std::cout << " --- or for a single directory ----:\n";
      std::cout << "  -i <in_directory> -o <out_directory>\n";
      std::cout << "  -v                     Enable verbose output.\n";
      std::cout << "  -no_e                  Disable E channel processing.\n";
      std::cout << "  -h, --help             Show this help message and exit.\n";
      std::cout << "  -r <start_run>         Start run number for processing output (default: 1).\n";
      std::cout << "  station_names...       List of station names to process.\n\n";
      std::cout << "Example: " << myexecutable << " -s /old_data/Northern_Mining/ -u /survey/Northern_Mining/ converts all\n";
      std::cout << "         with -no_e you can disable the E channel processing, for example parallel test with no E-Field connected.\n";
      std::cout << "         last arguments are station names, if not given, all stations will be processed.\n";
      std::cout << "         if you want to process only a subset of stations, you can specify them as arguments.\n";
      std::cout << "Example: " << myexecutable << " -s /old_data/Northern_Mining/ -u /survey/Northern_Mining/ Sariçam \n";
      std::cout << " -r 5 will start with run number 5 in the OUTPUT, default is 1.\n";
      std::cout << "  this can be the case if you already have a populated survey; you create a temporary survey tree, and then you can add the runs to the existing survey tree manually.\n";
      std::exit(0);
    }
    // if (survey_path_ats.empty() || survey_tree_path.empty()) {
    //   std::cerr << "Error: Missing required paths.\n";
    //   std::exit(0);
    // }
  }
  std::string survey_path_ats;       ///< Path to the input survey directory
  std::string survey_tree_path;      ///< Path to the output survey tree directory
  std::string in_directory;          ///< Input directory path for a quick and dirty conversion, together with out_directory
  std::string out_directory;         ///< Output directory path for a quick and dirty conversion, together with in_directory
  bool verbose = false;              ///< Enable verbose output
  bool no_e = false;                 ///< Disable E channel processing
  bool help = false;                 ///< Show help message
  std::vector<std::string> stations; ///< List of input stations to process
  size_t start_run = 0;              ///< Start run number for processing, 0 indicates no specific start run
};

#endif // ATS2ATSS_HPP