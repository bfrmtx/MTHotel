#ifndef SURVEY_TREE_CMDLINE_HPP
#define SURVEY_TREE_CMDLINE_HPP

// command line run configuration ******************************************************************************

/// @brief contains the run number and the channel numbers for that run
/// @details remind that we can Hx for example at channel number 2 and 5 in the same run, so we have to use channel numbers to identify channels
class run_config {
public:
  run_config() : channel_numbers(max_survey_channels, false) {}

  /// @brief we create a run_config from command line arguments
  /// @param it which is at "-r" position
  /// @param margs expected pattern: -r <run_no> -c <channels...>; parsing stops at next switch or end
  /// @return iterator positioned at next switch ("-r"/"-s"/end)
  std::list<std::string>::iterator create_run_config(std::list<std::string>::iterator it, std::list<std::string> &margs) {
    // check that we get "-r" otherwise throw
    if (it == margs.end() || *it != "-r") {
      throw std::runtime_error("Expected -r at the beginning of run_config");
    }
    it = margs.erase(it); // remove -r
    if (it == margs.end() || it->starts_with('-')) {
      throw std::runtime_error("No run number provided after -r");
    }
    try {
      this->run_no = std::stoul(*it);
      it = margs.erase(it); // remove the processed argument and get the next iterator
    } catch (const std::exception &e) {
      throw std::runtime_error("Invalid run number provided after -r");
    }
    it = parse_channel_args(it, margs);
    return it; // return iterator at next "-" or end of margs; for multiple runs it should be "-r" or "-s" if the next station comes
  }
  void add_channel_numbers(const std::vector<size_t> &chnums) {
    for (const auto chnum : chnums) {
      add_channel_number(chnum);
    }
  }
  std::list<std::string>::iterator add_channel_numbers(std::list<std::string>::iterator it, std::list<std::string> &margs) {
    return parse_channel_args(it, margs);
  }
  void add_channel_number(const size_t &chnum) {
    if (chnum >= max_survey_channels) {
      throw std::runtime_error("Channel number exceeds maximum supported channels");
    }
    channel_numbers[chnum] = true;
  }

  size_t run_no = SIZE_MAX;
  std::vector<bool> channel_numbers;

  std::list<std::string>::iterator parse_channel_args(std::list<std::string>::iterator it, std::list<std::string> &margs) {
    if (it == margs.end() || *it != "-c") {
      throw std::runtime_error("Expected -c after run number");
    }
    it = margs.erase(it); // remove -c
    if (it == margs.end() || it->starts_with('-')) {
      throw std::runtime_error("No channel numbers provided after -c");
    }

    while (it != margs.end() && !it->starts_with('-')) {
      size_t chnum = 0;
      try {
        chnum = std::stoul(*it);
      } catch (const std::exception &e) {
        throw std::runtime_error("Invalid channel number provided after -c");
      }
      if (chnum >= max_survey_channels) {
        throw std::runtime_error("Channel number exceeds maximum supported channels");
      }
      channel_numbers[chnum] = true;
      it = margs.erase(it); // remove the processed argument and get the next iterator
    }

    return it;
  }
};

class station_config {
public:
  /// @brief Default constructor for station_config
  station_config() = default;

  /// @brief Construct a station_config with a given station name
  /// @param name The name of the station
  station_config(const std::string &name) : station_name(name) {}

  void set_station_name(const std::string &name) {
    station_name = name;
  }

  void clone_from(const station_config &other, std::string new_station_name = "") {
    if (!new_station_name.empty()) {
      station_name = new_station_name;
    } else {
      station_name = other.station_name;
    }
    runs = other.runs;
  }

  /// @brief Create a station configuration from command line arguments
  /// @param it Iterator pointing to the current position in the argument list
  /// @param margs List of command line arguments
  /// @return Iterator pointing to the next unprocessed argument
  std::list<std::string>::iterator create_station_config(std::list<std::string>::iterator it, std::list<std::string> &margs) {
    if (it == margs.end() || it->starts_with('-')) {
      throw std::runtime_error("No station name provided after -s");
    }
    station_name = *it;
    it = margs.erase(it); // remove the processed argument and get the next iterator
    // now we get one or more runs like "-r 1 -c 2 3 4 -r 2 -c 2 3 4"
    while (it != margs.end() && *it == "-r") {
      run_config rc;
      it = rc.create_run_config(it, margs);
      runs[rc.run_no] = rc.channel_numbers;
    }
    return it;
  }
  void add_run_config(const run_config &rc) {
    runs[rc.run_no] = rc.channel_numbers;
  }

  void add_run(size_t run_number, const std::vector<bool> &channel_flags) {
    if (channel_flags.size() != max_survey_channels) {
      throw std::runtime_error("Channel flag vector size mismatch");
    }
    runs[run_number] = channel_flags;
  }

  void add_to_station_run_channels(std::map<std::string, std::map<size_t, std::vector<bool>>> &station_run_channels) const {
    station_run_channels[station_name] = runs;
  }

  std::string station_name;
  std::map<size_t, std::vector<bool>> runs; ///< run_number → channel selection flags
};

// std::map<std::string, std::map<size_t, std::vector<bool>>> &station_run_channels

// After parsing: -s Station1 -r 1 -c 0 1 2 -r 2 -c 0 1 2 -s Station2 -r 1 -c 0 1
// The selective survey will only contain:
// - Station1/run_001: channels[0], channels[1], channels[2] (others are nullptr)
// - Station1/run_002: channels[0], channels[1], channels[2] (others are nullptr)
// - Station2/run_001: channels[0], channels[1] (others are nullptr)

#endif // SURVEY_TREE_CMDLINE_HPP