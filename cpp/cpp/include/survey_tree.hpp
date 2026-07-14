#ifndef survey_tree_d_HPP
#define survey_tree_d_HPP

#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <regex>
#include <shared_mutex>
#include <string>
#include <type_traits>
#include <vector>

// my classes
#include "BS_thread_pool.hpp"
#include "channel.hpp"
#include "mt_base.hpp"
#include "raw_spectra.hpp"
#include "strings_etc.hpp"
#include "survey_tree_cmdline.hpp"
#include "xlogger.hpp"
namespace fs = std::filesystem;
/// @file survey_tree_d.hpp
/// the survey class is a tree structure with stations, runs and channels.
/// The survey class is a singleton, so there is only one instance of the survey class.
///
/// @verbatim
/// └── Northern_Mining
///    ├── config
///    ├── db
///    ├── dump
///    ├── edi
///    ├── filters
///    ├── jle
///    ├── jobs
///    ├── log
///    ├── meta
///    │   ├── Kocatepe
///    │   │   ├── run_001
///    │   │   │   ├── 085_2009-08-20_13-22-00_2009-08-21_07-00-00_R001_128H.xml
///    │   │   │   ├── 085_ADU-07e_C000_TEx_128Hz.json
///    │   │   │   ├── 085_ADU-07e_C001_TEy_128Hz.json
///    │   │   │   ├── 085_ADU-07e_C002_THx_128Hz.json
///    │   │   │   ├── 085_ADU-07e_C003_THy_128Hz.json
///    │   │   │   └── 085_ADU-07e_C004_THz_128Hz.json
///    │   │   └── run_002
///    │   │       ├── 085_2009-08-20_13-22-01_2009-08-21_06-59-59_R001_32H.xml
///    │   │       ├── 085_ADU-07e_C000_TEx_32Hz.json
///    │   │       ├── 085_ADU-07e_C001_TEy_32Hz.json
///    │   │       ├── 085_ADU-07e_C002_THx_32Hz.json
///    │   │       ├── 085_ADU-07e_C003_THy_32Hz.json
///    │   │       └── 085_ADU-07e_C004_THz_32Hz.json
///    │   └── Sarıçam
///    │       ├── run_001
///    │       │   ├── 084_2009-08-20_13-22-00_2009-08-21_07-00-00_R001_128H.xml
///    │       │   ├── 084_ADU-07e_C000_TEx_128Hz.json
///    │       │   ├── 084_ADU-07e_C001_TEy_128Hz.json
///    │       │   ├── 084_ADU-07e_C002_THx_128Hz.json
///    │       │   ├── 084_ADU-07e_C003_THy_128Hz.json
///    │       │   └── 084_ADU-07e_C004_THz_128Hz.json
///    │       └── run_002
///    │           ├── 084_2009-08-20_13-22-01_2009-08-21_06-59-59_R001_32H.xml
///    │           ├── 084_ADU-07e_C000_TEx_32Hz.json
///    │           ├── 084_ADU-07e_C001_TEy_32Hz.json
///    │           ├── 084_ADU-07e_C002_THx_32Hz.json
///    │           ├── 084_ADU-07e_C003_THy_32Hz.json
///    │           └── 084_ADU-07e_C004_THz_32Hz.json
///    ├── processings
///    ├── reports
///    ├── shell
///    │   ├── mkallproc.sh
///    │   ├── plot_ascii_table_edi.sh
///    │   └── procall.sh
///    ├── stations
///    │   ├── Kocatepe
///    │   │   ├── run_001
///    │   │   │   ├── 085_ADU-07e_C000_TEx_128Hz.atss
///    │   │   │   ├── 085_ADU-07e_C000_TEx_128Hz.json
///    │   │   │   ├── 085_ADU-07e_C001_TEy_128Hz.atss
///    │   │   │   ├── 085_ADU-07e_C001_TEy_128Hz.json
///    │   │   │   ├── 085_ADU-07e_C002_THx_128Hz.atss
///    │   │   │   ├── 085_ADU-07e_C002_THx_128Hz.json
///    │   │   │   ├── 085_ADU-07e_C003_THy_128Hz.atss
///    │   │   │   ├── 085_ADU-07e_C003_THy_128Hz.json
///    │   │   │   ├── 085_ADU-07e_C004_THz_128Hz.atss
///    │   │   │   └── 085_ADU-07e_C004_THz_128Hz.json
///    │   │   └── run_002
///    │   │       ├── 085_ADU-07e_C000_TEx_32Hz.atss
///    │   │       ├── 085_ADU-07e_C000_TEx_32Hz.json
///    │   │       ├── 085_ADU-07e_C001_TEy_32Hz.atss
///    │   │       ├── 085_ADU-07e_C001_TEy_32Hz.json
///    │   │       ├── 085_ADU-07e_C002_THx_32Hz.atss
///    │   │       ├── 085_ADU-07e_C002_THx_32Hz.json
///    │   │       ├── 085_ADU-07e_C003_THy_32Hz.atss
///    │   │       ├── 085_ADU-07e_C003_THy_32Hz.json
///    │   │       ├── 085_ADU-07e_C004_THz_32Hz.atss
///    │   │       └── 085_ADU-07e_C004_THz_32Hz.json
///    │   └── Sarıçam
///    │       ├── run_001
///    │       │   ├── 084_ADU-07e_C000_TEx_128Hz.atss
///    │       │   ├── 084_ADU-07e_C000_TEx_128Hz.json
///    │       │   ├── 084_ADU-07e_C001_TEy_128Hz.atss
///    │       │   ├── 084_ADU-07e_C001_TEy_128Hz.json
///    │       │   ├── 084_ADU-07e_C002_THx_128Hz.atss
///    │       │   ├── 084_ADU-07e_C002_THx_128Hz.json
///    │       │   ├── 084_ADU-07e_C003_THy_128Hz.atss
///    │       │   ├── 084_ADU-07e_C003_THy_128Hz.json
///    │       │   ├── 084_ADU-07e_C004_THz_128Hz.atss
///    │       │   └── 084_ADU-07e_C004_THz_128Hz.json
///    │       └── run_002
///    │           ├── 084_ADU-07e_C000_TEx_32Hz.atss
///    │           ├── 084_ADU-07e_C000_TEx_32Hz.json
///    │           ├── 084_ADU-07e_C001_TEy_32Hz.atss
///    │           ├── 084_ADU-07e_C001_TEy_32Hz.json
///    │           ├── 084_ADU-07e_C002_THx_32Hz.atss
///    │           ├── 084_ADU-07e_C002_THx_32Hz.json
///    │           ├── 084_ADU-07e_C003_THy_32Hz.atss
///    │           ├── 084_ADU-07e_C003_THy_32Hz.json
///    │           ├── 084_ADU-07e_C004_THz_32Hz.atss
///    │           └── 084_ADU-07e_C004_THz_32Hz.json
///    └── tmp
/// @endverbatim

// Scan for channels in the run directory
/// @brief Maximum number of channels; each channel is at a fixed position! As it comes from the data logger
/// @details we have several times "Ex" for example, so we have to use the channel number to identify the channel.
/// @details refer to the run_d class documentation for more details.
// forward declaration of tree class
class survey_tree_d;
// declaration of NodeType
enum class NodeType {
  experiment = 0, ///< this is only for HDF5 experiment nodes! NOT always present on filesystem!
  survey = 1,     ///< survey level node, directly to be opened on a filesystem
  station = 2,    ///< station level node, a directory or HDF5 group below survey
  run = 3         ///< run level node, a directory or HDF5 group below station
};

/// @brief virtual base class for run_d (filesystem based) and run_hdf5 (HDF5 based)
class run_d {
public:
  virtual ~run_d() = default;

  /// @brief Constructor that initializes the run_d instance by scanning the specified directory.
  /// @param path The filesystem path to the run directory containing channel JSON files.
  /// @param parent_ptr Parent survey_tree_d node (will be stored as weak_ptr internally)
  /// @note The directory must exist or be created before using this constructor.
  /// @details since we we have many shared pointers, you DON'T want to implement a copy constructor or assignment operator!
  /// @details run_d likely will be managed by shared pointers only!
  explicit run_d(const fs::path &path, std::shared_ptr<survey_tree_d> parent_ptr = nullptr, std::shared_ptr<BS::thread_pool<BS::tp::none>> pool_ptr = nullptr) {
    basePath = path;
    parent = parent_ptr;
    pool = pool_ptr;
    // directory must exist or created before using it
    if (!fs::exists(basePath) || !fs::is_directory(basePath)) {
      std::cerr << "Directory does not exist: " << basePath << "\n";
      throw std::runtime_error("Directory does not exist: " + basePath.string());
    }
    // this->scan(channel_numbers);
    // this->logger << "run_d: Scanned " << basePath << ", found " << this->size() << " channels.\n";
    this->number = std::regex_search(basePath.string(), std::regex(R"(run_(\d+))")) ? std::stoul(std::regex_replace(basePath.string(), std::regex(R"(.*run_(\d+).*)"), "$1")) : SIZE_MAX;
  }

  size_t scan(const std::optional<std::vector<bool>> &channel_flags = std::nullopt) {
    std::unique_lock lock(mutex_);
    // this is the only clean way; a channel may have been removed or changed
    for (auto &channel_ptr : channels) {
      channel_ptr.reset();
    }

    size_t count = 0;
    for (const auto &entry : fs::directory_iterator(basePath)) {
      if (entry.is_regular_file() && entry.path().filename().string().ends_with(".json")) {
        size_t channel_no = mstr::channel_number_from_channel_file(entry.path());
        if (channel_no != SIZE_MAX && channel_no < max_survey_channels) {
          // If selective loading is requested, skip channels that are not flagged
          if (channel_flags) {
            if (channel_no >= channel_flags->size()) {
              throw std::runtime_error("Channel flag vector too small for detected channel number");
            }
            if (!channel_flags->at(channel_no)) {
              continue;
            }
          }
          channels[channel_no] = std::make_shared<channel>(entry.path());
          count++;
        }
      }
    }
    return count;
  }

  // size_t scan(const std::vector<bool> &channel_flags) {
  //   return scan(std::optional<std::vector<bool>>(channel_flags));
  // }

  /// @brief sets the parent pointer, e.g the survey_tree_d node run !
  /// @param parent_ptr
  void set_parent(std::shared_ptr<survey_tree_d> parent_ptr) {
    std::unique_lock lock(mutex_);
    parent = parent_ptr;
  }
  void set_basepath(const fs::path &path) {
    std::unique_lock lock(mutex_);
    basePath = path;
  }

  /// @brief Set the parent pointer for this run_d instance in a CHILD CONSTRUCTOR lock free. add_child will lock the parent node!
  /// @param parent_ptr
  void construct_parent(std::shared_ptr<survey_tree_d> parent_ptr) {
    this->parent = parent_ptr;
  }

  void set_pool(std::shared_ptr<BS::thread_pool<BS::tp::none>> pool_ptr) {
    std::unique_lock lock(mutex_);
    pool = pool_ptr;
  }

  std::shared_ptr<survey_tree_d> get_parent() const {
    std::shared_lock lock(mutex_);
    return parent.lock();
  }

  /// @brief Thread-safe method to get a copy of the channels vector. Do this ONLY if you need to iterate over all channels INCLUDING nullptr entries!
  /// @return A vector of shared_ptr to channel instances
  std::vector<std::shared_ptr<channel>> get_channel_vector() const {
    std::shared_lock lock(mutex_);
    return channels;
  }

  /// @brief Thread-safe method to get all valid (non-nullptr) channels as a vector.
  /// @return A vector of shared_ptr to valid channel instances.
  std::vector<std::shared_ptr<channel>> get_channels() const {
    std::shared_lock lock(mutex_);
    size_t count = 0;
    for (const auto &channel : channels) {
      if (channel != nullptr) {
        count++;
      }
    }
    std::vector<std::shared_ptr<channel>> chans;
    chans.reserve(count);
    for (const auto &channel : channels) {
      if (channel != nullptr) {
        chans.push_back(channel);
      }
    }
    return chans;
  }

  /// @brief Thread-safe method to get a specific channel by its channel number.
  /// @param channel_no The channel number to retrieve.
  /// @return A shared_ptr to the channel instance, or nullptr if the channel does not exist.
  std::shared_ptr<channel>
  get_channel(const size_t &channel_no) const {
    std::shared_lock lock(mutex_);
    if (channel_no >= max_survey_channels) {
      return nullptr;
    }
    return channels[channel_no];
  }

  /// @brief Clear all channels and reset the run to an empty state
  /// @details Thread-safe operation that resets channels vector and raw spectra data
  void clear() {
    std::unique_lock lock(mutex_);
    for (auto &channel : channels) {
      channel.reset();
    }
    raw_spc.reset();
  }

  /// @brief Clear only the channels without affecting raw spectra
  /// @details Thread-safe operation that resets only the channels vector
  void clear_channels() {
    std::unique_lock lock(mutex_);
    for (auto &channel_ptr : channels) {
      channel_ptr.reset();
    }
  }

  /// @brief Clear only the raw spectra without affecting channels
  void clear_raw_spectra() {
    std::unique_lock lock(mutex_);
    raw_spc.reset();
  }

  /// @brief count channels that are not nullptr
  /// @return Number of channels which can be used or are in use
  size_t size() const {
    std::shared_lock lock(mutex_);
    size_t count = 0;
    for (const auto &channel : channels) {
      if (channel != nullptr) {
        count++;
      }
    }
    return count;
  }

  /// @brief List all loaded channels with their numbers and file paths for console output
  void ls(const std::string &indent_str = "") const {
    std::shared_lock lock(mutex_);
    for (const auto &channel : channels) {
      if (channel) {
        std::cout << indent_str << "Channel " << channel->get_channel_no() << ": " << channel->get_filepath_wo_ext() << "\n";
      }
    }
  }

  // /*!
  //  * @brief convert run number to string like "run_001" or "run_0001" depending on the number of digits
  //  * @return
  //  */
  // std::string runs_str() const {
  //   std::shared_lock lock(mutex_);
  //   return "run_" + mstr::run2string(this->number, run_digits);
  // }

  /// @brief Get the run number
  /// @return The run number
  size_t get_run_number() const {
    std::shared_lock lock(mutex_);
    return this->number;
  }

  std::string get_name() const {
    std::shared_lock lock(mutex_);
    if (this->basePath.filename().string().empty())
      return "";
    // filename returns the last part of the path in case of a directory
    // in case of a file, it returns the file name with extension; w/o extension use stem()
    // a complete path would be this->basePath.string()
    // in many cases you use "canonical" paths, so the input is converted to absolute paths without symlinks
    return this->basePath.filename().string();
  }

  std::filesystem::path get_basepath() const {
    std::shared_lock lock(mutex_);
    return this->basePath;
  }

  void set_run_number(const size_t &num) {
    std::unique_lock lock(mutex_);
    this->number = num;
    ///< @todo consider implementing a move/rename operation if run number changes
  }

  /// @brief Get the sample rate of the run by checking the first valid channel
  /// @return The sample rate, or 0.0 if no valid channels are found
  double get_sample_rate() const {
    std::shared_lock lock(mutex_);
    for (const auto &chan : channels) {
      if (chan != nullptr) {
        return chan->get_sample_rate();
      }
    }
    return 0.0; // no valid channel found
  }

  std::string get_sample_rate_str() const {
    std::shared_lock lock(mutex_);
    for (const auto &chan : channels) {
      if (chan != nullptr) {
        return chan->get_sample_rate_str();
      }
    }
    return "N/A"; // no valid channel found
  }
  std::string start_datetime() const {
    std::shared_lock lock(mutex_);
    for (const auto &chan : channels) {
      if (chan != nullptr) {
        return chan->start_datetime();
      }
    }
    throw std::runtime_error("No valid channel found to get start datetime");
  }

  /// @brief returns the p_timer at the start of the run from the first valid channel
  /// @details the timer object provides enhanced information compared to a string datetime
  /// @return start p_timer (sample 0)
  p_timer p_start() const {
    std::shared_lock lock(mutex_);
    for (const auto &chan : channels) {
      if (chan != nullptr) {
        return chan->pt.p_start();
      }
    }
    throw std::runtime_error("No valid channel found to get start p_timer");
  }

  /// @brief Get the p_timer from the first valid channel
  /// @return The p_timer object
  p_timer get_p_timer() const {
    std::shared_lock lock(mutex_);
    for (const auto &chan : channels) {
      if (chan != nullptr) {
        return chan->pt;
      }
    }
    throw std::runtime_error("No valid channel found to get p_timer");
  }

  // set run number: can NOT be changed after creation! run number is art of the file or node structure
  xlogger logger;

  std::string stop_datetime() const {
    std::shared_lock lock(mutex_);
    for (const auto &chan : channels) {
      if (chan != nullptr) {
        return chan->stop_datetime();
      }
    }
    throw std::runtime_error("No valid channel found to get stop datetime");
  }
  void init_raw_spectra() {
    std::unique_lock lock(mutex_);
    if (!raw_spc) {
      raw_spc = std::make_shared<raw_spectra>(this->pool);
    }
  }

  void move_raw_spectra() {
    std::unique_lock lock(mutex_);
    if (!raw_spc) {
      throw std::runtime_error("Raw spectra not initialized");
    }
    for (const auto &chan : channels) {
      if (chan != nullptr) {
        raw_spc->move_raw_spectra(chan);
      }
    }
  }

  std::shared_ptr<raw_spectra> get_raw_spectra() const {
    std::shared_lock lock(mutex_);
    return raw_spc;
  }

private:
  fs::path basePath;
  std::weak_ptr<survey_tree_d> parent; ///< Parent survey_tree_d, if any
  mutable std::shared_mutex mutex_;    ///< Mutex for thread-safe access to channels
  // this is not a node, this is the data of a run node
  size_t number = SIZE_MAX; ///<  run number, e.g., 1 for run_001 or run_0001
  std::vector<std::shared_ptr<channel>> channels = std::vector<std::shared_ptr<channel>>(max_survey_channels, nullptr);
  std::shared_ptr<raw_spectra> raw_spc;                ///< raw spectra for the run - contains Ex, Ey, Hx, Hy, Hz, REx, REy, RHx, RHy, RHz, EEx, EEy (emap)
  std::shared_ptr<BS::thread_pool<BS::tp::none>> pool; ///< Thread pool for parallel tasks
};

// skip hdf5 for a while ... todo implementation later

/*
***************************************************************************************************************
*
*
*
*
* S U R V E Y   T R E E  D I R E C T O R Y   B A S E D
*
*
*
*
****************************************************************************************************************
*/

/// @brief survey_tree_d class represents a hierarchical tree structure for managing MT survey data on the filesystem.
/// @details The tree consists of nodes representing surveys, stations, and runs, each mapped to directories on the filesystem.
/// @details Each node can have multiple child nodes, allowing for a flexible organization of survey data.
/// @details The class provides thread-safe access and modification of the tree structure using shared mutexes.
class survey_tree_d : public std::enable_shared_from_this<survey_tree_d> {
private:
  std::weak_ptr<survey_tree_d> parent;
  std::map<std::string, std::shared_ptr<survey_tree_d>> children; ///< Child nodes mapped by their names, THIS HOLDS THE DATA STRUCTURE !!
  mutable std::shared_mutex mutex_;
  fs::path basePath;                                   ///< Filesystem path of this node
  NodeType type;                                       ///< Level of the node in the tree, experiment, survey, station, run
  bool read_only = true;                               ///< If true, the node is read-only and cannot be modified
  bool verbose = false;                                ///< If true, verbose logging is enabled
  size_t run_digits = 3;                               ///< Number of digits for run formatting, e.g., 3 for run_001
  std::shared_ptr<run_d> run_data;                     ///< For run nodes, hold a single run_d instance
  std::shared_ptr<BS::thread_pool<BS::tp::none>> pool; ///< Thread pool for parallel tasks

public:
  xlogger logger;
  // this is used only during construction, so i make it public for simplicity, no no lock during construction

  /// @brief destructor
  ~survey_tree_d() = default;

  /// @brief Constructor for the main survey node (top-level)
  /// @param name_ Name of the survey and therefore the base directory
  /// @param pool_ptr Thread pool pointer for parallel tasks
  /// @param run_digits_ Number of digits for run formatting, e.g., 3 for run_001
  /// @param read_only_ If true, the node is read-only and cannot be modified
  /// @param verbose_ If true, verbose logging is enabled
  /// @throws runtime_error if the base directory does not exist in read-only mode or if directory creation fails in write mode
  /// @details This constructor initializes the survey_tree_d instance as the top-level survey node. IT DOES NOT SCAN FOR CHILD NODES! USE SCAN METHOD FOR THAT.
  survey_tree_d(const std::string &name_, std::shared_ptr<BS::thread_pool<BS::tp::none>> pool_ptr, size_t run_digits_ = 3, const bool &read_only_ = true, const bool &verbose_ = false) {
    this->type = NodeType::survey;
    this->pool = pool_ptr;
    if (!pool_ptr) {
      throw std::runtime_error("Thread pool must be provided for survey level node");
    }
    this->basePath = fs::path{name_};
    this->read_only = read_only_;
    this->verbose = verbose_;
    this->run_digits = run_digits_;
    if (this->read_only) {
      if (!fs::exists(this->basePath) || !fs::is_directory(this->basePath)) {
        if (verbose) {
          logger << "Directory does not exist (read-only mode): " << this->basePath << "\n";
        }
        throw std::runtime_error("Directory does not exist (read-only mode): " + this->basePath.string());
      }
    } else {
      // create basePath directory if it does not exist
      if (!fs::exists(this->basePath)) {
        try {
          fs::create_directories(this->basePath);
        } catch (const fs::filesystem_error &e) {
          logger << "Failed to create directory: " << this->basePath << " (" << e.what() << ")\n";
          throw std::runtime_error("Failed to create directory: " + this->basePath.string() + " (" + e.what() + ")");
        }
        if (verbose) {
          logger << "Created directory: " << this->basePath << "\n";
        }
      }
    }
    if (!this->read_only) {
      // create stations directory and other subdirectories with
      try {
        create_survey_dirs(this->basePath, survey_dirs());
      } catch (const std::runtime_error &e) {
        logger << "Failed to create survey directories: " << e.what() << "\n";
        throw std::runtime_error("Failed to create survey directories: " + std::string(e.what()));
      }
    } else {
      // check that stations directory exists
      fs::path stations_path = this->basePath / "stations";
      if (!fs::exists(stations_path) || !fs::is_directory(stations_path)) {
        logger << "Stations directory does not exist (read-only mode): " << stations_path << "\n";
        throw std::runtime_error("Stations directory does not exist (read-only mode): " + stations_path.string());
      }
    }
  }

  survey_tree_d(const std::shared_ptr<survey_tree_d> &rhs, const std::string &name_) {
    if (rhs == nullptr) {
      throw std::runtime_error("Cannot copy from a null survey_tree_d pointer");
    }
    if (rhs->type != NodeType::survey) {
      throw std::runtime_error("Can only copy construct from a survey level node");
    }
    this->type = NodeType::survey; // allow for survey only
    this->read_only = false;       // always false for copy constructor, because we create a new instance for working
    this->basePath = fs::path{name_};
    // path can not be the same as rhs->basePath
    if (this->basePath == rhs->basePath) {
      throw std::runtime_error("Cannot copy construct to the same basePath: " + this->basePath.string());
    }
    this->pool = rhs->pool;
    this->verbose = rhs->verbose;
    this->run_digits = rhs->run_digits;
    // try to create basePath directory like we did in the main constructor
    if (!fs::exists(this->basePath)) {
      try {
        fs::create_directories(this->basePath);
      } catch (const fs::filesystem_error &e) {
        logger << "Failed to create directory: " << this->basePath << " (" << e.what() << ")\n";
        throw std::runtime_error("Failed to create directory: " + this->basePath.string() + " (" + e.what() + ")");
      }
      if (verbose) {
        logger << "Created directory: " << this->basePath << "\n";
      }
    }
    // create survey subdirectories
    try {
      create_survey_dirs(this->basePath, survey_dirs());
    } catch (const std::runtime_error &e) {
      logger << "Failed to create survey directories: " << e.what() << "\n";
      throw std::runtime_error("Failed to create survey directories: " + std::string(e.what()));
    }
    // we can not copy the children here, because we need to set the parent pointers correctly
    // so we leave it to the user to create stations and runs as needed
  }
  void copy_create_structure(const std::shared_ptr<survey_tree_d> &rhs) {
    if (rhs == nullptr) {
      throw std::runtime_error("Cannot copy from a null survey_tree_d pointer");
    }
    if (rhs->type != NodeType::survey) {
      throw std::runtime_error("Can only copy structure from a survey level node");
    }
    // now we copy / create all child nodes (stations and runs) from rhs to this
    this->scan(false, false); // scan without loading run data
    for (const auto &[station_name, station_ptr] : rhs->children) {
      // create station node
      auto new_station_ptr = std::make_shared<survey_tree_d>(station_name, shared_from_this());
      // add to children
      this->children[station_name] = new_station_ptr;
      // now copy runs
      for (const auto &[run_name, run_ptr] : station_ptr->children) {
        auto new_run_ptr = std::make_shared<survey_tree_d>(run_name, new_station_ptr);
        // add to children
        this->children[station_name]->children[run_name] = new_run_ptr;
        // now copy run data
        if (run_ptr->run_data) {
          // set parent pointer for run_d instance
          new_run_ptr->run_data->construct_parent(new_run_ptr);
          // set basepath for run_d instance
          new_run_ptr->run_data->set_basepath(new_run_ptr->basePath);
          // scan channels
          new_run_ptr->run_data->scan();
        }
      }
    }
  }

  /// @brief the delete indicates that there is NO DEFAULT CONSTRUCTOR
  survey_tree_d() = delete; // no default constructor
  /// @brief constructor for station and run nodes
  /// @param name station or run name, where run name is like "run_001"
  survey_tree_d(const std::string &name_, std::shared_ptr<survey_tree_d> parent_ptr) {
    this->parent = parent_ptr;
    if (parent_ptr == nullptr) {
      throw std::runtime_error("Parent pointer must not be null for station or run nodes: " + name_);
    }
    this->pool = parent_ptr->pool;
    if (this->pool == nullptr) {
      throw std::runtime_error("Thread pool must not be null for station or run nodes: " + name_);
    }
    // set the type according to parent type
    if (parent_ptr->type == NodeType::survey) {
      this->type = NodeType::station;
      this->basePath = parent_ptr->basePath / "stations" / name_;
    } else if (parent_ptr->type == NodeType::station) {
      this->type = NodeType::run;
      this->basePath = parent_ptr->basePath / name_;
    } else {
      throw std::runtime_error("Invalid parent type for survey_tree_d node: " + name_);
    }
    this->read_only = parent_ptr->read_only;
    this->verbose = parent_ptr->verbose;
    this->run_digits = parent_ptr->run_digits;
    if (this->read_only) {
      if (!fs::exists(this->basePath) || !fs::is_directory(this->basePath)) {
        if (verbose) {
          logger << "Directory does not exist (read-only mode): " << this->basePath << "\n";
        }
        throw std::runtime_error("Directory does not exist (read-only mode): " + this->basePath.string());
      }
    } else {
      // create basePath directory if it does not exist
      if (!fs::exists(this->basePath)) {
        try {
          fs::create_directories(this->basePath);
        } catch (const fs::filesystem_error &e) {
          logger << "Failed to create directory: " << this->basePath << " (" << e.what() << ")\n";
          throw std::runtime_error("Failed to create directory: " + this->basePath.string() + " (" + e.what() + ")");
        }
        if (verbose) {
          logger << "Created directory: " << this->basePath << "\n";
        }
      }
    }
    // at run level, we add a run_d instance automatically
    if (this->type == NodeType::run) {
      // create run_d instance and assign to run member
      // you can not! use shared_from_this() here, because the shared_ptr is not yet created when we are in the constructor
      // so we set the parent pointer later when we add the run node to the tree, inside a lock use: construct_parent
      this->run_data = std::make_shared<run_d>(this->basePath, std::shared_ptr<survey_tree_d>(nullptr), this->pool);
    }
  }
  /// @brief Get the name of the node
  /// @return The name of the node
  /// @details For survey level, this is the survey name; for station level, the station name; for run level, the run name like "run_001", use get_basepath to get the full path
  std::string get_name() const {
    std::shared_lock lock(mutex_);
    return this->basePath.filename().string();
  }

  std::filesystem::path get_basepath() const {
    std::shared_lock lock(mutex_);
    return this->basePath;
  }

  /// @brief Set the read-only status of the node
  /// @param ro Read-only flag
  void
  set_read_only(const bool &ro) {
    std::unique_lock lock(mutex_);
    this->read_only = ro;
  }

  /// @brief Get the read-only status of the node
  /// @return Read-only flag
  bool get_read_only() const {
    std::shared_lock lock(mutex_);
    return this->read_only;
  }

  /// @brief Set the verbose logging status of the node
  /// @param vb Verbose flag
  void set_verbose(const bool &vb) {
    std::unique_lock lock(mutex_);
    this->verbose = vb;
  }

  /// @brief Get the verbose logging status of the node
  /// @return Verbose flag
  bool get_verbose() const {
    std::shared_lock lock(mutex_);
    return this->verbose;
  }

  /// @brief Get the node type
  /// @return NodeType enum value
  NodeType get_nodetype() const {
    std::shared_lock lock(mutex_);
    return this->type;
  }

  /// @brief Get a child node by string name
  /// @param string or run number as string (will be formatted as run_XXX for station level if "1" but not "run_1" is given)
  /// @return child node pointer
  std::shared_ptr<survey_tree_d> get_child(const std::string &name) const {
    std::shared_lock lock(mutex_);
    std::string child_name;
    // if we are at station level, the children are runs, so we format the number accordingly
    if ((this->type == NodeType::station) && (std::all_of(name.begin(), name.end(), ::isdigit))) {
      child_name = std::format("run_{:0{}}", std::stoi(name), this->run_digits);
    } else {
      child_name = name;
    }
    auto it = children.find(child_name);
    if (it != children.end()) {
      return it->second;
    }
    return nullptr; // Child not found
  }

  std::shared_ptr<survey_tree_d> get_child(const size_t &number) const {
    std::shared_lock lock(mutex_);
    // no assumption about the node type here, just format the number
    std::string child_name = std::format("{}", number);
    // unlock and use get_child(string)
    lock.unlock();
    return get_child(child_name);
  }
  /// @brief Get all child nodes as a vector
  /// @return all child nodes as a vector
  std::vector<std::shared_ptr<survey_tree_d>> get_children() const {
    std::shared_lock lock(mutex_);
    std::vector<std::shared_ptr<survey_tree_d>> child_nodes;
    for (const auto &[child_name, child_node] : children) {
      child_nodes.push_back(child_node);
    }
    return child_nodes;
  }

  std::shared_ptr<survey_tree_d> get_parent() const {
    std::shared_lock lock(mutex_);
    return parent.lock();
  }

  // some convenience functions
  bool is_survey() const {
    return this->type == NodeType::survey;
  }
  bool is_station() const {
    return this->type == NodeType::station;
  }
  bool is_run() const {
    return this->type == NodeType::run;
  }

  /// @brief Get the base path of the survey_tree_d node
  /// @return The filesystem path
  /// @details that is the full path up to this node, use get_name to get the node name only
  fs::path get_base_path() const {
    return basePath;
  }

  /// @brief Get the run object using run number
  /// @param number Run number (e.g., 1 for run_001)
  /// @param station Station name (only for survey-level nodes)
  /// @details You may mostly call this function on station level nodes. Optionally you can call it on survey level nodes by providing the station name (convenience).
  /// @return Shared pointer to run_d object, nullptr if run not found (not the run node)
  std::shared_ptr<run_d> get_run_data(std::optional<size_t> number = std::nullopt, std::optional<std::string> station = std::nullopt) const {
    std::shared_lock lock(mutex_);
    if (this->type == NodeType::run) {
      return this->run_data;
    }
    if (this->type == NodeType::survey) {
      // station argument is required
      if (!station) {
        return nullptr;
      }
      auto it = children.find(station.value());
      if (it != children.end()) {
        auto stationNode = it->second;
        return stationNode->get_run_data(number);
      }
      return nullptr; // Station not found
    }
    // station level that is logical here
    if (!number) {
      return nullptr; // Run number is required
    }
    std::string run_name = std::format("run_{:0{}}", number.value(), this->run_digits);
    auto it = children.find(run_name);
    if (it != children.end() && it->second->run_data) {
      return it->second->run_data;
    }
    return nullptr; // Run not found
  }

  /// @brief move a shared pointer to run_d into the run node
  /// @param run_ptr, the "&&" indicates that the shared_ptr will be moved
  /// @details create a run node first using add_child, then call this method to set the run_d instance
  void set_run_data(std::shared_ptr<run_d> &&run_ptr) {
    std::unique_lock lock(mutex_);
    if (this->type != NodeType::run) {
      throw std::runtime_error("set_run_data can only be called on run nodes");
    }
    this->run_data = std::move(run_ptr);
    this->run_data->set_parent(shared_from_this());
    this->run_data->set_run_number(std::stoi(this->basePath.filename().string().substr(4))); // Extract run number from "run_XXX", (4) means start after "run_"
    this->run_data->set_pool(this->pool);
    this->run_data->set_basepath(this->basePath);
  }

  /// @brief Get all runs for a given station or all runs if no station is specified
  /// @param station Optional station name. If not given, all runs under the current node are returned (e.g. survey level).
  /// @return Vector of shared pointers to run_d objects (not the rin nodes)
  std::vector<std::shared_ptr<run_d>> get_runs_data(const std::optional<std::string> station = std::nullopt) const {
    std::shared_lock lock(mutex_);
    std::vector<std::shared_ptr<run_d>> runs;
    if (this->type == NodeType::run) {
      throw std::runtime_error("get_runs_data cannot be called on run nodes");
    }

    if (this->type == NodeType::station) {
      // station level
      for (const auto &[child_name, child_node] : children) {
        if (child_node->run_data) {
          runs.push_back(child_node->run_data);
        }
      }
      return runs;
    }

    if (this->type == NodeType::survey) {
      if (station) {
        // specific station, station argument is given
        auto it = children.find(station.value());
        if (it != children.end()) {
          auto stationNode = it->second;
          for (const auto &[child_name, child_node] : stationNode->children) {
            if (child_node->run_data) {
              runs.push_back(child_node->run_data);
            }
          }
        }
      } else {
        // survey level and no station - we iterate child nodes (aka stations) and get ALL runs
        for (const auto &[station_name, station_node] : children) {
          for (const auto &[child_name, child_node] : station_node->children) {
            if (child_node->run_data) {
              runs.push_back(child_node->run_data);
            }
          }
        }
      }
      return runs;
    }
    throw std::runtime_error("get_runs_data can only be called on survey or station nodes");
  }

  std::vector<std::shared_ptr<channel>> get_all_channels(const std::optional<std::string> station = std::nullopt, const std::optional<size_t> run_number = std::nullopt) const {
    std::shared_lock lock(mutex_);
    std::vector<std::shared_ptr<channel>> all_channels;
    auto runs = this->get_runs_data(station);
    for (const auto &run : runs) {
      if (run_number) {
        if (run->get_run_number() == run_number.value()) {
          auto channels = run->get_channels();
          all_channels.insert(all_channels.end(), channels.begin(), channels.end());
        }
      } else {
        auto channels = run->get_channels();
        all_channels.insert(all_channels.end(), channels.begin(), channels.end());
      }
    }
    return all_channels;
  }

  /// @brief adds a child node with the given name if it does not exist
  /// @param child_name
  /// @return pointer to the child node
  std::shared_ptr<survey_tree_d> add_child(const std::string &child_name) {
    std::unique_lock lock(mutex_);
    // if child consists of numbers only and we are at station level, format it as run_XXX
    std::string formatted_child_name;
    if ((this->type == NodeType::station) && (std::all_of(child_name.begin(), child_name.end(), ::isdigit))) {
      formatted_child_name = std::format("run_{:0{}}", std::stoi(child_name), this->run_digits);
    } else {
      formatted_child_name = child_name;
    }
    if (children.find(formatted_child_name) != children.end()) {
      return children[formatted_child_name]; // Child already exists
    }
    // the child node will set its parent pointer, detect the type and set basePath accordingly, as well as the read_only flag
    auto child_node = std::make_shared<survey_tree_d>(formatted_child_name, shared_from_this());
    // if the child is a run node, set its run_d parent pointer
    if (child_node->type == NodeType::run) {
      child_node->run_data->set_parent(shared_from_this());
    }
    children[formatted_child_name] = child_node;
    return child_node;
  }

  /// @brief adds a child node with the given run number if it does not exist
  /// @param number run number
  /// @return pointer to the child node
  /// @details you can use get_nextRunId to get the next available run number; use addAutoRun for convenience; you don't need to scan the directory first, in case you have loaded the full tree.
  std::shared_ptr<survey_tree_d> add_child(const size_t &number) {
    std::unique_lock lock(mutex_);
    std::string child_name = std::format("{}", number);
    lock.unlock();
    return add_child(child_name);
  }

  void clear() {
    std::unique_lock lock(mutex_);
    children.clear();
  }

  /// @brief Scan the directory and update the tree structure
  /// @param recursive If true, recursively scan child directories; if false, only scan current level
  /// @param b_channels If true, scan channels.json files at run level (can be expensive)
  void scan(const bool recursive = true, const bool b_channels = false) {
    // std::unique_lock lock(mutex_);
    if (this->pool == nullptr) {
      throw std::runtime_error("Thread pool is null");
    }
    fs::path dir_path = this->basePath;
    if (this->type == NodeType::survey) {
      dir_path = this->basePath / "stations";
    }
    // scan directory
    if (!fs::exists(dir_path) || !fs::is_directory(dir_path)) {
      if (this->verbose) {
        logger << "directory does not exist: " << dir_path << "\n";
      }
      throw std::runtime_error(" directory does not exist: " + dir_path.string());
    }
    for (const auto &entry : fs::directory_iterator(dir_path)) {
      if (entry.is_directory()) {
        std::string station_name_run = entry.path().filename().string();
        if (this->type == NodeType::station) {
          // we are at station level, so the child must be a run
          if (!std::regex_match(station_name_run, std::regex(R"(run_\d{1,})"))) {
            if (this->verbose) {
              logger << "Skipping non-run directory at station level: " << station_name_run << "\n";
            }
            continue; // skip non-run directories
          }
        }
        auto node = this->add_child(station_name_run);
        if (recursive) {
          node->scan(recursive, b_channels);
        }
        // at run level, we can scan the run_d instance for channels
        if (b_channels && (node->type == NodeType::run) && (node->run_data != nullptr)) {
          // scan channels in parallel using the thread pool
          this->pool->detach_task([node]() {
            node->run_data->scan();
          });
        }
      }
    }
    pool->wait();
    return;
  }

  /// @brief create a new survey_tree_d instance containing ONLY the selected stations and runs
  /// @param  existing_tree
  /// @param station_configs
  /// @details a later processing shall ITERATE OVER ALL NODES! so no checks later.
  /// @details you don't need to scan first. IT WILL SCAN THE CHANNELS!
  /// @return new survey_tree_d instance with selected nodes
  std::shared_ptr<survey_tree_d> select_only(const std::vector<station_config> &station_configs) {
    std::unique_lock lock(mutex_);
    // create a new survey_tree_d instance for the selected nodes
    auto selected_tree = std::make_shared<survey_tree_d>(this->basePath, this->pool, this->run_digits, this->read_only, this->verbose);
    for (const auto &station_config : station_configs) {
      // we let the selected_tree iterate over the filesystem to add the station and runs
      auto station_node = selected_tree->add_child(station_config.station_name);
      for (const auto &run_config : station_config.runs) {
        size_t run_number = run_config.first;
        auto run_node = station_node->add_child(run_number);
        pool->detach_task([run_node, run_config]() {
          // scan channels selectively
          run_node->run_data->scan(run_config.second);
        });
      }
    }
    pool->wait();
    return selected_tree;
  }

  /// @brief list the tree structure to console
  /// @param recursive If true, recursively list child nodes; if false, only list current level
  /// @param indent space indentation for pretty printing
  void ls(const bool recursive = true, const size_t &indent = 3) const {
    std::shared_lock lock(mutex_);
    std::string indent_str(indent, ' ');
    std::cout << indent_str << "Node: " << basePath.filename().string() << " (Type: ";
    switch (type) {
    case NodeType::survey:
      std::cout << "Survey";
      break;
    case NodeType::station:
      std::cout << "Station";
      break;
    case NodeType::run:
      std::cout << "Run";
      break;
    default:
      std::cout << "Unknown";
      break;
    }
    std::cout << ", Path: " << basePath << ")\n";
    if (type == NodeType::run && run_data != nullptr) {
      std::cout << indent_str << "  Channels:\n";
      std::string indent_str_channels = indent_str + "    ";
      run_data->ls(indent_str_channels);
    }
    for (const auto &[child_name, child_node] : children) {
      child_node->ls(recursive, indent + 2);
      if (!recursive) {
        break;
      }
    }
  }
  /// @brief adds a run node with the next available run ID
  /// @param min_run_id in case you want to enforce a minimum run ID, you have run_003 and want to get at least run_005, set min_run_id=5
  /// @param scanDisk in case the run directories are not yet loaded in memory, scan the filesystem
  /// @return pointer to the newly created run node
  std::shared_ptr<survey_tree_d> addAutoRun(const size_t min_run_id = 0, bool scanDisk = false) {
    int nextId = getNextRunId(min_run_id, scanDisk);
    return add_child(nextId);
  }
  /// @brief scans a station node for the next available run ID;
  /// @param min_run_id in case you want to enforce a minimum run ID, you have run_003 and want to get at least run_005, set min_run_id=5
  /// @param scanDisk in case the run directories are not yet loaded in memory, scan the filesystem
  /// @return
  size_t getNextRunId(const size_t min_run_id = 0, bool scanDisk = false) const {
    if (NodeType::station != this->type) {
      throw std::runtime_error("getNextRunId can only be called on station nodes");
    }
    std::shared_lock lock(mutex_);
    int maxId = -1; // start with -1, so nextId will be at least 0 and can be converted to size_t
    if (scanDisk) {
      fs::path dir_path = this->basePath;
      if (!fs::exists(dir_path) || !fs::is_directory(dir_path)) {
        throw std::runtime_error(" directory does not exist: " + dir_path.string());
      }
      for (const auto &entry : fs::directory_iterator(dir_path)) {
        if (entry.is_directory()) {
          std::string station_name_run = entry.path().filename().string();
          std::smatch match;
          if (std::regex_match(station_name_run, match, std::regex(R"(run_(\d{1,}))"))) {
            int runId = std::stoi(match[1]);
            if (runId > maxId) {
              maxId = runId;
            }
          }
        }
      }
    }
    // also check the children in memory
    for (const auto &[child_name, child_node] : children) {
      std::smatch match;
      if (std::regex_match(child_name, match, std::regex(R"(run_(\d{1,}))"))) {
        int runId = std::stoi(match[1]);
        if (runId > maxId) {
          maxId = runId;
        }
      }
    }
    size_t nextId = static_cast<size_t>(maxId + 1);
    if (nextId < min_run_id) {
      nextId = min_run_id;
    }
    if (nextId < 1) {
      nextId = 1;
    }
    return nextId;
  }

  /// @brief delete empty run nodes from the tree
  /// @details a run node is considered empty if its run_d instance is nullptr or has zero channels <br>
  /// you want FINALLY iterate over all DATA and PROCESS. The philosophy is to operate on VALID runs only, so empty runs are useless and force you check inside processing loops.
  void delete_empty_runs() {
    std::unique_lock lock(mutex_);
    for (auto it = children.begin(); it != children.end();) {
      auto &child = it->second;
      if (child->type == NodeType::run) {
        auto run_node = child;
        if (run_node->run_data == nullptr || run_node->run_data->size() == 0) {
          if (this->verbose) {
            logger << "Deleting empty run node: " << run_node->get_name() << "\n";
          }
          it = children.erase(it);
          continue;
        }
      } else if (child->type == NodeType::station) {
        // recurse into station children to prune their runs
        lock.unlock();
        child->delete_empty_runs();
        lock.lock();
      }
      ++it;
    }
  }
}; // end of class survey_tree_d

// stl style functions *************************************************************************

/// @brief compare two run instances and their run_d content for equality
/// @param lhs left-hand side run shared pointer (so tree elements can be passed directly)
/// @details it will use the channel comparison operator to compare the channels inside the run_d instances <br>
/// so you may find duplicates, stations has two runs with same content but different run numbers etc.
/// @warning if you have older imports with slightly different values for FILTER settings, those will be detected as different content! even thought the are the same for practical purposes.
/// @return true if both runs have the same channels with the same content, false otherwise
inline auto compare_same_content = [](const std::shared_ptr<survey_tree_d> &lhs, const std::shared_ptr<survey_tree_d> &rhs) {
  if (!lhs || !rhs) {
    throw std::runtime_error("Null pointer passed to compare_same_content");
  }
  // protect to work on the same node
  if (lhs == rhs) {
    throw std::runtime_error("compare_same_content called with the same node pointers");
  }
  if (lhs->get_run_data() == nullptr) {
    throw std::runtime_error("Left-hand side run pointer is null in compare_same_content");
  }
  if (rhs->get_run_data() == nullptr) {
    throw std::runtime_error("Right-hand side run pointer is null in compare_same_content");
  }
  auto lhs_channels = lhs->get_run_data()->get_channel_vector();
  auto rhs_channels = rhs->get_run_data()->get_channel_vector();
  // check if nullptr are at same positions
  if (lhs_channels.size() != rhs_channels.size()) {
    return false;
  }
  for (size_t i = 0; i < lhs_channels.size(); ++i) {
    if ((lhs_channels[i] == nullptr) != (rhs_channels[i] == nullptr)) {
      return false;
    }
    if (lhs_channels[i] != nullptr && rhs_channels[i] != nullptr) {
      if (*lhs_channels[i] != *rhs_channels[i]) {
        return false;
      }
    }
  }
  return true;
};

inline auto compare_earlier_run = [](const std::shared_ptr<survey_tree_d> &lhs, const std::shared_ptr<survey_tree_d> &rhs) {
  if (!lhs || !rhs) {
    throw std::runtime_error("Null pointer passed to compare_earlier_run");
  }
  // protect to work on the same node
  if (lhs == rhs) {
    throw std::runtime_error("compare_same_content called with the same node pointers");
  }
  if (lhs->get_run_data() == nullptr) {
    throw std::runtime_error("Left-hand side run pointer is null in compare_earlier_run");
  }
  if (rhs->get_run_data() == nullptr) {
    throw std::runtime_error("Right-hand side run pointer is null in compare_earlier_run");
  }
  if (lhs->get_run_data()->size() == 0) {
    throw std::runtime_error("Left-hand side run has no channels in compare_earlier_run");
  }
  if (rhs->get_run_data()->size() == 0) {
    throw std::runtime_error("Right-hand side run has no channels in compare_earlier_run");
  }
  try {
    auto lhs_start = lhs->get_run_data()->p_start();
    auto rhs_start = rhs->get_run_data()->p_start();
    return lhs_start < rhs_start;
  } catch (const std::runtime_error &e) {
    throw std::runtime_error("Error comparing run start datetimes: " + std::string(e.what()));
  }
};

inline auto compare_is_parallel_run = [](const std::shared_ptr<survey_tree_d> &lhs, const std::shared_ptr<survey_tree_d> &rhs) {
  if (!lhs || !rhs) {
    throw std::runtime_error("Null pointer passed to compare_is_parallel_run");
  }
  // protect to work on the same node
  if (lhs == rhs) {
    throw std::runtime_error("compare_same_content called with the same node pointers");
  }
  if (lhs->get_run_data() == nullptr) {
    throw std::runtime_error("Left-hand side run pointer is null in compare_is_parallel_run");
  }
  if (rhs->get_run_data() == nullptr) {
    throw std::runtime_error("Right-hand side run pointer is null in compare_is_parallel_run");
  }
  if (lhs->get_run_data()->size() == 0) {
    throw std::runtime_error("Left-hand side run has no channels in compare_is_parallel_run");
  }
  if (rhs->get_run_data()->size() == 0) {
    throw std::runtime_error("Right-hand side run has no channels in compare_is_parallel_run");
  }
  // both must at least same amount of channels
  if (lhs->get_run_data()->size() != rhs->get_run_data()->size()) {
    return false;
  }
  size_t matches_needed = lhs->get_run_data()->size();
  size_t matches_found = 0;
  try {
    // now run lhs can have Ex, Ey, Hx, Hy, Hz, null, null, null, null, null, null
    // and rhs can have Ex, Ey,null, null, null, Hx, Hy, Hz
    // so we iterate over lhs channels and check if rhs has a channel with same component and overlapping time
    auto lhs_channels = lhs->get_run_data()->get_channel_vector();
    auto rhs_channels = rhs->get_run_data()->get_channel_vector();
    // outer loop over lhs channels
    for (const auto &lhs_chan : lhs_channels) {
      if (lhs_chan == nullptr) {
        continue;
      }
      // find matching channel in rhs using is_parallel_channel from channel.hpp
      // inner loop over rhs channels
      for (const auto &rhs_chan : rhs_channels) {
        if (rhs_chan == nullptr) {
          continue;
        }
        if (is_parallel_channel(lhs_chan, rhs_chan)) {
          matches_found++;
          break; // break inner loop, go to next lhs channel}
        }
      } // inner loop over rhs channels
    } // outer loop ends
    if (matches_found != matches_needed) {
      return false;
    }
  } catch (const std::runtime_error &e) {
    throw std::runtime_error("Error comparing run datetimes: " + std::string(e.what()));
  }
  return false;
};

/// @brief the run std::shared_ptr<survey_tree_d> node is different, but the station node std::shared_ptr<survey_tree_d> is the same.
inline auto compare_run_d_same_station = [](const std::shared_ptr<run_d> &lhs, const std::shared_ptr<run_d> &rhs) {
  if (!lhs || !rhs) {
    throw std::runtime_error("Null pointer passed to compare_run_d_same_station");
  }
  if (lhs == rhs) {
    throw std::runtime_error("compare_run_d_same_station called with the same run_d pointers");
  }
  auto lhs_parent = lhs->get_parent(); // run
  auto rhs_parent = rhs->get_parent(); // run
  if (!lhs_parent || !rhs_parent) {
    throw std::runtime_error("Null parent pointer in compare_run_d_same_station");
  }
  lhs_parent = lhs_parent->get_parent(); // station
  rhs_parent = rhs_parent->get_parent(); // station
  if (!lhs_parent || !rhs_parent) {
    throw std::runtime_error("Null station parent pointer in compare_run_d_same_station");
  }
  return lhs_parent == rhs_parent;
};

#endif // SURVEY_TREE_HPP
