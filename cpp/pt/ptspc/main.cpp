#include "atss.h"
#include "freqs.h"
#include <algorithm>
#include <chrono>
#include <complex>
#include <fftw3.h>
#include <filesystem>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <survey.h>
#include <utility>
#include <vector>

#include "channel_collector.h"
#include "gnuplotter.h"
#include "mini_math.h"
#include "raw_spectra.h"
#include "sqlite_handler.h"
#include "strings_etc.h"
#include "vector_math.h"

// remark:
// in this main code I use extensively irun as run numbers and schan as channel types (strings)
// alternatively you could iterate over runs as shared pointers and channels as shared pointers (this is done a few times here, where I need the channel pointer)
// operators are provided in the survey class to access runs and channels by index or name

// /ptspc -u /home/bfr/tmp/rhd2    -c Hx Hy  -highres  -f_range 10 12000 -m6 -pl -cplt -s pt_1 -r 1  7

namespace fs = std::filesystem;

int main(int argc, char *argv[]) {
  std::cout << "*******************************************************************************" << std::endl
            << std::endl;
  std::cout << "reads data from a survey containing PT data" << std::endl;
  std::cout << " -u /home/bfr/tmp/ptr  -c Hx Hy Hz -s pt_1 -r 1 2" << std::endl;
  std::cout << " -ref Ey ... use Ey as reference channel for E/H" << std::endl;
  std::cout << std::endl
            << "*******************************************************************************" << std::endl
            << std::endl;

  bool same_base = false; // // always compare against first RMS, default no (outer), yes likely for inner f range
  bool inner_range = false;
  // administration
  std::shared_ptr<survey_d> survey;
  std::shared_ptr<station_d> station;
  std::vector<size_t> run_numbers;

  // which channel types to use
  std::vector<std::string> channel_types;

  std::vector<std::shared_ptr<fftw_freqs>> tmp_fft_freqs; // fftw_freqs for labels

  std::vector<std::shared_ptr<run_d>> runs;
  auto pool = std::make_shared<BS::thread_pool>();
  // try to make a stable reference by dividing by E in general
  std::vector<std::string> main_channel_types;
  std::string ref_channel;

  const double median_limit = 0.5; // for median limit
  bool lowres = false;             // low resolution plot only
  bool highres = false;            // high resolution plot only
  bool normalize = false;          // normalize the calibration amplitude by f (old style)

  std::pair<double, double> f_range = {0, 0}; // frequency range
  std::pair<double, double> a_range = {0, 0}; // amplitude range
  std::pair<double, double> p_range = {0, 0}; // phase range

  // get the runtime path of this executable
  fs::path ptspc_path = fs::canonical(argv[0]);
  // cd up to bin
  ptspc_path = ptspc_path.parent_path().parent_path();
  // cd to doc
  ptspc_path = ptspc_path / "data";
  fs::path sqlfile = ptspc_path / "info.sql3";

  if (!fs::exists(sqlfile)) {
    std::cerr << "could not find " << sqlfile.string() << std::endl;
    return EXIT_FAILURE;
  }

  bool magnify_06e = false; // take a smaller subset of the frequency data for MFS-06e
  bool magnify_07e = false; // take a smaller subset of the frequency data for MFS-07e
  bool no_cal_plot = true;  // skip the calibration plots at the end
  bool railway = false;     // railway data 16 2/3 Hz
  bool power_lines = false; // power line data 50, 150 Hz
  size_t min_wl = 256;      // minimum window length for fftw
  std::vector<std::pair<double, double>> power_lines_ranges = {{12, 20}, {46, 54}, {146, 154}};

  bool exit_flag = false;

  unsigned l = 1;
  try {
    bool br = false; // prepare for runs
    while (argc > 1 && (l < unsigned(argc)) && *argv[l] == '-') {
      std::string marg(argv[l]);

      if (marg.compare("-u") == 0) {
        fs::path survey_name = std::string(argv[++l]);
        if (!fs::is_directory(survey_name)) {
          std::ostringstream err_str(__func__, std::ios_base::ate);
          err_str << " -u survey_dir needs an existing directory with stations inside" << std::endl;
          err_str << " given: " << survey_name.string() << std::endl;
          throw std::runtime_error(err_str.str());
        }
        survey_name = fs::canonical(survey_name);
        survey = std::make_shared<survey_d>(survey_name);
        if (survey == nullptr) {
          std::ostringstream err_str(__func__, std::ios_base::ate);
          err_str << " first use -u survey_name in order to init the survey";
          throw std::runtime_error(err_str.str());
        }
      }
      if (marg.compare("-s") == 0) {
        if (survey == nullptr) {
          std::ostringstream err_str(__func__, std::ios_base::ate);
          err_str << " first use -u survey_name in order to init the survey";
          throw std::runtime_error(err_str.str());
        }
        auto station_name = std::string(argv[++l]);
        station = survey->get_station(station_name); // that is a shared pointer from survey
        if (station == nullptr) {
          std::ostringstream err_str(__func__, std::ios_base::ate);
          err_str << " secondly use -s station_name in order to init station";
          throw std::runtime_error(err_str.str());
        }
      }
      if ((marg.compare("-c") == 0) && (l < unsigned(argc - 2))) {

        while (l < unsigned(argc - 2) && *argv[l + 1] != '-') {
          std::string channel_type = std::string(argv[++l]);
          channel_types.emplace_back(channel_type);
        }
      }
      if (marg.compare("-ref") == 0) {
        ref_channel = std::string(argv[++l]);
      }
      if (marg.compare("-sb") == 0) {
        same_base = true; // prepare for runs
      }
      if (marg.compare("-m6") == 0) {
        magnify_06e = true; // prepare for MFS-06e coils
      }
      if (marg.compare("-m7") == 0) {
        magnify_07e = true; // prepare for MFS-07e coils
      }
      if (marg.compare("-rwy") == 0) {
        railway = true; // prepare for runs
      }
      if (marg.compare("-pl") == 0) {
        power_lines = true; // prepare for runs
      }
      if (marg.compare("-lowres") == 0) {
        lowres = true; // only low resolution plot - aka parzen plots
      }
      if (marg.compare("-highres") == 0) {
        highres = true; // only high resolution plot - standard plots
      }
      if (marg.compare("-r") == 0) {
        br = true; // prepare for runs
      }
      if (marg.compare("-i") == 0) {
        inner_range = true; // prepare for runs
      }
      if (marg.compare("-cplt") == 0) {
        no_cal_plot = false; //  activate calibration plots AND the calibration of the spectra!
      }
      if (marg.compare("-n") == 0) {
        normalize = true; //  activate calibration plots normalized by f
      }

      if (marg.compare("-f_range") == 0) {
        f_range.first = mstr::mystod(std::string(argv[++l]));
        f_range.second = mstr::mystod(std::string(argv[++l]));
        if (f_range.first > f_range.second) {
          std::cout << " must be min max: f_range.first < f_range.second" << std::endl;
          exit_flag = true;
        }
        if (f_range.first <= 0) {
          std::cout << " f_range.first > 0 for log plot" << std::endl;
          exit_flag = true;
        }
      }

      if (marg.compare("-a_range") == 0) {
        a_range.first = mstr::mystod(std::string(argv[++l]));
        a_range.second = mstr::mystod(std::string(argv[++l]));
        if (a_range.first > a_range.second) {
          std::cout << " must be min max: a_range.first < a_range.second" << std::endl;
          exit_flag = true;
        }
        if (a_range.first <= 0) {
          std::cout << " a_range.first > 0 for log plot" << std::endl;
          exit_flag = true;
        }
      }

      if (marg.compare("-p_range") == 0) {
        p_range.first = mstr::mystod(std::string(argv[++l]));
        p_range.second = mstr::mystod(std::string(argv[++l]));
        if (p_range.first > p_range.second) {
          std::cout << " must be min max: p_range.first < p_range.second" << std::endl;
          exit_flag = true;
        }
      }

      if (marg.compare("-") == 0) {
        std::cerr << "\nunrecognized option " << argv[l] << std::endl;
        return EXIT_FAILURE;
      }
      ++l;
    }

    // all options including the last -r is here, now get the runs
    while ((l < unsigned(argc))) {
      std::string marg(argv[l]);
      if ((survey != nullptr) && (station != nullptr) && br) {
        size_t ri = stoul(marg);
        runs.emplace_back(station->get_run(ri));
        run_numbers.emplace_back(ri);
      }
      ++l;
    }
  } catch (const std::runtime_error &error) {
    std::cerr << error.what() << std::endl;
    return EXIT_FAILURE;
  } catch (const std::invalid_argument &ia) {
    std::cerr << ia.what() << std::endl;
    std::cerr << "invalid run number" << std::endl;
    return EXIT_FAILURE;
  } catch (...) {
    std::cerr << "general error" << std::endl;
    return EXIT_FAILURE;
  }
  if (!runs.size()) {
    std::cerr << "no run numbers given" << std::endl;
    return EXIT_FAILURE;
  }
  if (exit_flag) {
    std::cout << " exit failure setting axis " << std::endl;
    return EXIT_FAILURE;
  }

  if (!lowres && !highres) {
    std::cout << "no plot option given, use -lowres or -highres" << std::endl;
    return EXIT_FAILURE;
  }

  if (ref_channel.size()) {
    main_channel_types = channel_types;      // safe the main channel types
    channel_types.emplace_back(ref_channel); // append the reference channel
  }

  // ******************************** read data *************************************************************************************

  size_t wl;

  // channels, fft_freqs and raws are connected by index !!!
  // the have the same order - so [i] is the same for all
  // 1 Hz bandwidth is needed for parallel sensor test
  // if sample rate is less than 256 - we have to adjust the fft_freqs

  for (const auto &irun : run_numbers) {
    for (const auto &schan : channel_types) {
      try {
        wl = (size_t)station->at(irun, schan)->get_sample_rate(); // want 1 Hz bandwidth if possible
        if (wl < min_wl)
          wl = min_wl; // else use min_wl
        // init a fftw for each run - all channels have the same sample rate
        // bandwidth 1 Hz ; during the first loop / irun fft_freqs are created, otherwise copied
        // if nullptr is given, the fft_freqs are created INSIDE the channel (best practice)
        station->at(irun, schan)->init_fftw(nullptr, wl, wl);

      } catch (const std::runtime_error &error) {
        std::cerr << error.what() << std::endl;
        return EXIT_FAILURE;
      } catch (const std::exception &e) {
        std::cerr << "error: " << e.what() << std::endl;
        return EXIT_FAILURE;

      } catch (...) {
        std::cerr << "could not allocate all channels" << std::endl;
        return EXIT_FAILURE;
      }
    }
    // raws are connected to the thread pool - one for each run
    station->get_run(irun)->init_raw_spectra(pool);
  }

  double f_or_s;
  std::string unit;

  for (const auto &irun : run_numbers) {
    for (const auto &schan : channel_types) {
      mstr::sample_rate_to_str(station->at(irun, schan)->get_sample_rate(), f_or_s, unit);
      std::cout << "use sample rates of " << f_or_s << " " << unit << std::endl;
    }
  }
  size_t thread_index = 0;
  for (const auto &irun : run_numbers) {
    for (const auto &schan : channel_types) {
      try {
        std::cout << "push thread " << thread_index++ << std::endl;
        // the read_all_fftw pushes the fftw slices into a queue inside the channel object
        pool->push_task(&channel::read_all_fftw, station->at(irun, schan), false, nullptr); // each channel is read in parallel
      }

      catch (const std::runtime_error &error) {
        std::cerr << error.what() << std::endl;
        std::cerr << "could not execute fftw" << std::endl;
        return EXIT_FAILURE;
      } catch (...) {
        std::cerr << "could not execute fftw" << std::endl;
        return EXIT_FAILURE;
      }
    }
  }
  pool->wait_for_tasks(); // wait for all tasks to finish, read time series and perform fftw

  inner_outer<double> innerouter; // inner and outer range for the resulting spectra, we do not want all frequencies
  for (const auto &irun : run_numbers) {
    for (const auto &schan : channel_types) {
      try {
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
          innerouter.set_low_high(chan->fft_freqs->auto_range(0.01, 0.7)); // cut off spectra; we need these values later
        if (no_cal_plot == false) {
          std::cout << "calibration" << std::endl;
          chan->cal->interpolate(chan->fft_freqs->get_frequencies());
          chan->cal->gen_cal_sensor(chan->fft_freqs->get_frequencies());
          chan->cal->join_lower_theo_and_measured_interpolated();
        }
        // chan->prepare_raw_spc(false); // no calibration yet
        // push task can not use default arguments, supply all arguments
        // the prepare_raw_spc pushes the raw spectra queue into a vector spc inside the channel object
        pool->push_task(&channel::prepare_raw_spc, chan, !no_cal_plot, true); // no calibration yet
      }

      catch (const std::runtime_error &error) {
        std::cerr << error.what() << std::endl;
        std::cerr << "could not prepare_raw_spc" << std::endl;
        return EXIT_FAILURE;
      } catch (...) {
        std::cerr << "could not prepare_raw_spc" << std::endl;
        return EXIT_FAILURE;
      }
    }
  }
  pool->wait_for_tasks(); // wait for all tasks to finish

  for (auto &run : runs) {
    run->fetch_raw_spectra(); // fetch the raw spectra from the thread pool; move operation (fast)
  }

  std::cout << "stacking" << std::endl;
  thread_index = 0;
  for (auto &run : runs) {
    // run raw spectra fires up all channels
    run->raw_spc->advanced_stack_all(median_limit);
    // run->raw_spc->simple_stack_all(); // run contains the raw spectra Ex, Ey, Ez, Hx, Hy, Hz or what was given
  }
  pool->wait_for_tasks(); // wait for all tasks to finish
  std::cout << "done" << std::endl;

  size_t i;
  std::string init_err;

  // std::vector<double> v = runs[0]->get_channel("Hx")->fft_freqs->get_frequencies();
  // for (const auto &val : v)
  //   std::cout << val << " ";
  // std::cout << std::endl;
  // interpolate the cal data to the fft_freqs
  for (auto &irun : run_numbers) {
    for (auto &schan : channel_types) {
      try {
        station->at(irun, schan)->cal->interpolate(station->at(irun, schan)->fft_freqs->get_frequencies());
        station->at(irun, schan)->cal->gen_cal_sensor(station->at(irun, schan)->fft_freqs->get_frequencies());
        station->at(irun, schan)->cal->join_lower_theo_and_measured_interpolated();
      } catch (const std::runtime_error &error) {
        std::cerr << error.what() << std::endl;
        return EXIT_FAILURE;
      }
    }
  }

  std::ostringstream all_coils;
  std::string all_coils_title;

  // title of the plot shall contain all coils names and serial numbers
  for (auto &irun : run_numbers) {
    for (auto &schan : channel_types) {
      auto scoil = station->at(irun, schan)->cal->sensor + " " + station->at(irun, schan)->cal->serial2string();
      // if all_coils does not contain scoil, add it
      if (all_coils.str().find(scoil) == std::string::npos) {
        all_coils << scoil << ", ";
      }
    }
  }
  // rewind the stream by 2 to remove the last comma and space
  all_coils.seekp(-2, std::ios_base::end);
  // overwrite the last 2 characters
  all_coils << " ";
  all_coils_title = all_coils.str().substr(0, all_coils.str().size() - 2);

  // ******************************** high resolution plot *******************************************************************************

  if (highres) {
    auto gplt = std::make_shared<gnuplotter<double, double>>(init_err);

    if (init_err.size()) {
      std::cout << init_err << std::endl;
      return EXIT_FAILURE;
    }

    gplt->cmd << "set terminal qt size 2048,768 enhanced" << std::endl;
    gplt->cmd << "set title '" << all_coils_title << "'" << std::endl;
    // gplt->cmd << "set key off" << std::endl;

    gplt->cmd << "set xlabel 'frequency [Hz]'" << std::endl;
    gplt->cmd << "set ylabel 'amplitude'" << std::endl;
    gplt->cmd << "set grid" << std::endl;
    gplt->cmd << "set format y '1E^%+03T'" << std::endl;
    gplt->cmd << "set logscale xy" << std::endl;
    gplt->cmd << "set key font \"Hack, 10\"" << std::endl;
    gplt->set_y_range(a_range);

    if (!gplt->set_x_range(f_range)) { // we did not use manual range
      if (inner_range) {
        gplt->set_x_range(innerouter.get_inner());
        std::cout << "inner range: " << mstr::f_to_string(innerouter.get_inner().first) << " <-> " << mstr::f_to_string(innerouter.get_inner().second) << std::endl;
        for (auto &irun : run_numbers) {
          for (auto &schan : channel_types) {
            std::cout << mstr::f_to_string(station->at(irun, schan)->fft_freqs->get_frange().first) << " <-> " << mstr::f_to_string(station->at(irun, schan)->fft_freqs->get_frange().second) << std::endl;
          }
        }
      } else {
        gplt->set_x_range(innerouter.get_outer());
        std::cout << "outer range: " << mstr::f_to_string(innerouter.get_outer().first) << " <-> " << mstr::f_to_string(innerouter.get_outer().second) << std::endl;
        for (auto &irun : run_numbers) {
          for (auto &schan : channel_types) {
            std::cout << mstr::f_to_string(station->at(irun, schan)->fft_freqs->get_frange().first) << " <-> " << mstr::f_to_string(station->at(irun, schan)->fft_freqs->get_frange().second) << std::endl;
          }
        }
      }
    }
    auto rmss = std::make_shared<std::vector<double>>();

    for (auto &irun : run_numbers) {
      for (auto &schan : channel_types) {
        try {
          std::vector<double> f, v;
          if (inner_range)
            f = station->at(irun, schan)->fft_freqs->get_frequency_slice(innerouter.get_inner());
          else
            f = station->at(irun, schan)->fft_freqs->get_frequency_slice(innerouter.get_outer());
          if (inner_range)
            v = trim_by_index<double>(station->get_run(irun)->raw_spc->get_abs_sa_spectra(schan), station->at(irun, schan)->fft_freqs->get_index_slice());
          else
            v = station->get_run(irun)->raw_spc->get_abs_sa_spectra(schan);
          double rms = 0;
          for (const auto val : v)
            rms += val * val;
          rms /= double(v.size());
          rms = sqrt(rms);

          std::cout << "rms: " << rms << std::endl;
          (*rmss).emplace_back(rms);
        } catch (const std::runtime_error &error) {
          std::cerr << error.what() << std::endl;
          return EXIT_FAILURE;
        }
      }
    }

    // need all fft_freqs for the labels

    for (auto &irun : run_numbers) {
      for (auto &schan : channel_types) {
        tmp_fft_freqs.emplace_back(station->at(irun, schan)->fft_freqs);
      }
    }

    auto labels = gnuplot_labels(tmp_fft_freqs, rmss);
    std::cout << "formatted" << std::endl;
    for (const auto &lab : labels) {
      std::cout << lab.str() << std::endl;
    }

    // make the plot
    try {
      i = 0;
      for (auto &irun : run_numbers) {
        for (auto &schan : channel_types) {
          std::string label = station->at(irun, schan)->cal->sensor + " " + station->at(irun, schan)->cal->serial2string() + " " + labels.at(i++).str();
          if (!power_lines)
            gplt->set_xy_lines(station->at(irun, schan)->fft_freqs->get_frequencies(), station->get_run(irun)->raw_spc->get_abs_sa_spectra(schan), label, 1, gplt->default_color(schan));
          else {
            std::vector<double> f1, v1;
            std::vector<double> f2, v2;
            remove_spectral_range(station->at(irun, schan)->fft_freqs->get_frequencies(), station->get_run(irun)->raw_spc->get_abs_sa_spectra(schan), f1, v1, power_lines_ranges);
            remove_spectral_lines(f1, v1, f2, v2, 50, 3);
            gplt->set_xy_lines(f2, v2, label, 1, gplt->default_color(schan));
          }
        }
      }
    } catch (const std::runtime_error &error) {
      std::cerr << error.what() << std::endl;
      std::cerr << "could not pipe all files to gnuplot" << std::endl;
      return EXIT_FAILURE;
    } catch (...) {
      std::cerr << "could not pipe all files to gnuplot" << std::endl;
      return EXIT_FAILURE;
    }
    gplt->plot();

  } // end lowres only

  // ******************************** parzening  *******************************************************************************
  // now do the same with parzening
  if (lowres) {
    std::cout << "parzening" << std::endl;

    // start to set the targe frequencies and create the parzen vectors
    std::string sql_query = "SELECT * FROM default_mt_frequencies"; // ORDER by will not work - it is TEXT
    std::vector<double> target_freqs;                               // target frequencies from SQL table
    auto sql_info = std::make_unique<sqlite_handler>(sqlfile);

    try {
      target_freqs = sql_info->sqlite_vector_double(sql_query);
      std::sort(target_freqs.begin(), target_freqs.end());
    } catch (const std::runtime_error &error) {
      std::cerr << error.what() << std::endl;
      std::cerr << "could not read default_mt_frequencies" << std::endl;
      return EXIT_FAILURE;
    } catch (...) {
      std::cerr << "could not read default_mt_frequencies" << std::endl;
      return EXIT_FAILURE;
    }

    // if the vector contains value 15, remove 15.0000
    auto it = std::find(target_freqs.begin(), target_freqs.end(), 15.0);
    if (it != target_freqs.end()) {
      target_freqs.erase(it);
    }

    try {
      for (auto &irun : run_numbers) {
        for (auto &schan : channel_types) {
          station->at(irun, schan)->fft_freqs->set_target_freqs(target_freqs, 0.15);
          // fft_freq->set_target_freqs(target_freqs, 0.15);
          pool->push_task(&fftw_freqs::create_parzen_vectors, station->at(irun, schan)->fft_freqs);
        }
      }

    } catch (const std::runtime_error &error) {
      std::cerr << error.what() << std::endl;
      return EXIT_FAILURE;
    } catch (const std::exception &e) {
      std::cerr << "error: " << e.what() << std::endl;
      return EXIT_FAILURE;
    } catch (...) {
      std::cerr << "could not allocate all channels" << std::endl;
      return EXIT_FAILURE;
    }
    pool->wait_for_tasks(); // always wait for tasks for parzening

    try {
      for (auto &irun : run_numbers) {
        station->get_run(irun)->raw_spc->parzen_stack_all();
        std::cout << station->get_run(irun)->raw_spc->get_abs_sa_spectra(channel_types.at(0)).size() << std::endl;
      }
    } catch (const std::runtime_error &error) {
      std::cerr << error.what() << std::endl;
      return EXIT_FAILURE;
    } catch (const std::exception &e) {
      std::cerr << "error: " << e.what() << std::endl;
      return EXIT_FAILURE;
    } catch (...) {
      std::cerr << "could not allocate all channels" << std::endl;
      return EXIT_FAILURE;
    }
    pool->wait_for_tasks(); // always wait for tasks

    // ******************************** parzen plot *******************************************************************************

    init_err.clear();
    auto gplt_prz = std::make_shared<gnuplotter<double, double>>(init_err);

    if (init_err.size()) {
      std::cout << init_err << std::endl;
      return EXIT_FAILURE;
    }

    gplt_prz->cmd << "set terminal qt size 2048,768 enhanced" << std::endl;
    // gplt_prz->cmd << "set title 'FFT Parzen'" << std::endl;
    gplt_prz->cmd << "set title '" << all_coils_title << "'" << std::endl;

    // gplt_prz->cmd << "set key off" << std::endl;

    gplt_prz->cmd << "set xlabel 'frequency [Hz]'" << std::endl;
    gplt_prz->cmd << "set ylabel 'amplitude'" << std::endl;
    gplt_prz->cmd << "set grid" << std::endl;
    gplt_prz->cmd << "set format y '1E^%+03T'" << std::endl;
    gplt_prz->cmd << "set logscale xy" << std::endl;
    if (inner_range)
      gplt_prz->set_x_range(innerouter.get_inner());
    else
      gplt_prz->set_x_range(innerouter.get_outer());
    gplt_prz->cmd << "set key font \"Hack, 10\"" << std::endl;
    // gplt_prz->set_x_range(40, 60);

    i = 0;
    try {
      // concatenate all spectra

      std::vector<std::unique_ptr<channel_collector_gplt>> channel_collectors(channel_types.size());
      i = 0;
      for (auto &channel_collector : channel_collectors) {
        channel_collector = std::make_unique<channel_collector_gplt>(gplt_prz, channel_types.at(i++), true);
      }
      for (auto &irun : run_numbers) {
        for (auto &schan : channel_types) {
          for (auto &cc : channel_collectors) {
            cc->collect(station->at(irun, schan), station->at(irun, schan)->fft_freqs, station->get_run(irun)->raw_spc);
          }
        }
      }

      // plot the trailing data !
      for (auto &cc : channel_collectors) {
        cc->plot();
      }

    } catch (const std::runtime_error &error) {
      std::cerr << error.what() << std::endl;
      std::cerr << "could not pipe all files to gnuplot" << std::endl;
      return EXIT_FAILURE;
    } catch (...) {
      std::cerr << "could not pipe all files to gnuplot" << std::endl;
      return EXIT_FAILURE;
    }
    gplt_prz->plot();
  }

  // ******************************** calibration plots *******************************************************************************

  if (!no_cal_plot) {
    init_err.clear();
    auto gplt_cal_a = std::make_shared<gnuplotter<double, double>>(init_err);
    gplt_cal_a->cmd << "set terminal qt size 1024,768 enhanced" << std::endl;
    gplt_cal_a->cmd << "set title 'Calibration'" << std::endl;
    gplt_cal_a->cmd << "set xlabel 'frequency [Hz]'" << std::endl;
    gplt_cal_a->cmd << "set ylabel 'amplitude'" << std::endl;
    gplt_cal_a->cmd << "set grid" << std::endl;
    gplt_cal_a->cmd << "set format y '1E^%+03T'" << std::endl;
    gplt_cal_a->cmd << "set logscale xy" << std::endl;

    try {
      for (auto &irun : run_numbers) {
        for (auto &schan : channel_types) {

          auto f = station->at(irun, schan)->cal->f;
          auto a = station->at(irun, schan)->cal->a;
          if (normalize)
            std::transform(a.begin(), a.end(), f.begin(), a.begin(), std::divides<double>()); // normalize the amplitude by f

          // remove values from f between 12 and 22
          if (railway) {
            for (auto it = f.begin(); it != f.end();) {
              if ((*it > 12) && (*it < 22)) {
                it = f.erase(it);
                a.erase(a.begin() + std::distance(f.begin(), it));
              } else
                ++it;
            }
          }
          gplt_cal_a->set_xy_lines(f, a, station->at(irun, schan)->cal->sensor + " " + station->at(irun, schan)->cal->serial2string(), 1, gplt_cal_a->default_color(station->at(irun, schan)->channel_type));
          // for testing
          std::vector<double> f_theo, a_theo, p_theo;
          station->at(irun, schan)->cal->get_theo_cal(f_theo, a_theo, p_theo);
          if (normalize)
            std::transform(a_theo.begin(), a_theo.end(), f_theo.begin(), a_theo.begin(), std::divides<double>()); // normalize the amplitude by f
          gplt_cal_a->set_xy_points(f_theo, a_theo, station->at(irun, schan)->cal->sensor + " " + station->at(irun, schan)->cal->serial2string() + " theo", 1, "pt 7");
        }
      }
    } catch (const std::runtime_error &error) {
      std::cerr << error.what() << std::endl;
      std::cerr << "could not pipe all files to gnuplot" << std::endl;
      return EXIT_FAILURE;
    } catch (const std::exception &e) {
      std::cerr << e.what() << '\n';
    } catch (...) {
      std::cerr << "could not pipe all files to gnuplot" << std::endl;
      return EXIT_FAILURE;
    }
    gplt_cal_a->plot();

    init_err.clear();
    auto gplt_cal_p = std::make_shared<gnuplotter<double, double>>(init_err);
    gplt_cal_p->cmd << "set terminal qt size 1024,768 enhanced" << std::endl;
    gplt_cal_p->cmd << "set title 'Calibration'" << std::endl;
    gplt_cal_p->cmd << "set xlabel 'frequency [Hz]'" << std::endl;
    gplt_cal_p->cmd << "set ylabel 'phase'" << std::endl;
    gplt_cal_p->cmd << "set grid" << std::endl;
    // gplt_cal_p->cmd << "set format y '1E^%+03T'" << std::endl;
    gplt_cal_p->cmd << "set logscale x" << std::endl;

    try {
      for (auto &irun : run_numbers) {
        for (auto &schan : channel_types) {

          auto f = station->at(irun, schan)->cal->f;
          auto p = station->at(irun, schan)->cal->p;
          // remove values from f between 12 and 22
          if (railway) {
            for (auto it = f.begin(); it != f.end();) {
              if ((*it > 12) && (*it < 22)) {
                it = f.erase(it);
                p.erase(p.begin() + std::distance(f.begin(), it));
              } else
                ++it;
            }
          }

          gplt_cal_p->set_xy_lines(f, p, station->at(irun, schan)->cal->sensor + " " + station->at(irun, schan)->cal->serial2string(), 1, gplt_cal_p->default_color(station->at(irun, schan)->channel_type));
        }
      }
    } catch (const std::runtime_error &error) {
      std::cerr << error.what() << std::endl;
      std::cerr << "could not pipe all files to gnuplot" << std::endl;
      return EXIT_FAILURE;
    } catch (const std::exception &e) {
      std::cerr << e.what() << '\n';
    } catch (...) {
      std::cerr << "could not pipe all files to gnuplot" << std::endl;
      return EXIT_FAILURE;
    }
    gplt_cal_p->plot();
  }

  return EXIT_SUCCESS;
}
