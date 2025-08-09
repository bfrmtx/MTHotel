#ifndef SURVEY_TREE_HPP
#define SURVEY_TREE_HPP

#include <filesystem>
#include <memory>
#include <mutex>
#include <regex>
#include <shared_mutex>
#include <string>
#include <vector>

// my classes
#include "channel.hpp"
#include "mt_base.hpp"
#include "strings_etc.hpp"

/*! @file survey_tree.hpp
 * the survey class is a tree structure with stations, runs and channels.
 * The survey class is a singleton, so there is only one instance of the survey class.
 *└── Northern_Mining
 *    ├── config
 *    ├── db
 *    ├── dump
 *    ├── edi
 *    ├── filters
 *    ├── jle
 *    ├── jobs
 *    ├── log
 *    ├── meta
 *    │   ├── Kocatepe
 *    │   │   ├── run_001
 *    │   │   │   ├── 085_2009-08-20_13-22-00_2009-08-21_07-00-00_R001_128H.xml
 *    │   │   │   ├── 085_ADU-07e_C000_TEx_128Hz.json
 *    │   │   │   ├── 085_ADU-07e_C001_TEy_128Hz.json
 *    │   │   │   ├── 085_ADU-07e_C002_THx_128Hz.json
 *    │   │   │   ├── 085_ADU-07e_C003_THy_128Hz.json
 *    │   │   │   └── 085_ADU-07e_C004_THz_128Hz.json
 *    │   │   └── run_002
 *    │   │       ├── 085_2009-08-20_13-22-01_2009-08-21_06-59-59_R001_32H.xml
 *    │   │       ├── 085_ADU-07e_C000_TEx_32Hz.json
 *    │   │       ├── 085_ADU-07e_C001_TEy_32Hz.json
 *    │   │       ├── 085_ADU-07e_C002_THx_32Hz.json
 *    │   │       ├── 085_ADU-07e_C003_THy_32Hz.json
 *    │   │       └── 085_ADU-07e_C004_THz_32Hz.json
 *    │   └── Sarıçam
 *    │       ├── run_001
 *    │       │   ├── 084_2009-08-20_13-22-00_2009-08-21_07-00-00_R001_128H.xml
 *    │       │   ├── 084_ADU-07e_C000_TEx_128Hz.json
 *    │       │   ├── 084_ADU-07e_C001_TEy_128Hz.json
 *    │       │   ├── 084_ADU-07e_C002_THx_128Hz.json
 *    │       │   ├── 084_ADU-07e_C003_THy_128Hz.json
 *    │       │   └── 084_ADU-07e_C004_THz_128Hz.json
 *    │       └── run_002
 *    │           ├── 084_2009-08-20_13-22-01_2009-08-21_06-59-59_R001_32H.xml
 *    │           ├── 084_ADU-07e_C000_TEx_32Hz.json
 *    │           ├── 084_ADU-07e_C001_TEy_32Hz.json
 *    │           ├── 084_ADU-07e_C002_THx_32Hz.json
 *    │           ├── 084_ADU-07e_C003_THy_32Hz.json
 *    │           └── 084_ADU-07e_C004_THz_32Hz.json
 *    ├── processings
 *    ├── reports
 *    ├── shell
 *    │   ├── mkallproc.sh
 *    │   ├── plot_ascii_table_edi.sh
 *    │   └── procall.sh
 *    ├── stations
 *    │   ├── Kocatepe
 *    │   │   ├── run_001
 *    │   │   │   ├── 085_ADU-07e_C000_TEx_128Hz.atss
 *    │   │   │   ├── 085_ADU-07e_C000_TEx_128Hz.json
 *    │   │   │   ├── 085_ADU-07e_C001_TEy_128Hz.atss
 *    │   │   │   ├── 085_ADU-07e_C001_TEy_128Hz.json
 *    │   │   │   ├── 085_ADU-07e_C002_THx_128Hz.atss
 *    │   │   │   ├── 085_ADU-07e_C002_THx_128Hz.json
 *    │   │   │   ├── 085_ADU-07e_C003_THy_128Hz.atss
 *    │   │   │   ├── 085_ADU-07e_C003_THy_128Hz.json
 *    │   │   │   ├── 085_ADU-07e_C004_THz_128Hz.atss
 *    │   │   │   └── 085_ADU-07e_C004_THz_128Hz.json
 *    │   │   └── run_002
 *    │   │       ├── 085_ADU-07e_C000_TEx_32Hz.atss
 *    │   │       ├── 085_ADU-07e_C000_TEx_32Hz.json
 *    │   │       ├── 085_ADU-07e_C001_TEy_32Hz.atss
 *    │   │       ├── 085_ADU-07e_C001_TEy_32Hz.json
 *    │   │       ├── 085_ADU-07e_C002_THx_32Hz.atss
 *    │   │       ├── 085_ADU-07e_C002_THx_32Hz.json
 *    │   │       ├── 085_ADU-07e_C003_THy_32Hz.atss
 *    │   │       ├── 085_ADU-07e_C003_THy_32Hz.json
 *    │   │       ├── 085_ADU-07e_C004_THz_32Hz.atss
 *    │   │       └── 085_ADU-07e_C004_THz_32Hz.json
 *    │   └── Sarıçam
 *    │       ├── run_001
 *    │       │   ├── 084_ADU-07e_C000_TEx_128Hz.atss
 *    │       │   ├── 084_ADU-07e_C000_TEx_128Hz.json
 *    │       │   ├── 084_ADU-07e_C001_TEy_128Hz.atss
 *    │       │   ├── 084_ADU-07e_C001_TEy_128Hz.json
 *    │       │   ├── 084_ADU-07e_C002_THx_128Hz.atss
 *    │       │   ├── 084_ADU-07e_C002_THx_128Hz.json
 *    │       │   ├── 084_ADU-07e_C003_THy_128Hz.atss
 *    │       │   ├── 084_ADU-07e_C003_THy_128Hz.json
 *    │       │   ├── 084_ADU-07e_C004_THz_128Hz.atss
 *    │       │   └── 084_ADU-07e_C004_THz_128Hz.json
 *    │       └── run_002
 *    │           ├── 084_ADU-07e_C000_TEx_32Hz.atss
 *    │           ├── 084_ADU-07e_C000_TEx_32Hz.json
 *    │           ├── 084_ADU-07e_C001_TEy_32Hz.atss
 *    │           ├── 084_ADU-07e_C001_TEy_32Hz.json
 *    │           ├── 084_ADU-07e_C002_THx_32Hz.atss
 *    │           ├── 084_ADU-07e_C002_THx_32Hz.json
 *    │           ├── 084_ADU-07e_C003_THy_32Hz.atss
 *    │           ├── 084_ADU-07e_C003_THy_32Hz.json
 *    │           ├── 084_ADU-07e_C004_THz_32Hz.atss
 *    │           └── 084_ADU-07e_C004_THz_32Hz.json
 *    └── tmp
 *
 */

// Scan for channels in the run directory
/*!
 * @brief Maximum number of channels; each channel is at a fixed position! As it comes from the data logger
 * @details we have several times "Ex" for example, so we have to use the channel number to identify the channel.
 * @details refer to the run_d class documentation for more details.
 */

// forward declaration of tree class
class survey_tree;

namespace fs = std::filesystem;
class run_d {
public:
  explicit run_d(const fs::path &path) :
      basePath(path) {
    // directory must exist or created before using it
    if (!fs::exists(basePath) || !fs::is_directory(basePath)) {
      std::cerr << "Directory does not exist: " << basePath << "\n";
    }
    this->scan();
  }

  size_t scan() {
    std::unique_lock lock(mutex_);
    size_t count = 0;
    for (const auto &entry : fs::directory_iterator(basePath)) {
      if (entry.is_regular_file() && entry.path().filename().string().ends_with(".json")) {

        // Only process .json files as valid channel files
        size_t channel_no = mstr::channel_number_from_channel_file(entry.path());
        if (channel_no != SIZE_MAX && channel_no < max_survey_channels) {
          channels[channel_no] = std::make_shared<channel>(entry.path());
          count++;
        }
      }
    }
    return count;
  }

  void ls() const {
    std::shared_lock lock(mutex_);
    for (const auto &channel : channels) {
      if (channel) {
        std::cout << "Channel " << channel->get_channel_no() << ": " << channel->get_filepath_wo_ext() << "\n";
      }
    }
  }

  size_t nchannels() const {
    std::shared_lock lock(mutex_);
    size_t count = 0;
    for (const auto &channel : channels) {
      if (channel) {
        count++;
      }
    }
    return count;
  }

  std::vector<std::shared_ptr<channel>> get_channels() const {
    std::shared_lock lock(mutex_);
    return channels; // that must be size of max_survey_channels, filled with nullptrs if not used
  }

  double get_sample_rate() const {
    std::shared_lock lock(mutex_);
    if (channels.empty()) {
      return 0.0; // No channels available
    }
    for (const auto &channel : channels) {
      if (channel) {
        return channel->get_sample_rate(); // Return the sample rate of the first valid channel
      }
    }
    return 0.0; // No valid channels found
  }

  std::string get_datetime() const {
    std::shared_lock lock(mutex_);
    if (channels.empty()) {
      return "1970-01-01T00:00:00"; // Default date if no channels are available
    }
    for (const auto &channel : channels) {
      if (channel) {
        return channel->get_datetime(); // Return the datetime of the first valid channel
      }
    }
    return "1970-01-01T00:00:00"; // No valid channels found
  }

  p_timer get_pt() const {
    std::shared_lock lock(mutex_);
    if (channels.empty()) {
      return p_timer(); // Default timer if no channels are available
    }
    for (const auto &channel : channels) {
      if (channel) {
        return channel->pt; // Return the timer of the first valid channel
      }
    }
    return p_timer(); // No valid channels found
  }

  std::vector<std::shared_ptr<channel>> channels = std::vector<std::shared_ptr<channel>>(max_survey_channels, nullptr);
  std::shared_ptr<survey_tree> parent; //!< Parent survey_tree, if any
private:
  fs::path basePath;
  mutable std::shared_mutex mutex_; //!< Mutex for thread-safe access to channels
};

/*!
 * @brief Compare two run_d instances for equality, NOT for filepath equality (sure they are different)
 * @details comparing filenames is not enough, we have to compare the content of the channels
 * @details This is used to check if two runs are the same, and my remove duplicate runs
 * @details they have the same amount of files AND the channels are same.
 */
static auto compare_same_content = [](const std::shared_ptr<run_d> &lhs, const std::shared_ptr<run_d> &rhs) {
  if (!lhs || !rhs) {
    return false; // If either is null, they are not the same
  }
  auto channels_lhs = lhs->get_channels();
  auto channels_rhs = rhs->get_channels();
  if (channels_lhs.size() != channels_rhs.size()) {
    return false; // Different number of channels
  }
  // now first check the channels. valid shared_pointers and nullptrs must have the same INDEX, otherwise they are not the same, and I can later not iterate over BOTH simultaneously
  for (size_t i = 0; i < channels_lhs.size(); ++i) {
    if ((channels_lhs[i] == nullptr) != (channels_rhs[i] == nullptr)) {
      return false; // Channels at index i are not both nullptr or both valid
    }
  }
  // now we can check using operator == for each channel shared_ptr from the channel class
  for (size_t i = 0; i < channels_lhs.size(); ++i) {
    if (channels_lhs[i] && channels_rhs[i]) {
      if (!(channels_lhs[i] == channels_rhs[i])) {
        return false; // Channel content doesn't match
      }
    }
  }

  return true; // All checks passed, they are considered the same; only json files are checked.
};

/*!
 * @brief compare start times of two runs; we late sort the runs by their start time (earlier run first)
 */
static auto compare_earlier_run = [](const std::shared_ptr<run_d> &lhs, const std::shared_ptr<run_d> &rhs) {
  if (!lhs || !rhs)
    return false; // If either is null, we cannot compare

  p_timer pt_lhs = lhs->get_pt();
  p_timer pt_rhs = rhs->get_pt();
  return pt_lhs < pt_rhs; // Compare the timers directly

  return false; // If either is null, we cannot compare
};

class survey_tree : public std::enable_shared_from_this<survey_tree> {
private:
  std::string name;
  std::weak_ptr<survey_tree> parent;
  std::map<std::string, std::shared_ptr<survey_tree>> children;
  mutable std::shared_mutex mutex_;
  int level; //!< Level of the node in the tree, 0 for survey root, 1 for stations, 2 for runs
  fs::path basePath;

  // For level 2 nodes, hold a single run_d instance
  std::shared_ptr<run_d> run;

public:
  survey_tree(const std::string name, std::shared_ptr<survey_tree> parent = nullptr) :
      name(name),
      parent(parent),
      level(parent ? parent->level + 1 : 0),
      basePath(parent ? parent->get_path() / name : fs::path{name}) {
    if (level > 2) {
      throw std::runtime_error("Only 3 levels allowed");
    }

    std::error_code ec;
    fs::create_directories(basePath, ec);
    if (ec) {
      std::cerr << "Failed to create directory: " << basePath << " (" << ec.message() << ")\n";
    }
    if (level == 0) {
      bool is_created = false;
      // we take care of the root directory.
      // a) if empty, create_survey_dirs(basePath, survey_dirs()); and / stations
      if (fs::is_empty(basePath)) {
        // Directory is empty, create survey directories and stations
        create_survey_dirs(basePath, survey_dirs());
        is_created = true;
      }
      // b) if not empty, check for basePath / stations
      if (!fs::exists(basePath / "stations") || !fs::is_directory(basePath / "stations")) {
        throw std::runtime_error("Survey directory is not properly initialized");
      } else {
        this->basePath = basePath / "stations";
      }
    }

    // For level 2, run will be set when a run_d is created
    if (level == 2) {
      run = std::make_shared<run_d>(basePath);
    }
  }
  void scan() {
    if (level != 0) {
      return;
    }
    std::unique_lock lock(mutex_);
    for (const auto &entry : fs::directory_iterator(basePath)) {
      // station dirs, level 1
      if (entry.is_directory()) {
        std::string childName = entry.path().filename().string();
        children[childName] = std::make_shared<survey_tree>(childName, shared_from_this());
        std::cout << "Found child: " << childName << " in " << get_path() << "\n";
        // If this is a level 2 directory the runs are initialized by the constructor
        // no need to check for runs here!
      }
    }
    // now scan the stations for runs
    for (const auto &[childName, child] : children) {
      if (child->level == 1) {
        std::cout << "Scanning runs in: " << child->get_path() << "\n";
        for (const auto &entry : fs::directory_iterator(child->get_path())) {
          if (entry.is_directory()) {
            std::string runName = entry.path().filename().string();
            if (std::regex_match(runName, std::regex(R"(run_(\d{3}))"))) {
              std::cout << "Found run: " << runName << " in " << child->get_path() << "\n";
            }
          }
        }
      }
    }
  }

  std::shared_ptr<survey_tree> add_child(const std::string &childName) {
    std::unique_lock lock(mutex_);
    if (children.contains(childName)) {
      return children.at(childName);
    }

    auto newChild = std::make_shared<survey_tree>(childName, shared_from_this());
    children[childName] = newChild;
    return newChild;
  }
  std::shared_ptr<survey_tree> get_child(const std::string &childName) const {
    std::shared_lock lock(mutex_);
    auto it = children.find(childName);
    if (it != children.end()) {
      return it->second;
    }
    return nullptr; // Child not found
  }

  std::shared_ptr<survey_tree> add_child(int id) {
    if (level >= 2) {
      throw std::logic_error("Cannot add numeric children beyond level 2");
    }
    std::string childName = (level == 0)
                                ? std::format("{}", id)
                                : std::format("run_{:03}", id);
    return add_child(childName);
  }

  std::shared_ptr<survey_tree> get_child(const int &id) const {
    if (level >= 2) {
      throw std::logic_error("Cannot get numeric children beyond level 2");
    }
    std::string childName = (level == 0)
                                ? std::format("{}", id)
                                : std::format("run_{:03}", id);
    return get_child(childName);
  }

  std::shared_ptr<survey_tree> addAutoRun(const size_t min_run_id = 0, bool scanDisk = false) {
    int nextId = getNextRunId(min_run_id, scanDisk);
    return add_child(nextId);
  }

  size_t getNextRunId(const size_t min_run_id = 0, bool scanDisk = false) const {
    if (level != 1) {
      throw std::logic_error("Auto-run IDs only valid at level 1 (test directories)");
    }

    int maxId = -1;
    std::regex runPattern(R"(run_(\d{3}))");

    if (scanDisk) {
      for (const auto &entry : fs::directory_iterator(get_path())) {
        if (entry.is_directory()) {
          std::smatch match;
          std::string name = entry.path().filename().string();
          if (std::regex_match(name, match, runPattern)) {
            int id = std::stoi(match[1]);
            maxId = std::max(maxId, id);
          }
        }
      }
    } else {
      std::shared_lock lock(mutex_);
      for (const auto &[childName, _] : children) {
        std::smatch match;
        if (std::regex_match(childName, match, runPattern)) {
          int id = std::stoi(match[1]);
          maxId = std::max(maxId, id);
        }
      }
    }
    if (maxId == -1) {
      maxId = 1; // No runs found, start from 1
    } else {
      maxId++; // Increment to get the next ID
    }
    // Return the next ID, incrementing the maximum found
    if (min_run_id > 0) {
      return std::max(static_cast<size_t>(maxId), min_run_id);
    }
    return maxId;
  }

  void set_run(std::shared_ptr<run_d> runObj) {
    if (level != 2) {
      throw std::logic_error("Only level-2 directories can attach run_d instances");
    }
    std::unique_lock lock(mutex_);
    run = std::move(runObj);
  }

  std::shared_ptr<run_d> get_run() const {
    if (level != 2)
      return nullptr;
    std::shared_lock lock(mutex_);
    return run;
  }

  std::shared_ptr<run_d> get_run(const std::string &station, const size_t &run_no) const {
    if (level != 0) {
      throw std::logic_error("get_run() only valid at level 0 (survey directories)");
    }
    std::shared_lock lock(mutex_);
    auto it = children.find(station);
    if (it != children.end()) {
      auto stationNode = it->second;
      // now we have the station node, continue to find the run by iterating through its children
      for (const auto &[runName, runNode] : stationNode->children) {
        if (runName == std::format("run_{:03}", run_no)) {
          return runNode->get_run();
        }
      }
      return nullptr; // Run not found in the station
    }
    return nullptr; // Station not found
  }

  std::string getFullPath() const {
    if (auto p = parent.lock()) {
      return p->getFullPath() + "/" + name;
    }
    return "/" + name;
  }

  fs::path get_path() const {
    return basePath;
  }
  std::string get_name() const {
    return name;
  }

  void list_children() const {
    std::shared_lock lock(mutex_);
    std::cout << "survey_tree: " << get_path() << " has children:\n";
    for (const auto &[childName, _] : children) {
      std::cout << " - " << childName << "\n";
    }
  }

  void list_children_recursive() const {
    std::shared_lock lock(mutex_);
    // std::cout << "survey_tree: " << get_path() << " has children:\n";
    for (const auto &[childName, child] : children) {
      if (level == 0) {
        std::cout << " -> " << childName << "\n";
      } else {
        std::cout << "   ├─── " << childName << "\n";
      }
      if (child) {
        child->list_children_recursive();
      }
    }
  }
};

#endif // SURVEY_TREE_HPP
