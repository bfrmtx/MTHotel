#include "ptspc_lib.h"

// constructor
ptspc_lib::ptspc_lib() {
  this->pool = std::make_shared<BS::thread_pool<BS::tp::none>>();
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
void ptspc_lib::get_options(const std::vector<std::string> &margs, const bool &has_gui) {
  size_t l = 0;
  // Iterate through the arguments and process them
  for (l = 0; l < margs.size(); ++l) {
    std::string marg = margs[l];
    if (marg == "-u") {
      if (++l >= margs.size())
        throw std::runtime_error("-u requires an argument");
      fs::path survey_name = margs[l];
      if (!fs::is_directory(survey_name)) {
        std::ostringstream err_str(__func__, std::ios_base::ate);
        err_str << " -u survey_dir needs an existing directory with stations inside" << std::endl;
        err_str << " given: " << survey_name.string() << std::endl;
        throw std::runtime_error(err_str.str());
      }
      survey_name = fs::canonical(survey_name);
      survey = std::make_shared<survey_tree>(survey_name);
      if (survey == nullptr) {
        std::ostringstream err_str(__func__, std::ios_base::ate);
        err_str << " first use -u survey_name in order to init the survey";
        throw std::runtime_error(err_str.str());
      }
      survey->scan();
    } else if (marg == "-s") {
      if (survey == nullptr) {
        std::ostringstream err_str(__func__, std::ios_base::ate);
        err_str << " first use -u survey_name in order to init the survey";
        throw std::runtime_error(err_str.str());
      }
      // repeat until we find a - or the end of the arguments
      while ((l + 1) < margs.size() && !margs[l + 1].empty() && margs[l + 1][0] != '-') {
        std::string station_name = margs[++l];
        stations.emplace_back(survey->get_child(station_name));
        if (stations.back() == nullptr) {
          std::ostringstream err_str(__func__, std::ios_base::ate);
          err_str << " secondly use -s station_name [station names] in order to init station[s]";
          err_str << " station_name " << station_name << " not found in survey " << survey->get_path().string() << std::endl;
          stations.clear();
          throw std::runtime_error(err_str.str());
        }
      }
    } else if (marg == "-c") {
      while ((l + 1) < margs.size() && !margs[l + 1].empty() && margs[l + 1][0] != '-') {
        std::string channel_type = margs[++l];
        if (std::find(available_channel_types.begin(), available_channel_types.end(), channel_type) == available_channel_types.end()) {
          std::ostringstream err_str(__func__, std::ios_base::ate);
          err_str << " channel type " << channel_type << " not available" << std::endl;
          throw std::runtime_error(err_str.str());
        }
        channel_types.emplace_back(channel_type);
        auto_cross_spectra_names.emplace_back(channel_type, channel_type);
      }
    } else if (marg == "-cx") {
      while ((l + 2) < margs.size() && !margs[l + 1].empty() && margs[l + 1][0] != '-') {
        std::string first_channel = margs[++l];
        if ((l + 1) >= margs.size())
          throw std::runtime_error("-cx requires two arguments");
        std::string second_channel = margs[++l];
        if (first_channel == second_channel) {
          std::ostringstream err_str(__func__, std::ios_base::ate);
          err_str << " first and second channel must be different, you want cross spectra with -cx option" << std::endl;
          throw std::runtime_error(err_str.str());
        }
        std::string ac = first_channel + second_channel;
        if (std::find(available_ac_spectra_types.begin(), available_ac_spectra_types.end(), ac) == available_ac_spectra_types.end()) {
          std::ostringstream err_str(__func__, std::ios_base::ate);
          err_str << " ac spectra type " << ac << " not available" << std::endl;
          throw std::runtime_error(err_str.str());
        }
        if ((std::find(channel_types.begin(), channel_types.end(), first_channel) == channel_types.end()) ||
            (std::find(channel_types.begin(), channel_types.end(), second_channel) == channel_types.end())) {
          std::ostringstream err_str(__func__, std::ios_base::ate);
          err_str << " first and second channel must be in channel_types (the -c option)" << std::endl;
          throw std::runtime_error(err_str.str());
        }
        auto_cross_spectra_names.emplace_back(first_channel, second_channel);
      }
      bcross_spectra = true;
    } else if (marg == "-ref") {
      if (++l >= margs.size())
        throw std::runtime_error("-ref requires an argument");
      ref_channel = margs[l];
    } else if (marg == "-sb") {
      same_base = true;
    } else if (marg == "-m6") {
      magnify_06e = true;
    } else if (marg == "-m7") {
      magnify_07e = true;
    } else if (marg == "-rwy") {
      railway = true;
    } else if (marg == "-lowres") {
      lowres = true;
    } else if (marg == "-highres") {
      highres = true;
    } else if (marg == "-r") {
      br = true;
    } else if (marg == "-i") {
      inner_range = true;
    } else if (marg == "-cplt") {
      no_cal_plot = false;
    } else if (marg == "-syscal") {
      syscal = true;
    } else if (marg == "-n") {
      normalize = true;
    } else if (marg == "-sm") {
      smooth = true;
    } else if (marg == "-mc") {
      use_master_cal = true;
      master_cal = std::make_unique<get_from_master_cal>(master_cal_db);
    } else if (marg == "-ml") {
      if (++l >= margs.size())
        throw std::runtime_error("-ml requires an argument");
      median_limit = mstr::mystod(margs[l]);
      if (median_limit < 0.0 || median_limit > 1.0) {
        std::cout << " median limit must be between 0.0 and 1.0" << std::endl;
        throw std::runtime_error("median limit out of range");
      }
    } else if (marg == "-f_range") {
      if ((l + 2) >= margs.size())
        throw std::runtime_error("-f_range requires two arguments");
      f_range.first = mstr::mystod(margs[++l]);
      f_range.second = mstr::mystod(margs[++l]);
      if (f_range.first > f_range.second) {
        std::cout << " must be min max: f_range.first < f_range.second" << std::endl;
        throw std::runtime_error("frequency range invalid");
      }
      if (f_range.first <= 0) {
        std::cout << " f_range.first > 0 for log plot" << std::endl;
        throw std::runtime_error("frequency range invalid");
      }
    } else if (marg == "-a_range") {
      if ((l + 2) >= margs.size())
        throw std::runtime_error("-a_range requires two arguments");
      a_range.first = mstr::mystod(margs[++l]);
      a_range.second = mstr::mystod(margs[++l]);
      if (a_range.first > a_range.second) {
        std::cout << " must be min max: a_range.first < a_range.second" << std::endl;
        throw std::runtime_error("amplitude range invalid");
      }
      if (a_range.first <= 0) {
        std::cout << " a_range.first > 0 for log plot" << std::endl;
        throw std::runtime_error("amplitude range invalid");
      }
    } else if (marg == "-p_range") {
      if ((l + 2) >= margs.size())
        throw std::runtime_error("-p_range requires two arguments");
      p_range.first = mstr::mystod(margs[++l]);
      p_range.second = mstr::mystod(margs[++l]);
      if (p_range.first > p_range.second) {
        std::cout << " must be min max: p_range.first < p_range.second" << std::endl;
        throw std::runtime_error("phase range invalid");
      }
    } else if (marg == "-pl") {
      if (++l >= margs.size())
        throw std::runtime_error("-pl requires an argument");
      pwr_base = mstr::mystod(margs[l]);
      std::cout << "power lines suppressed >= " << pwr_base << " Hz" << std::endl;
    } else if (marg == "-") {
      std::cerr << "\nunrecognized option " << marg << std::endl;
      throw std::runtime_error("unrecognized option: " + marg);
    } else if (!marg.empty() && marg[0] == '-') {
      std::cerr << "\nunrecognized option " << marg << std::endl;
      throw std::runtime_error("unrecognized option: " + marg);
    }
    // else: skip, already incremented l
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
        if (station->get_run(irun) == nullptr) {
          std::cerr << station->get_name() << " run " << irun << " not found" << std::endl;
          throw std::runtime_error("Run not found in station: " + station->get_name() + " run: " + std::to_string(irun));
        }
        if (station->at(irun, schan) == nullptr) {
          std::cerr << station->get_name() << " channel " << schan << " not found" << std::endl;
          throw std::runtime_error("Channel not found in station: " + station->get_name() + " channel: " + schan);
        }
      }
    }
  }
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
}

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
      auto run = station->get_child(std::format("run_{:03}", run_no));
      if (run == nullptr) {
        std::ostringstream err_str(__func__, std::ios_base::ate);
        err_str << " run " << std::format("run_{:03}", run_no) << " not found in station " << station->get_path() << std::endl;
        throw std::runtime_error(err_str.str());
      }
      runs.emplace_back(run->get_run());
      if (runs.back() == nullptr) {
        std::ostringstream err_str(__func__, std::ios_base::ate);
        err_str << " run_d pointer is null for run " << std::format("run_{:03}", run_no) << " in station " << station->get_path() << std::endl;
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