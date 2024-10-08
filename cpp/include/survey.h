#ifndef SURVEY_H
#define SURVEY_H

#include "atss.h" // channels - n channels for ech run
#include "files_dirs.h"
#include "raw_spectra.h"

/*!
 * @file survey.h
 * @brief survey class for the atss project; basically you operate on the file system and many return values are paths!
 */

// ************************************************************************  R U N *******************************************************************

static const size_t prep_channels = 12; //!< assume an amount of channels per run

/*!
 * \brief The run_d class - the main survey class should be in a try block and keep the locks
 * a run contains ONE ENTITY - so one sampling frequency at one start time ONE LOGGER
 */
class run_d {

public:
  /*!
   * \brief run_d
   * \param run_dir create a dir or...
   * \param no_create ... scan a dir (default)
   */
  run_d(const std::filesystem::path &run_dir, const bool no_create = true) : run_dir(run_dir) {

    this->channels.reserve(prep_channels);
    if (!no_create) {
      std::filesystem::create_directory(this->run_dir);
    } else {
      this->scan();
    }
    this->run_dir = std::filesystem::canonical(this->run_dir);
  }

  /*!
   * \brief run_d constructor as deep copy
   * \param rhs shared pointer of a run
   */
  run_d(const std::shared_ptr<run_d> &rhs) {
    this->run_dir = std::filesystem::canonical(rhs->run_dir);
    this->channels.reserve(prep_channels);
    for (const auto &chan : rhs->channels) {
      this->channels.emplace_back(std::make_shared<channel>(chan)); // COPY constructor
    }
  }

  ~run_d() {
    this->clear();
  }

  /*!
   * \brief add_channel
   * \param new_channel
   * \return run path in case of success - otherwise empty path
   */
  std::filesystem::path add_channel(std::shared_ptr<channel> &new_channel) {

    if (!this->channels.size()) {
      new_channel->set_dir(this->run_dir);
      this->channels.push_back(new_channel);
      return this->run_dir;
    } else {
      for (auto &chan : this->channels) {
        if (same_run(chan, new_channel)) {
          new_channel->set_dir(this->run_dir);
          this->channels.push_back(new_channel);
          std::sort(this->channels.begin(), this->channels.end(), compare_channel_name_lt);
          return this->run_dir;
        }
      }
    }
    return std::filesystem::path();
  }
  /*!
   * \brief add_channel_set
   * \param channel_types create a set of channels according to the vector channel_types
   * \param sample_rate and sample rate
   * \return run path
   */
  std::filesystem::path add_channel_set(std::vector<std::string> &channel_types, const double &sample_rate) {
    this->clear(); // clear the channels, avoid mixed up runs
    for (const auto &ch : channel_types) {
      auto chan = std::make_shared<channel>(ch, sample_rate);
      make_channel(chan, sample_rate, chan->get_channel_type());
      chan->set_dir(this->run_dir);
      this->channels.push_back(chan);
      // this->channels.back()->set_dir(this->run_dir);
    }
    return this->run_dir;
  }

  size_t get_run_no() const {
    auto run_no = this->run_dir;
    // remove all until the last /
    run_no = run_no.filename();
    // remove run_ at the beginning
    auto srun_no = run_no.string().substr(4);
    // convert to size_t
    return size_t(stoul(srun_no));
  }
  std::string get_name() const {
    return this->run_dir.filename();
  }

  void clear() {
    for (auto &chan : this->channels) {
      if (chan != nullptr)
        chan.reset();
    }
    this->channels.clear();
    this->raw_spc.reset();
  }

  size_t scan() {
    this->clear();
    this->channels.reserve(prep_channels);
    for (auto const &atssfile : std::filesystem::recursive_directory_iterator(this->run_dir)) {
      if (std::filesystem::is_regular_file(atssfile) && atssfile.path().extension() == ".json") {
        std::filesystem::path atss(atssfile); // construct an atss file from json
        atss.replace_extension(".atss");
        if (std::filesystem::exists(atss)) { // check the corresponding atss
          // std::cout << atssfile << " " << atss.filename() << '\n';
          this->channels.emplace_back(std::make_shared<channel>(atssfile)); // add file
                                                                            // if test ..
        }
      }
      std::sort(this->channels.begin(), this->channels.end(), compare_channel_name_lt);
    }

    return this->channels.size();
  }

  std::shared_ptr<channel> ch_first() const {
    if (!channels.size()) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << " Station " << this->run_dir.parent_path().filename() << " " << this->run_dir.filename() << " is empty";
      throw std::runtime_error(err_str.str());
    }
    return channels.at(0);
  }

  /*!
   * @brief
   * @param chtype like Ex, Ey, Hx, Hy, Hz
   * @return std::shared_ptr<channel> or nullptr
   */
  std::shared_ptr<channel> get_channel(const std::string &chtype) const {
    if (!channels.size()) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << " Station " << this->run_dir.parent_path().filename() << " " << this->run_dir.filename() << " is empty";
      throw std::runtime_error(err_str.str());
    }

    for (auto &ch : this->channels) {
      if (ch->get_channel_type() == chtype)
        return ch;
    }

    return nullptr;
  }

  std::vector<std::shared_ptr<channel>> get_channels() const {
    return this->channels;
  }

  /*!
   * @brief having a vector of channels, find the index of the channel with the same name inside the runs
   * your main program should have the sort order like this:
   * for (const auto &run : runs) {
   *  for (const auto &channel_type : channel_types) {
   * @param channels vector of shared pointers of channels
   * @param name Ex, Ey, Hx, Hy, Hz
   * @return index of the given channel vector or SIZE_MAX
   */
  size_t find_index_of_channel_vector_by_name(const std::vector<std::shared_ptr<channel>> &channels, const std::string &name) const {
    size_t i = 0;
    if (!channels.size())
      return SIZE_MAX;
    if (name.empty())
      return SIZE_MAX;

    auto find_me = this->get_channel(name); // shared pointer of this run and channel name
    // find the index of the reference channel using the == operator from atss.h for shared pointers of channel
    auto it = std::find_if(channels.begin(), channels.end(), [&find_me](const std::shared_ptr<channel> &chan) { return chan == find_me; });
    if (it != channels.end()) {
      i = std::distance(channels.begin(), it);
    } else {
      i = SIZE_MAX;
    }
    return i;
  }

  bool check_sample_rate(const double &sample_rate) const {
    if (!this->channels.size())
      return false;
    if (this->channels.at(0)->get_sample_rate() == sample_rate)
      return true;
    return false;
  }

  void ls() const {
    size_t i = 0;
    for (const auto &ch : channels) {
      ++i;
      if (i != channels.size()) {
        std::cout << "    "
                  << "│   "
                  << "│   "
                  << "├"
                  << "───" << ch->filename() << "   " << ch->brief() << std::endl;
      } else {
        std::cout << "    "
                  << "│   "
                  << "│   "
                  << "└"
                  << "───" << ch->filename() << "   " << ch->brief() << std::endl;
      }
    }
  }

  void init_raw_spectra(std::shared_ptr<BS::thread_pool> &pool) {

    if (!this->channels.size()) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << " Station " << this->run_dir.parent_path().filename() << " " << this->run_dir.filename() << " is empty";
      throw std::runtime_error(err_str.str());
    }
    this->raw_spc = std::make_shared<raw_spectra>(pool, this->channels);
  }

  void fetch_raw_spectra() {
    if (!this->channels.size()) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << " Station " << this->run_dir.parent_path().filename() << " " << this->run_dir.filename() << " is empty";
      throw std::runtime_error(err_str.str());
    }
    if (this->raw_spc == nullptr) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << " Station " << this->run_dir.parent_path().filename() << " " << this->run_dir.filename() << " raw_spc is nullptr";
      throw std::runtime_error(err_str.str());
    }
    for (auto &ch : this->channels) {
      if (ch->spc.size())
        this->raw_spc->move_raw_spectra(ch); // move the raw spectra from the channel to the raw_spectra
                                             // raw spectra also contains the channel pointer and therewith the channel name and FFT properties
    }
  }

  // std::vector<size_t> get_channel_with_spectra(bool parzen_spectra) const {
  //   std::vector<size_t> channels_with_spectra;
  //   for (size_t i = 0; i < this->channels.size(); ++i) {
  //     std::string ch_type = this->channels.at(i)->get_channel_type();
  //     if (parzen_spectra) {
  //       if (raw_spc->get_abs_sa_prz_spectra(ch_type).size())
  //         channels_with_spectra.push_back(i);
  //     } else if (this->raw_spc->get_abs_sa_spectra(ch_type).size())
  //       channels_with_spectra.push_back(i);
  //   }
  //   return channels_with_spectra;
  // }

  std::filesystem::path run_dir;
  std::vector<std::shared_ptr<channel>> channels;
  std::shared_ptr<raw_spectra> raw_spc; //!< raw spectra for the run - contains Ex, Ey, Hx, Hy, Hz, REx, REy, RHx, RHy, RHz, EEx, EEy (emap)
};

bool operator==(const std::shared_ptr<run_d> &lhs, const std::shared_ptr<run_d> &rhs) {
  if (lhs->run_dir != rhs->run_dir)
    return false;
  if (lhs->channels.size() != rhs->channels.size())
    return false;
  std::vector<std::shared_ptr<channel>> channels_lhs(lhs->channels);
  std::vector<std::shared_ptr<channel>> channels_rhs(rhs->channels);
  std::sort(channels_lhs.begin(), channels_lhs.end(), compare_channel_name_lt);
  std::sort(channels_rhs.begin(), channels_rhs.end(), compare_channel_name_lt);

  for (size_t i = 0; i < channels_lhs.size(); ++i) {
    if (channels_lhs.at(i) != channels_rhs.at(i))
      return false;
  }

  return true;
}

// ************************************************************************ S T A T I O N *******************************************************************
//
//
//
//
//
//
//
//
/*!
 * \brief The station_d class
 */
class station_d {
public:
  /*!
   * \brief station_d
   * \param station_dir
   * \param no_create if true, we perform a site scan
   */
  station_d(const std::filesystem::path &station_dir, const bool no_create = true) : station_dir(station_dir) {

    if (!no_create) {
      std::filesystem::create_directories(this->station_dir);
    } else {
      this->scan();
    }
    this->station_dir = std::filesystem::canonical(this->station_dir);
  }

  /*!
   * \brief station_d constructor as deep copy
   * \param rhs
   */
  station_d(const std::shared_ptr<station_d> &rhs) {

    this->station_dir = std::filesystem::canonical(rhs->station_dir);

    this->runs.reserve(rhs->runs.size());
    for (const auto &r : rhs->runs) {
      this->runs.emplace_back(std::make_shared<run_d>(r));
    }
  }

  ~station_d() {
    this->clear();
  }

  void clear() {
    for (auto &run : this->runs) {
      if (run != nullptr) {
        run.reset();
      }
    }
    this->runs.clear();
  }

  size_t scan() {

    this->clear();
    for (const auto &entry : std::filesystem::directory_iterator(station_dir)) {
      std::cout << entry.path() << std::endl;
      if (std::filesystem::is_directory(entry)) {
        auto is_run_no = mstr::string2run(entry.path().filename().string());
        if (is_run_no != SIZE_MAX)
          runs.push_back(std::make_shared<run_d>(entry));
      }
    }

    return runs.size();
  }

  /*!
   * \brief scan_free_run_no
   * \return first free run number inside the station directory
   */
  size_t scan_free_run_no() const {
    return fdirs::scan_runs(this->station_dir);
  }

  size_t scan_high_run_no() const {
    return fdirs::scan_runs_high(this->station_dir);
  }

  std::filesystem::path add_channel_set(std::vector<std::string> &channel_types, const double &sample_rate) {
    std::filesystem::path runpath;
    auto irun = this->scan_high_run_no();
    if (irun == SIZE_MAX)
      return std::filesystem::path();
    runpath = this->station_dir / mstr::run2string(irun);

    runs.push_back(std::make_shared<run_d>(runpath, false));
    runpath = runs.back()->add_channel_set(channel_types, sample_rate);

    std::filesystem::path mrunpath = fdirs::meta_dir(this->station_dir) / mstr::run2string(irun);
    if (!std::filesystem::exists(mrunpath))
      std::filesystem::create_directory(mrunpath);

    return runpath;
  }

  std::filesystem::path add_create_run(std::shared_ptr<channel> &new_channel) {

    std::filesystem::path runpath;

    for (auto &run : this->runs) {
      runpath = run->add_channel(new_channel);
    }

    if (runpath.empty()) {
      auto irun = this->scan_free_run_no();
      if (irun == SIZE_MAX)
        return std::filesystem::path();
      runpath = this->station_dir / mstr::run2string(irun);

      runs.push_back(std::make_shared<run_d>(runpath, false));
      runpath = runs.back()->add_channel(new_channel);

      std::filesystem::path mrunpath = fdirs::meta_dir(this->station_dir) / mstr::run2string(irun);
      if (!std::filesystem::exists(mrunpath))
        std::filesystem::create_directory(mrunpath);
    }

    return runpath;
  }

  std::shared_ptr<run_d> get_run(const size_t &run_no) const {
    std::filesystem::path spath;
    auto srun = mstr::run2string(run_no);
    spath = this->station_dir / srun;

    if (!std::filesystem::exists(spath)) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << ":: run does not exists! " << this->station_dir.filename() << " wanted: " << srun;
      throw std::runtime_error(err_str.str());
    } else {
      for (const auto &r : this->runs) {
        if (r->get_run_no() == run_no)
          return r;
      }
    }
    return nullptr;
  }
  std::shared_ptr<run_d> get_run(const std::filesystem::path &run_path) const {
    return this->get_run(mstr::string2run(run_path.filename().string()));
  }

  // operator (size_t run_no, std::string channel_type)
  /*!
   * \brief at size_t run_no, std::string channel_type
   * \param run_no
   * \param channel_type
   * \return
   */
  std::shared_ptr<channel>
  at(const size_t &run_no, const std::string &channel_type) const {

    auto tmp_run = this->get_run(run_no);
    if (tmp_run == nullptr) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << ":: run does not exists! " << this->station_dir.filename() << " wanted: " << run_no;
      throw std::runtime_error(err_str.str());
    }
    return tmp_run->get_channel(channel_type);
  }

  std::vector<std::shared_ptr<run_d>> get_sample_rate(const double &sample_rate) const {
    std::vector<std::shared_ptr<run_d>> runs_s;
    for (const auto &run : runs) {
      if (run->check_sample_rate(sample_rate))
        runs_s.emplace_back(run);
    }
    return runs_s;
  }

  void ls() const {
    size_t i = 0;
    for (const auto &run : runs) {
      ++i;
      if (i != runs.size()) {
        std::cout << "    "
                  << "│   "
                  << "├"
                  << "───" << run->run_dir.filename().string();
        if (run->channels.size())
          std::cout << std::endl;
        else
          std::cout << " (empty) " << std::endl;
      } else {
        std::cout << "    "
                  << "│   "
                  << "└"
                  << "───" << run->run_dir.filename().string();
        if (run->channels.size())
          std::cout << std::endl;
        else
          std::cout << " (empty) " << std::endl;
      }
      run->ls();
    }
  }

  std::string get_name() const {
    return this->station_dir.filename();
  }

  std::vector<std::shared_ptr<run_d>> runs;
  std::filesystem::path station_dir;
};

// ************************************************************************ S U R V E Y *******************************************************************
//
//
//
//
//
//
//
//
class survey_d {

public:
  /*!
   * \brief survey_d, put in a try block and catch(catch (std::filesystem::filesystem_error& e) { e.what() } )
   * \param survey_dir
   * \param no_create if true we perform a survey scan and guarantee that we do not write to the filesystem
   * \param no_create == false we add tree_size files to the survey only
   */

  survey_d(const std::filesystem::path &survey_dir, const bool no_create = true, size_t tree_size = 0) : survey_dir(survey_dir) {

    std::unique_lock lock(this->station_lock);
    if (!no_create) {
      // if (!std::filesystem::exists(this->survey_dir)) {
      std::filesystem::create_directories(this->survey_dir);
      this->survey_dir = std::filesystem::canonical(this->survey_dir);
      create_survey_dirs(this->survey_dir, survey_dirs());
      //}
    }

    this->survey_dir = std::filesystem::canonical(this->survey_dir);
    this->scan();

    if (tree_size)
      this->all_channels.reserve(tree_size);
  }

  /*!
   * \brief survey_d deep copy constructor
   * \param rhs
   */
  survey_d(const std::shared_ptr<survey_d> &rhs) {

    this->survey_dir = std::filesystem::canonical(rhs->survey_dir);
    this->stations.reserve(rhs->stations.size());
    for (const auto &s : rhs->stations) {
      this->stations.emplace_back(std::make_shared<station_d>(s));
    }
    for (const auto &chan : rhs->all_channels) {
      this->all_channels.emplace_back(std::make_shared<channel>(chan));
    }
  }

  void collect(const std::shared_ptr<channel> &chan) {
    std::unique_lock lock(this->station_lock);
    this->all_channels.push_back(chan);
  }

  void mk_tree() {

    std::sort(this->all_channels.begin(), this->all_channels.end(), compare_channel_start_lt);
    if (!std::filesystem::exists(this->survey_dir)) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << " Survey dir " << this->survey_dir.string() << " does not exists";
      throw std::runtime_error(err_str.str());
    }
    std::map<std::string, int> all_stations;
    int i = 0;
    for (const auto &ch : this->all_channels) {
      all_stations.emplace(ch->tmp_station, i++);
    }
    i = 0;
    for (const auto &s : all_stations) {
      auto station = this->create_station(s.first);
      for (auto &ch : this->all_channels) {
        if (station.filename() == ch->tmp_station) {
          this->add_create_run(station, ch);
          if (ch->filepath_wo_ext.empty()) {
            std::ostringstream err_str(__func__, std::ios_base::ate);
            err_str << ":: NO PATH for channel CREATED ";
            throw std::runtime_error(err_str.str());
          }
        }
      }
    }
  }

  std::shared_ptr<channel> get_channel_from_all(const size_t &index) const {
    std::shared_lock lock(this->station_lock);
    return this->all_channels.at(index);
  }

  std::vector<std::shared_ptr<channel>> get_all_channels() const {
    std::shared_lock lock(this->station_lock);
    return this->all_channels;
  }

  size_t nchannels() const {
    std::shared_lock lock(this->station_lock);
    return this->all_channels.size();
  }

  ~survey_d() {

    this->clear();
  }

  /*!
   * \brief clear the stations vector; this is performed before a scan; scan is performed in the constructor and does NOT call a LOCK
   */
  void clear() {
    for (auto &station : this->stations) {
      if (station != nullptr) {
        station.reset();
      }
    }
    this->stations.clear();
  }
  /*!
   * \brief get_station, scans the file tree and returns a shared pointer to a station
   * \param station_name
   * \return a shared pointer to a station or throw an exception
   */
  std::shared_ptr<station_d> get_station(const std::string &station_name) const {
    std::shared_lock lock(this->station_lock); // can not add for example
    std::filesystem::path spath;
    spath = this->survey_dir / "stations" / station_name;
    // lamda
    auto stat = std::find_if(this->stations.begin(), this->stations.end(), [spath](const std::shared_ptr<station_d> s) { return s->station_dir == spath; });
    if (stat == stations.end()) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << " Station " << station_name << " does not exists";
      throw std::runtime_error(err_str.str());
    }
    return *stat;
  }

  /*!
   * @brief get_station - get a station by path, see above
   * @param station_path
   * @return shared pointer to a station or nullptr
   */
  std::shared_ptr<station_d> get_station(const std::filesystem::path &station_path) const {
    return this->get_station(station_path.filename().string());
  }

  /*!
   * @brief get the run by station name and run number
   * @param station_name
   * @param run_no
   * @return shared pointer to a run or nullptr
   */
  std::shared_ptr<run_d> get_run(const std::string &station_name, const size_t &run_no) const {
    auto station = this->get_station(station_name);
    return station->get_run(run_no);
  }

  /*!
   * @brief get the run by station name and run number - overloaded for convenience
   * @param station_name_run_no as pair
   * @return shared pointer to a run or nullptr
   */
  std::shared_ptr<run_d> get_run(const std::pair<std::string, size_t> &station_name_run_no) const {
    auto station = this->get_station(station_name_run_no.first);
    return station->get_run(station_name_run_no.second);
  }
  std::shared_ptr<channel> at(const std::string &station_name, const size_t &run_no, const std::string &channel_type) const {
    auto station = this->get_station(station_name);
    return station->at(run_no, channel_type);
  }

  std::vector<std::string> get_station_names() const {
    std::shared_lock lock(this->station_lock);
    std::vector<std::string> station_names;
    for (const auto &station : this->stations) {
      station_names.push_back(station->station_dir.filename());
    }
    return station_names;
  }

  std::shared_ptr<channel> get_first_channel(const std::string &station_name, const size_t &run_no) const {

    auto station = this->get_station(station_name);
    // check nullptr ... ? use catch!
    auto run = station->get_run(run_no);
    return run->ch_first();
  }

  void hide_station(const std::string &station_name) {
    try {

      std::unique_lock lock(this->station_lock);
      auto iter = this->stations.begin();
      while (iter != this->stations.end()) {
        // remove strings having length 5
        if ((*iter)->station_dir.filename() == station_name) {
          iter = this->stations.erase(iter);
        } else {
          ++iter;
        }
      }
    } catch (...) {
      std::cerr << "can not hide" << std::endl;
    }
  }

  std::filesystem::path add_create_run(const std::filesystem::path &station_path, std::shared_ptr<channel> &new_channel) {

    std::unique_lock lock(this->station_lock);

    for (auto &station : this->stations) {
      if (station->station_dir == station_path)
        return station->add_create_run(new_channel);
    }
    return std::filesystem::path();
  }

  std::filesystem::path create_station(const std::string &station_name) {
    std::unique_lock lock(this->station_lock);
    std::filesystem::path spath;

    spath = this->survey_dir / "stations" / station_name;
    // lamda
    if (std::find_if(this->stations.begin(), this->stations.end(), [spath](const std::shared_ptr<station_d> s) { return s->station_dir == spath; }) != stations.end()) {

      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << " Station " << station_name << " already exists";
      throw std::runtime_error(err_str.str());
    }

    std::filesystem::path mpath = this->survey_dir / "meta" / station_name;

    if (!std::filesystem::exists(spath))
      this->stations.emplace_back(std::make_shared<station_d>(spath, false));
    else {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << " Station " << station_name << " already exists in filesystem - you can't create it";
      throw std::runtime_error(err_str.str());
    }
    if (!std::filesystem::exists(mpath))
      std::filesystem::create_directory(mpath);

    return spath;
  }

  std::filesystem::path survey_path() const {
    std::shared_lock lock(this->station_lock);
    return this->survey_dir;
  }

  void ls() const {
    std::shared_lock lock(this->station_lock);
    std::cout << std::endl
              << survey_dir.parent_path().string() << " contains:" << std::endl;
    std::cout << survey_dir.filename().string() << std::endl;
    size_t i = 0;
    for (const auto &station : stations) {
      ++i;
      if (i != stations.size()) {
        std::cout << "    "
                  << "├"
                  << "───" << station->station_dir.filename().string();
        if (station->runs.size())
          std::cout << std::endl;
        else
          std::cout << " (empty) " << std::endl;
      } else {
        std::cout << "    "
                  << "└"
                  << "───" << station->station_dir.filename().string();
        if (station->runs.size())
          std::cout << std::endl;
        else
          std::cout << " (empty) " << std::endl;
      }
      station->ls();
    }
  }

  std::string get_name() const {
    std::shared_lock lock(this->station_lock);
    return this->survey_dir.filename();
  }

  std::pair<std::string, size_t> get_station_run(const std::filesystem::path &filepath) const {
    std::shared_lock lock(this->station_lock);
    std::string station_name;
    size_t run_no = SIZE_MAX;
    for (const auto &station : stations) {
      for (const auto &run : station->runs) {

        station_name = station->station_dir.filename();
        run_no = run->get_run_no();
        if (filepath == run->run_dir)
          return std::make_pair(station_name, run_no);
      }
    }

    return std::make_pair(station_name, run_no);
  }

  std::filesystem::path add_station_auto_num(const std::string &pattern = "s_") {
    std::unique_lock lock(this->survey_lock);
    size_t i = 1;
    std::string station_name;
    do {
      station_name = pattern + std::to_string(i++);
    } while (std::filesystem::exists(this->survey_dir / "stations" / station_name));
    return this->create_station(station_name);
  }

private:
  size_t scan() {
    try {
      this->clear();
      std::filesystem::path spath = this->survey_dir / "stations";
      for (const auto &entry : std::filesystem::directory_iterator(spath)) {
        // std::cout << entry.path() << std::endl;
        if (std::filesystem::is_directory(entry)) {
          // no create == true -> we scan the station
          this->stations.emplace_back(std::make_shared<station_d>(entry, true));
        }
      }
    } catch (std::filesystem::filesystem_error &e) {
      std::cerr << e.what() << std::endl;
    }
    return this->stations.size();
  }

  std::vector<std::shared_ptr<station_d>> stations;
  std::vector<std::shared_ptr<channel>> all_channels;

  std::filesystem::path survey_dir;
  mutable std::shared_mutex station_lock;
  mutable std::shared_mutex survey_lock;
};

void survey_tokens(const std::filesystem::path &filepath, std::string &survey_name, std::string &station_name, size_t &run_no) {

  // split all by directory separator and get the last four tokens
  std::vector<std::string> tokens;
  auto survey_dir(filepath);
  for (const auto &token : survey_dir) {
    tokens.push_back(token.string());
  }
  survey_name = tokens.at(tokens.size() - 4);
  station_name = tokens.at(tokens.size() - 2);
  auto run_str = tokens.back();
  // remove run_ at the beginning
  run_no = std::stoul(run_str.substr(4));
}

#endif // SURVEY_H
