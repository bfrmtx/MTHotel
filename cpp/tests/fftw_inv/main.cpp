#include "atss.h"
#include "freqs.h"
#include "gnuplotter.h"
#include <algorithm>
#include <chrono>
#include <complex>
#include <fftw3.h>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <survey.h>
#include <vector>

#include "raw_spectra.h"
#include <random>

namespace fs = std::filesystem;

/*
-u /home/bfr/devel/ats_data/Northern_Mining -s Sarıçam -r 7 8

*/
int main(int argc, char *argv[]) {
  std::cout << "*******************************************************************************" << std::endl
            << std::endl;
  std::cout << "reads data from a survey (e.g. Northern Mining, Station Sarıçam" << std::endl;
  std::cout << "performs different FFT lentghs and padding" << std::endl;
  std::cout << "performs different FFT lengths and padding" << std::endl;
  std::cout << "results should give same results - all curves on top of each other" << std::endl;
  std::cout << "the envelope of the padded must be be the non-padded - you just obtain points inbetween" << std::endl;
  std::cout << "the base level is differrent: you have a different amount of stacks; noise may go down for shorter wl -> more stacks" << std::endl;
  std::cout << std::endl
            << "*******************************************************************************" << std::endl
            << std::endl;

  std::shared_ptr<survey_d> survey;
  std::shared_ptr<station_d> station;
  std::vector<std::string> channel_types;
  std::vector<std::shared_ptr<run_d>> runs;
  std::vector<std::shared_ptr<channel>> channels;
  auto pool = std::make_shared<BS::thread_pool>(4);
  // that is how we use it in tsplotter
  auto status = std::make_shared<stats>();

  unsigned l = 1;
  try {

    bool br = false;
    while (argc > 1 && (l < unsigned(argc)) && *argv[l] == '-') {
      std::string marg(argv[l]);

      if (marg.compare("-u") == 0) {
        fs::path surveyname = std::string(argv[++l]);
        if (!fs::is_directory(surveyname)) {
          std::ostringstream err_str(__func__, std::ios_base::ate);
          err_str << " -u surveydir needs an existing directoy with stations inside" << std::endl;
          err_str << " given: " << surveyname.string() << std::endl;
          throw std::runtime_error(err_str.str());
        }
        surveyname = fs::canonical(surveyname);
        survey = std::make_shared<survey_d>(surveyname);
        if (survey == nullptr) {
          std::ostringstream err_str(__func__, std::ios_base::ate);
          err_str << " first use -u surveyname in order to init the survey";
          throw std::runtime_error(err_str.str());
        }
      }
      if (marg.compare("-s") == 0) {
        if (survey == nullptr) {
          std::ostringstream err_str(__func__, std::ios_base::ate);
          err_str << " first use -u surveyname in order to init the survey";
          throw std::runtime_error(err_str.str());
        }
        auto stationname = std::string(argv[++l]);
        station = survey->get_station(stationname); // that is a shared pointer from survey
        if (station == nullptr) {
          std::ostringstream err_str(__func__, std::ios_base::ate);
          err_str << " secondly use -s stationname in order to init station";
          throw std::runtime_error(err_str.str());
        }
      }
      if ((marg.compare("-c") == 0) && (l < unsigned(argc - 2))) {

        while (l < unsigned(argc - 2) && *argv[l + 1] != '-') {
          std::string channel_type = std::string(argv[++l]);
          // if channel_type is in available_channel_types
          if (std::find(available_channel_types.begin(), available_channel_types.end(), channel_type) == available_channel_types.end()) {
            std::ostringstream err_str(__func__, std::ios_base::ate);
            err_str << " channel type " << channel_type << " not available" << std::endl;
            throw std::runtime_error(err_str.str());
          }
          channel_types.emplace_back(channel_type); // like Hx, Hy, Hz
        }
      }

      if (marg.compare("-r") == 0) {
        br = true; // prepare for runs
      }

      if (marg.compare("-wl") == 0) {
        status->wl = stoul(std::string(argv[++l]));
      }
      if (marg.compare("-rl") == 0) {
        status->rl = stoul(std::string(argv[++l]));
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
      }
      ++l;
    }
  } catch (const std::runtime_error &error) {
    std::cerr << error.what() << std::endl;
    return EXIT_FAILURE;
  } catch (const std::invalid_argument &ia) {
    std::cerr << ia.what() << std::endl;
    std::cerr << "unvalid run number" << std::endl;
    return EXIT_FAILURE;
  } catch (...) {
    std::cerr << "general error" << std::endl;
    return EXIT_FAILURE;
  }
  if (!runs.size()) {
    std::cerr << "no run numbers given" << std::endl;
    return EXIT_FAILURE;
  }
  if (!status->wl) {
    std::cerr << "no wl given" << std::endl;
    return EXIT_FAILURE;
  }
  if (!status->rl) {
    std::cerr << "no rl given" << std::endl;
    return EXIT_FAILURE;
  }

  // truncate runs to 1 element
  if (runs.size() > 1) {
    runs.resize(1);
  }
  // truncate channels_types to 1 element
  if (channel_types.size() > 1) {
    channel_types.resize(1);
  }

  size_t min_wl = 256;
  size_t min_rl = 256;
  size_t wl = 0;
  size_t padded = 1024; // min padded

  for (auto &run : runs) {

    // shared pointer from survey
    try {
      channels.emplace_back(run->get_channel(channel_types.at(0))); // that is a link, NOT a copy; you can do it only ONCE
      channels.back()->open_atss_read();
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
  // so we reached to the end of the tsplotter drop event.
  // now the set_wl() part
  for (auto &chan : channels) {
    if (chan != nullptr) {
      chan->cal->restore();                                    // restore the calibration file, we change inperpolated values
      chan->init_fftw(nullptr, false, status->wl, status->rl); // nullptr for using fftw inside the channel
      // electric field normally does not have a calibration file
      if (chan->cal->f.size()) {
        chan->cal->interpolate(chan->fft_freqs->get_frequencies());
        chan->cal->gen_cal_sensor(chan->fft_freqs->get_frequencies());
        chan->cal->join_lower_theo_and_measured_interpolated();
      }
      chan->init_inv_fftw();
    }
    chan->read_plotter(status, 0);
  }
  size_t show = 6;
  show = channels[0]->ts_slice.size();
  for (size_t i = 0; i < show; i++) {
    std::cout << channels[0]->ts_slice[i] << " ";
  }
  std::cout << std::endl;
  show = channels[0]->spc_slice.size();
  std::cout << "spectra size: " << channels[0]->spc_slice.size() << std::endl;
  std::cout << "calibration size: " << channels[0]->cal->f.size() << std::endl;
  for (size_t i = 0; i < show; i++) {
    std::cout << channels[0]->spc_slice[i] << " ";
  }
  std::cout << std::endl;
  show = channels[0]->ts_slice_inv.size();
  for (size_t i = 0; i < show; i++) {
    std::cout << channels[0]->ts_slice_inv[i] << " ";
  }

  status->read_pos = channels[0]->read_pos;

  return EXIT_SUCCESS;
}
