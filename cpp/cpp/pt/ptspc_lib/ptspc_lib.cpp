#include "ptspc_lib.hpp"
#include <algorithm>
#include <set>

// constructor
ptspc_lib::ptspc_lib(std::shared_ptr<BS::thread_pool<BS::tp::none>> pool_in) {
  this->pool = pool_in;
  // get the executable path
  this->ptspc_path = get_exec_dir();
  // cd up to bin
  ptspc_path = ptspc_path.parent_path().parent_path();
  // cd to doc
  ptspc_path = ptspc_path / "data";
  fs::path sqlfile = ptspc_path / "info.db";
  fs::path master_cal_db = ptspc_path / "master_calibration.db";

  // Check if the SQL file exists and throw an error if it does not

  if (!fs::exists(master_cal_db)) {
    std::cerr << "could not find " << master_cal_db.string() << std::endl;
    throw std::runtime_error("Master calibration database not found.");
  }

  if (!fs::exists(sqlfile)) {
    std::cerr << "could not find " << sqlfile.string() << std::endl;
    throw std::runtime_error("SQL file not found.");
  }
}

// get the options from the GUI or command line arguments "my arguments == margs"
void ptspc_lib::get_options(const std::list<std::string> &args, const bool &has_gui) {
  std::list<std::string> margs = args;
  this->has_gui = has_gui;
  // we iterate over the arguments and delete them when processed
  // we start with options that require no arguments
  auto it = margs.begin();
  while (it != margs.end()) {
    if (*it == "-sb") {
      same_base = true;
      it = margs.erase(it);
    } else if (*it == "-m6") { // limited frequency range for MFS-06e
      magnify_06e = true;
      it = margs.erase(it);
    } else if (*it == "-m7") { // limited frequency range for MFS-07e
      magnify_07e = true;
      it = margs.erase(it);
    } else if (*it == "-rwy") {
      railway = true;
      it = margs.erase(it);
    } else if (*it == "-lowres") {
      lowres = true;
      it = margs.erase(it);
    } else if (*it == "-highres") {
      highres = true;
      it = margs.erase(it);
    } else if (*it == "-i") {
      inner_range = true;
      it = margs.erase(it);
    } else if (*it == "-cplt") { // activate calibration plots AND CALIBRATION
      no_cal_plot = false;
      it = margs.erase(it);
    } else if (*it == "-syscal") {
      syscal = true;
      it = margs.erase(it);
    } else if (*it == "-n") { // normalize the calibration amplitude by f (old style)
      normalize = true;
      it = margs.erase(it);
    } else if (*it == "-sm") { // smooth the spectra with a parzen window
      smooth = true;
      it = margs.erase(it);
    } else if (*it == "-aaspc") { // all auto spectra (so HxHx for same channel)
      all_auto_spectra = true;
      it = margs.erase(it);
    } else {
      ++it;
    }
  }
  // now options which require one or more arguments; next argument will start with -
  it = margs.begin();
  while (it != margs.end()) {
    if (*it == "-u") {
      it = margs.erase(it); // erase -u; next is survey name, because erase moves the iterator forward
      if (it == margs.end() || it->starts_with("-")) {
        throw std::runtime_error("No survey name provided after -u");
      }
      fs::path survey_name = *it;
      if (!fs::is_directory(survey_name)) {
        std::ostringstream err_str(__func__, std::ios_base::ate);
        err_str << " -u survey_dir needs an existing directory with stations inside" << std::endl;
        err_str << " given: " << survey_name.string() << std::endl;
        throw std::runtime_error(err_str.str());
      }
      survey_name = fs::canonical(survey_name);
      survey_tmp = std::make_shared<survey_tree_d>(survey_name, this->pool, run_digits, true); // read-only mode first
      if (survey_tmp == nullptr) {
        std::ostringstream err_str(__func__, std::ios_base::ate);
        err_str << " first use -u survey_name in order to init the survey";
        throw std::runtime_error(err_str.str());
      }
      this->survey_path = survey_name;
      it = margs.erase(it);   // erase survey name
    } else if (*it == "-s") { // get one or more station names
      it = margs.erase(it);   // erase -s
      station_config config;
      it = config.create_station_config(it, margs);
      station_configs.emplace_back(std::move(config));
    } else {
      ++it;
    }
  }

  // enable / disable both lines in case for debugging
  // survey_tmp->scan(true, true); // scan the survey directory recursively for stations and runs; you don't need that for select_only
  // survey_tmp->ls(true, 3);      // list the survey structure with 3 indentation spaces, call scan first
}

void ptspc_lib::process_station_configs(const bool verbose) {

  if (survey_tmp == nullptr) {
    throw std::runtime_error("Survey scanner is not initialized");
  }
  if (station_configs.empty()) {
    // No station configurations to process
    throw std::runtime_error("No station configurations provided, empty");
  }
  this->channels.clear();
  this->runs.clear();
  this->stations.clear();
  if (verbose) {
    std::cout << "SURVEY " << survey_tmp->get_name() << " at " << survey_path.string() << std::endl;
    std::cout << "Processing " << station_configs.size() << " station configurations..." << std::endl;
    std::cout << "Creating selective survey tree based on provided station configurations..." << std::endl
              << std::endl;
  }
  try {
    survey = survey_tmp->select_only(station_configs); // survey_tmp does not need to be scanned before select_only
  } catch (const std::runtime_error &e) {
    std::cerr << "Error during selective survey tree creation: " << e.what() << std::endl;
    throw;
  }
  if (survey == nullptr) {
    throw std::runtime_error("Selective survey tree creation failed, survey is nullptr");
  }
  survey_tmp.reset(); // free temporary survey scanner

  this->runs = survey->get_runs_data();
  for (const auto &run : runs) {
    auto run_channels = run->get_channels();
    this->channels.insert(this->channels.end(), run_channels.begin(), run_channels.end());
  }

  if (verbose) {
    for (const auto &run : runs) {
      // std::cout << run->get_sample_rate() << " Hz" << std::endl;
      std::cout << "station: " << run->get_parent()->get_name() << " run: " << run->get_name() << std::endl;
      std::cout << "Channels:" << std::endl;
      for (const auto &chan : run->get_channels()) {
        // get channels() has no nullptr entries compared to get_channel_vector()
        std::cout << "  " << chan->brief() << std::endl;
      }
      std::cout << std::endl;
    }
  }
}
void ptspc_lib::prepare_fft(const bool verbose) {
  if (survey == nullptr) {
    throw std::runtime_error("Survey is not initialized, use -u survey_name");
  }
  size_t wl, rl;
  for (const auto &run : runs) {
    wl = (size_t)run->get_sample_rate(); // want 1 Hz bandwidth if possible
    rl = wl;
    if (wl < min_wl) {
      if (verbose) {
        std::cout << "window length " << wl << " is less than minimum " << min_wl << ", using minimum" << std::endl;
      }
      wl = min_wl; // use minimum window length, causes padding, 512 and rl = 256 is 256 data and 256 zero padding
      rl = min_rl; // use minimum read length
    }
    // init a fftw for each run - all channels have the same sample rate in each run
    // bandwidth 1 Hz ; during the first loop / irun fft_freqs are created, otherwise copied
    // if nullptr is given, the fft_freqs are created INSIDE the channel (best practice)
    for (const auto &chan : run->get_channels()) { // all channels have the same run numbers
      chan->init_fftw(nullptr, false, wl, rl);
    }
    run->init_raw_spectra(); // this does not start a thread, it just prepares the raw spectra
    if (verbose) {
      std::cout << "Prepared FFTW for run: " << run->get_name() << " with window length: " << wl << " and read length: " << rl << std::endl;
    }
  }
}
void ptspc_lib::process_raw_spectra() {
  // finally calls read_all_fftw(false, nullptr) for each channel
  // and pool->detach_task to read in parallel
  if (survey == nullptr) {
    throw std::runtime_error("Survey is not initialized, use -u survey_name");
  }
  size_t thread_index = 0;
  for (const auto &run : runs) {
    for (const auto &chan : run->get_channels()) {
      try {
        pool->detach_task([chan]() { chan->read_all_fftw(false, nullptr); });
        std::cout << "push thread " << thread_index++ << std::endl;
      } catch (const std::runtime_error &error) {
        std::cerr << error.what() << std::endl;
        std::cerr << "could not execute fftw" << std::endl;
        throw std::runtime_error("Error executing FFTW: " + std::string(error.what()));
      } catch (...) {
        std::cerr << "could not execute fftw" << std::endl;
        throw std::runtime_error("Unknown error executing FFTW");
      }
    }
  }
  pool->wait(); // wait for all tasks to finish, read time series and perform fftw
}

void ptspc_lib::set_inner_outer_frequencies_prepare_spectra() {
  if (survey == nullptr) {
    throw std::runtime_error("Survey is not initialized, use -u survey_name");
  }
  std::vector<int> selected_channels(this->channels.size(), 0);
  int idx = 0;
  for (const auto &chan : this->channels) {
    if (magnify_06e) {
      if (chan->get_sample_rate() > 64000)
        innerouter.set_low_high(chan->fft_freqs->set_lower_upper_f(680, 14000, true)); // cut off spectra; we need these values later
      if (chan->get_sample_rate() < 513)
        innerouter.set_low_high(chan->fft_freqs->set_lower_upper_f(10, 200, true)); // cut off spectra; we need these values later
    } else if (magnify_07e) {
      if (chan->get_sample_rate() > 64000)
        innerouter.set_low_high(chan->fft_freqs->set_lower_upper_f(680, 60000, true)); // cut off spectra; we need these values later
      if (chan->get_sample_rate() < 513)
        innerouter.set_low_high(chan->fft_freqs->set_lower_upper_f(10, 200, true)); // cut off spectra; we need these values later
    } else
      innerouter.set_low_high(chan->fft_freqs->auto_range(0.05, 0.5)); // cut off spectra; we need these values later

    if (no_cal_plot == false) { // we WANT calibration
      // std::cout << "calibration" << std::endl;
      if (use_master_cal) {
        if (master_cal == nullptr) {
          std::ostringstream err_str(__func__, std::ios_base::ate);
          err_str << " no master calibration NULLPTR " << chan->cal->sensor << " " << chan->cal->serial2string() << std::endl;
          std::cerr << err_str.str();
          throw std::runtime_error(err_str.str());
        } else {
          master_cal->get_master_cal(chan->cal);
          chan->cal->set_master_as_caldata();
        }
      }
      if (chan->cal->f.size() == 0) {
        std::ostringstream err_str(__func__, std::ios_base::ate);
        err_str << " no calibration data available for " << chan->cal->sensor << " " << chan->cal->serial2string() << std::endl;
        std::cerr << err_str.str();
      } else {
        // a) want calibration and
        // b) have calibration data
        selected_channels[idx] = 1; // mark as to be calibrated
      }
      ++idx;
    }
  }
  idx = 0;
  for (const auto &chan : this->channels) {
    if (selected_channels[idx] == 1) {
      std::cout << "cal " << chan->cal->sensor << " " << chan->cal->serial2string() << std::endl;
      // chan->cal->interpolate(chan->fft_freqs->get_frequencies()); we thread this
      pool->detach_task([chan]() { chan->cal->interpolate(chan->fft_freqs->get_frequencies()); });
    }
    ++idx;
  }
  pool->wait();
  idx = 0;
  for (const auto &chan : this->channels) {
    if (selected_channels[idx] == 1) {
      // chan->cal->gen_cal_sensor(chan->fft_freqs->get_frequencies()); we thread this
      pool->detach_task([chan]() { chan->cal->gen_cal_sensor(chan->fft_freqs->get_frequencies()); });
    }
    ++idx;
  }
  pool->wait();
  idx = 0;
  for (const auto &chan : this->channels) {
    if (selected_channels[idx] == 1) {
      // chan->cal->join_lower_theo_and_measured_interpolated(); we thread this
      pool->detach_task([chan]() { chan->cal->join_lower_theo_and_measured_interpolated(); });
    }
    ++idx;
  }
  pool->wait();
  for (const auto &chan : this->channels) {
    // now prepare the raw spectra inside the channel object
    // chan->prepare_raw_spc(!no_cal_plot, syscal, true); // this prepares the spc vector of vectors inside the channel object
    pool->detach_task([chan, this]() { chan->prepare_raw_spc(!this->no_cal_plot, this->syscal, true); });
  }
  pool->wait(); // wait for all tasks to finish, prepare raw spectra inside channels
}

void ptspc_lib::move_raw_spectra() {
  if (survey == nullptr) {
    throw std::runtime_error("Survey is not initialized, use -u survey_name");
  }
  for (auto &run : runs) {
    // ---> at this point the calibrated channel raw spectra are MOVED OUT into the run->raw_spc object
    // ---> we have a complex vector of single spectra like <Hx,> <Hy,> which will be used later on
    run->move_raw_spectra(); // fetch the raw spectra from the thread pool; move operation (fast); also initializes the ac_spectra!
    // raw spectra also contains the channel pointer and therewith the channel name and FFT properties
    run->get_raw_spectra()->info();
  }
}

void ptspc_lib::prepare_auto_spectra(const bool verbose) {
  for (auto &run : runs) {
    auto aspc_chans = run->get_raw_spectra()->generate_auto_spectra_channels(verbose);
    for (const auto &name : aspc_chans) {
      run->get_raw_spectra()->sa.prepare_ac_cross_spectra(name);
      run->get_raw_spectra()->sa_prz.prepare_ac_cross_spectra(name);
      run->get_raw_spectra()->sa_avg.prepare_ac_cross_spectra(name);
    }
  }
}

void ptspc_lib::stack_spectra() {
  for (auto &run : runs) {
    std::cout << "stacking auto spectra for run " << run->get_name() << std::endl;
    run->get_raw_spectra()->advanced_stack_all(median_limit);
  }
  pool->wait(); // wait for all tasks to finish
  std::cout << "done" << std::endl;
}
void ptspc_lib::save(const fs::path &top_dir_, const std::string &sub_dir_) {
  if (this->survey == nullptr) {
    throw std::runtime_error("Survey is not initialized, nothing read yet");
  }
  fs::path top_dir = top_dir_;
  std::string sub_dir = sub_dir_;

  if (top_dir.empty()) {
    top_dir = getenv("HOME");
    top_dir /= "dump_spectra";
    try {
      if (!std::filesystem::exists(top_dir)) {
        std::filesystem::create_directory(top_dir);
      }
    } catch (const std::filesystem::filesystem_error &e) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << "::failed to create dump directory " << top_dir << ": " << e.what();
      throw std::runtime_error(err_str.str());
    }
  }

  if (sub_dir.empty()) {
    sub_dir = this->survey->get_name();
  }
  fs::path survey_dir = top_dir / sub_dir;
  survey_dir = fs::absolute(survey_dir);
  std::cout << "Saving survey data to " << survey_dir.string() << std::endl;

  auto result_survey = std::make_shared<survey_tree_d>(this->survey, survey_dir.string());
  // now we have the survey directory, we create station directories inside
  result_survey->copy_create_structure(this->survey); // create the structure in the new survey tree
  // Note: sa, sa_prz, sa_avg contain scalar data (double), not vectors, so we don't call save_ascii on them
  // The spectrum data will be saved via the proper data serialization mechanism
  for (const auto &run : runs) {
    fs::path run_dir = survey_dir / "stations" / run->get_parent()->get_name() / run->get_name();
    pool->detach_task([run, run_dir]() { run->get_raw_spectra()->sa.save_ascii(run_dir); });
  }
  this->pool->wait(); // wait for all tasks to finish
}
/*
void ptspc_lib::collect_and_calibrate() {
  for (const auto &irun : run_numbers) {
    for (const auto &schan : channel_types) {
      for (auto &station : stations) {
        auto chan = station->get_run_data(irun)->get_channel(schan);
        // ***************************** HENCE that here we are really remove frequencies from the fft_freqs *********************************************************
        if (magnify_06e) {
          if (chan->get_sample_rate() > 64000)
            innerouter.set_low_high(chan->fft_freqs->set_lower_upper_f(680, 14000, true)); // cut off spectra; we need these values later
          if (chan->get_sample_rate() < 513)
            innerouter.set_low_high(chan->fft_freqs->set_lower_upper_f(10, 200, true)); // cut off spectra; we need these values later
        } else if (magnify_07e) {
          if (chan->get_sample_rate() > 64000)
            innerouter.set_low_high(chan->fft_freqs->set_lower_upper_f(680, 60000, true)); // cut off spectra; we need these values later
          if (chan->get_sample_rate() < 513)
            innerouter.set_low_high(chan->fft_freqs->set_lower_upper_f(10, 200, true)); // cut off spectra; we need these values later
        } else
          innerouter.set_low_high(chan->fft_freqs->auto_range(0.05, 0.5)); // cut off spectra; we need these values later
        if (no_cal_plot == false) {
          std::cout << "calibration" << std::endl;
          if (use_master_cal) {
            if (master_cal == nullptr) {
              std::ostringstream err_str(__func__, std::ios_base::ate);
              err_str << " no master calibration NULLPTR " << chan->cal->sensor << " " << chan->cal->serial2string() << std::endl;
              std::cerr << err_str.str();
              throw std::runtime_error(err_str.str());
            } else {
              master_cal->get_master_cal(chan->cal);
              chan->cal->set_master_as_caldata();
            }
          }
          if (chan->cal->f.size() == 0) {
            std::ostringstream err_str(__func__, std::ios_base::ate);
            err_str << " no calibration data available for " << chan->cal->sensor << " " << chan->cal->serial2string() << std::endl;
            std::cerr << err_str.str();

          } else {
            chan->cal->interpolate(chan->fft_freqs->get_frequencies());
            chan->cal->gen_cal_sensor(chan->fft_freqs->get_frequencies());
            chan->cal->join_lower_theo_and_measured_interpolated();
          }
        }
        // all spectra a still in a queue, we have to fetch them into a vector of vectors
        // push task can not use default arguments, supply all arguments
        // the prepare_raw_spc pushes the raw spectra queue into a vector spc inside the channel object
        // calibration is done if !no_cal_plot -> so cal_plot is done; true at end means that the fft window calibration is on additionally
        // pool->push_task(&channel::prepare_raw_spc, chan, !no_cal_plot, true);

        // do not use &chan here, because the channel pointer is not valid in the lambda function while the main loop is calling the next channel!
        // ---> at this point the raw spectra queue is calibrated and so and CONVERTED into a vector of vectors INSIDE the CHANNEL object
        bool local_no_cal_plot = this->no_cal_plot;
        bool local_syscal = this->syscal;
        pool->detach_task([chan, local_no_cal_plot, local_syscal]() { chan->prepare_raw_spc(!local_no_cal_plot, local_syscal, true); });
        // after this we have the spc vector of vectors in the channel object
      }
    }
  }
  pool->wait(); // wait for all tasks to finish, collect and calibrate spectra
  for (auto &run : runs) {
    // ---> at this point the calibrated channel raw spectra are MOVED OUT into the run->raw_spc object
    // ---> we have a complex vector of single spectra like <Hx,> <Hy,> which will be used later on
    run->fetch_raw_spectra(); // fetch the raw spectra from the thread pool; move operation (fast); also initializes the ac_spectra!
    // raw spectra also contains the channel pointer and therewith the channel name and FFT properties
  }
  pool->wait(); // wait for all tasks to finish, fetch raw spectra from the channels into the runs
}

void ptspc_lib::run_info_console() {
  for (const auto &run : runs) {
    run->raw_spc->info();
  }
}

void ptspc_lib::stack_ac_spectra() {
  //!< @todo should I use high res coherence and noise?
  for (auto &run : runs) {
    std::cout << "setting auto- and cross- spectra ";
    for (auto &ac : auto_cross_spectra_names) {
      std::cout << "preparing " << ac.first << ac.second << " ";
      run->raw_spc->sa.prepare_ac_cross_spectra(run->get_channel_pair(ac));     // we need the auto spectra for the parzen stack; cost is low
      run->raw_spc->sa_prz.prepare_ac_cross_spectra(run->get_channel_pair(ac)); // we may need the parzen spectra later; cost is low
      run->raw_spc->sa_avg.prepare_ac_cross_spectra(run->get_channel_pair(ac)); // we may need the average spectra later; cost is low
      // coherence and noise will auto prepare from EXISTING
    }
    std::cout << std::endl;
  }
  std::cout << "stacking" << std::endl;
  thread_index = 0;
  for (auto &run : runs) {
    // run raw spectra fires up all channels; USE wait_for_tasks() after this
    run->raw_spc->advanced_stack_all(median_limit); // stack all auto and cross spectra
  }
  pool->wait(); // wait for all tasks to finish
  if (bcross_spectra) {
    for (auto &run : runs) {
      run->raw_spc->coherence_raw_spectra(12); // calculate the coherency spectra for all channels; a simple moving average is used
    }
    pool->wait(); // wait for all tasks to finish
  }
}

void ptspc_lib::dump_ac_spectra_coh_noise() {
  for (auto &run : runs) {
    // if empty the function does nothing
    // else it does dump to $HOME/dump_spectra
    run->raw_spc->dump_coh_spectra();
    run->raw_spc->dump_noise_spectra();
    run->raw_spc->dump_sa_spectra();
    run->raw_spc->dump_sa_prz_spectra();
    run->raw_spc->dump_coh_prz_spectra();
    run->raw_spc->dump_noise_prz_spectra();
  }
}
*/

//////////////////////////////////////////////////////////////////////////////////////

/*
ptspc_lib::ptspc_lib(std::shared_ptr<survey_tree> survey,
                     std::vector<std::string> stations_names,
                     const std::vector<size_t> run_numbers, const std::filesystem::path &exePath) :
    survey(survey), run_numbers(run_numbers) {
  if (survey == nullptr) {
    throw std::runtime_error("survey pointer is null");
  }
  if (stations_names.empty()) {
    throw std::runtime_error("stations_names is empty");
  } else {
    for (const auto &station_name : stations_names) {
      auto station = survey->get_child(station_name);
      if (station == nullptr) {
        std::ostringstream err_str(__func__, std::ios_base::ate);
        err_str << " station " << station_name << " not found in survey " << survey->get_path() << std::endl;
        throw std::runtime_error(err_str.str());
      }
      stations.emplace_back(station);
    }
  }
  if (stations.empty()) {
    throw std::runtime_error("stations vector is empty after processing stations_names");
  }
  if (run_numbers.empty()) {
    throw std::runtime_error("run_numbers is empty");
  } else {
    for (const auto &run_no : run_numbers) {
      if (run_no == 0) {
        std::ostringstream err_str(__func__, std::ios_base::ate);
        err_str << " run number cannot be zero" << std::endl;
        throw std::runtime_error(err_str.str());
      }
    }
  }
  // get the runs from the stations
  for (const auto &station : stations) {
    for (const auto &run_no : run_numbers) {
      auto run = station->get_child(std::format("run_{:0{}}", run_no, run_digits));
      if (run == nullptr) {
        std::ostringstream err_str(__func__, std::ios_base::ate);
        err_str << " run " << std::format("run_{:0{}}", id, run_digits) << " not found in station " << station->get_path() << std::endl;
        throw std::runtime_error(err_str.str());
      }
      runs.emplace_back(run->get_run_data());
      if (runs.back() == nullptr) {
        std::ostringstream err_str(__func__, std::ios_base::ate);
        err_str << " run_d pointer is null for run " << std::format("run_{:0{}}", run_no, run_digits) << " in station " << station->get_path() << std::endl;
        throw std::runtime_error(err_str.str());
      }
    }
  }
  if (runs.empty()) {
    throw std::runtime_error("runs vector is empty after processing run_numbers");
  }
  // Initialize other members if needed
  this->pool = std::make_shared<BS::thread_pool<BS::tp::none>>();
  // get the path to the calling executable
  this->ptspc_path = std::filesystem::canonical(exePath);
  // cd up to bin
  ptspc_path = ptspc_path.parent_path().parent_path();
  // cd to doc
  ptspc_path = ptspc_path / "data";
  fs::path sqlfile = ptspc_path / "info.db";
  fs::path master_cal_db = ptspc_path / "master_calibration.db";

  // Check if the SQL file exists and throw an error if it does not

  if (!fs::exists(master_cal_db)) {
    std::cerr << "could not find " << master_cal_db.string() << std::endl;
    throw std::runtime_error("Master calibration database not found.");
  }

  if (!fs::exists(sqlfile)) {
    std::cerr << "could not find " << sqlfile.string() << std::endl;
    throw std::runtime_error("SQL file not found.");
  }
}

*/
