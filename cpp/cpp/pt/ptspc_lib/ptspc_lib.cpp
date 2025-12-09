#include "ptspc_lib.h"
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
  fs::path sqlfile = ptspc_path / "info.sql3";
  fs::path master_cal_db = ptspc_path / "master_calibration.sql3";

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
    } else if (*it == "-m6") {
      magnify_06e = true;
      it = margs.erase(it);
    } else if (*it == "-m7") {
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
    } else if (*it == "-cplt") {
      no_cal_plot = false;
      it = margs.erase(it);
    } else if (*it == "-syscal") {
      syscal = true;
      it = margs.erase(it);
    } else if (*it == "-n") {
      normalize = true;
      it = margs.erase(it);
    } else if (*it == "-sm") {
      smooth = true;
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
      survey = std::make_shared<survey_tree>(survey_name, true); // read-only mode
      if (survey == nullptr) {
        std::ostringstream err_str(__func__, std::ios_base::ate);
        err_str << " first use -u survey_name in order to init the survey";
        throw std::runtime_error(err_str.str());
      }
      it = margs.erase(it); // erase survey name
    } else if (*it == "-s") {
      // Parse station block: -s station_name -r run1 run2 ... -ch channel1 channel2 ...
      it = margs.erase(it); // erase -s

      if (it == margs.end() || it->starts_with("-")) {
        throw std::runtime_error("No station name provided after -s");
      }

      // Create new station configuration
      station_config config;
      config.name = *it;
      it = margs.erase(it); // erase station name

      // Expect -r next
      if (it == margs.end() || *it != "-r") {
        throw std::runtime_error("Expected -r after station name '" + config.name + "'");
      }
      it = margs.erase(it); // erase -r

      if (it == margs.end() || it->starts_with("-")) {
        throw std::runtime_error("No run numbers provided after -r for station '" + config.name + "'");
      }

      // Collect run numbers until we hit -ch
      while (it != margs.end() && *it != "-ch") {
        if (it->starts_with("-")) {
          throw std::runtime_error("Expected -ch after run numbers for station '" + config.name + "', found: " + *it);
        }
        try {
          config.run_numbers.emplace_back(static_cast<size_t>(std::stoul(*it)));
        } catch (const std::invalid_argument &e) {
          throw std::runtime_error("Invalid run number format: '" + *it + "' for station '" + config.name + "' - must be a positive integer");
        } catch (const std::out_of_range &e) {
          throw std::runtime_error("Run number out of range: '" + *it + "' for station '" + config.name + "' - value too large");
        }
        it = margs.erase(it);
      }

      // Expect -ch next
      if (it == margs.end() || *it != "-ch") {
        throw std::runtime_error("Expected -ch after run numbers for station '" + config.name + "'");
      }
      it = margs.erase(it); // erase -ch

      if (it == margs.end() || it->starts_with("-")) {
        throw std::runtime_error("No channel types provided after -ch for station '" + config.name + "'");
      }

      // Collect channel types until we hit next option or end
      while (it != margs.end() && !it->starts_with("-")) {
        config.channels.emplace_back(*it);
        it = margs.erase(it);
      }

      // Store the station configuration
      station_configs.emplace_back(std::move(config));
    } else {
      ++it;
    }
  }

  survey->scan(); // scan the survey directory for stations and runs
  survey->list_children_recursive();

  // Process station configurations
  process_station_configs();
}

void ptspc_lib::process_station_configs() {
  if (station_configs.empty()) {
    return; // No station configurations to process
  }

  if (survey == nullptr) {
    throw std::runtime_error("Survey is not initialized - use -u survey_name before -s options");
  }

  // Clear existing data structures
  stations.clear();
  runs.clear();
  run_numbers.clear();
  channel_types.clear();

  std::set<std::string> unique_channels; // To collect all unique channel types

  for (const auto &config : station_configs) {
    // Get station from survey
    auto station = survey->get_child(config.name);
    if (station == nullptr) {
      throw std::runtime_error("Station '" + config.name + "' not found in survey");
    }
    stations.emplace_back(station);

    // Process runs for this station
    for (const auto &run_no : config.run_numbers) {
      auto run = station->get_run(run_no);
      if (run == nullptr) {
        throw std::runtime_error("Run " + std::to_string(run_no) + " not found in station '" + config.name + "'");
      }
      runs.emplace_back(run);

      // Add run number if not already present
      if (std::find(run_numbers.begin(), run_numbers.end(), run_no) == run_numbers.end()) {
        run_numbers.emplace_back(run_no);
      }
    }

    // Add channel types to unique set
    for (const auto &channel : config.channels) {
      unique_channels.insert(channel);
    }
  }

  // Convert unique channels to vector
  for (const auto &channel : unique_channels) {
    channel_types.emplace_back(channel);
  }

  // Sort run numbers for consistent processing
  std::sort(run_numbers.begin(), run_numbers.end());

  std::cout << "Processed " << station_configs.size() << " station configurations:" << std::endl;
  for (const auto &config : station_configs) {
    std::cout << "  Station: " << config.name;
    std::cout << ", Runs: ";
    for (size_t i = 0; i < config.run_numbers.size(); ++i) {
      if (i > 0)
        std::cout << ", ";
      std::cout << config.run_numbers[i];
    }
    std::cout << ", Channels: ";
    for (size_t i = 0; i < config.channels.size(); ++i) {
      if (i > 0)
        std::cout << ", ";
      std::cout << config.channels[i];
    }
    std::cout << std::endl;
  }
}

void ptspc_lib::get_run_numbers(const std::vector<size_t> &run_numbers_in) {
  if (run_numbers_in.empty()) {
    std::ostringstream err_str(__func__, std::ios_base::ate);
    err_str << " no run numbers given, use -r run_number [run_number] ...";
    throw std::runtime_error(err_str.str());
  }
  for (const auto &run_number : run_numbers_in) {
    for (const auto &station : stations) {
      auto run = station->get_run(run_number);
      if (run == nullptr) {
        std::ostringstream err_str(__func__, std::ios_base::ate);
        err_str << " run number " << run_number << " not found in station " << station->get_name() << std::endl;
        throw std::runtime_error(err_str.str());
      }
      runs.emplace_back(run);
    }
    this->run_numbers.emplace_back(run_number);
  }
}

void ptspc_lib::read_survey() {
  if (survey == nullptr) {
    std::ostringstream err_str(__func__, std::ios_base::ate);
    err_str << " survey is not initialized, use -u survey_name";
    throw std::runtime_error(err_str.str());
  }
  // check existence of all channels and runs first
  for (const auto &irun : run_numbers) {
    for (const auto &schan : channel_types) {
      for (auto &station : stations) {
        auto xrun = station->get_run(irun);
        if (xrun == nullptr) {
          std::cerr << station->get_name() << " run " << irun << " not found" << std::endl;
          throw std::runtime_error("Run not found in station: " + station->get_name() + " run: " + std::to_string(irun));
        }
        if (xrun->get_channel(schan) == nullptr) {
          std::cerr << station->get_name() << " channel " << schan << " not found" << std::endl;
          throw std::runtime_error("Channel not found in station: " + station->get_name() + " channel: " + schan);
        }
      }
    }
  }
  /*
  for (const auto &irun : run_numbers) {
    for (const auto &schan : channel_types) {
      wl = (size_t)stations.back()->at(irun, schan)->get_sample_rate(); // want 1 Hz bandwidth if possible
      rl = wl;
      if (wl < min_wl) {
        std::cout << "window length " << wl << " is less than minimum " << min_wl << ", using minimum" << std::endl;
        wl = min_wl; // use minimum window length, causes padding
        rl = min_rl; // use minimum read length
      }
      // init a fftw for each run - all channels have the same sample rate in each run
      // bandwidth 1 Hz ; during the first loop / irun fft_freqs are created, otherwise copied
      // if nullptr is given, the fft_freqs are created INSIDE the channel (best practice)
      for (auto &station : stations) // all stations have the same run numbers
        station->at(irun, schan)->init_fftw(nullptr, false, wl, rl);
    }
    // raws are connected to the thread pool - one for each run; the fft_freqs are connected to the channel, first valid is taken
    for (auto &station : stations)
      station->get_run(irun)->init_raw_spectra(pool); // this does not start a thread, it just prepares the raw spectra
  }
  */
}

/*
void ptspc_lib::read_info_console() {
  double f_or_s;
  std::string unit;
  for (const auto &irun : run_numbers) {
    for (const auto &schan : channel_types) {
      auto station = stations.back();
      mstr::sample_rate_to_str(station->at(irun, schan)->get_sample_rate(), f_or_s, unit);
      std::cout << "use sample rates of " << f_or_s << " " << unit << std::endl;
    }
  }
}

void ptspc_lib::process_spectra() {
  thread_index = 0; // reset thread index
  for (const auto &irun : run_numbers) {
    for (const auto &schan : channel_types) { // schan = string channel
      try {

        // the read_all_fftw pushes the fftw slices into a queue inside the channel object
        // pool->push_task(&channel::read_all_fftw, station->at(irun, schan), false, nullptr); // each channel is read in parallel
        for (auto &station : stations) {
          std::cout << "push thread " << thread_index++ << std::endl;
          // ---> at this point the time series is read and the FFT result will be INSIDE the channel object
          pool->detach_task([irun, schan, &station]() { station->at(irun, schan)->read_all_fftw(false, nullptr); });
        }
      }

      catch (const std::runtime_error &error) {
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

void ptspc_lib::collect_and_calibrate() {
  for (const auto &irun : run_numbers) {
    for (const auto &schan : channel_types) {
      for (auto &station : stations) {
        auto chan = station->get_run(irun)->get_channel(schan);
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
      runs.emplace_back(run->get_run());
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
  fs::path sqlfile = ptspc_path / "info.sql3";
  fs::path master_cal_db = ptspc_path / "master_calibration.sql3";

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