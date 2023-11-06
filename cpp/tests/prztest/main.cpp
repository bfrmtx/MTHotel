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
#include <vector>

#include "gnuplotter.h"
#include "raw_spectra.h"

int main() {

  size_t i;
  size_t lw = 3; // long window
  size_t zp = 4; // zero padded ... last file
  const double median_limit = 0.7;

  std::filesystem::path home_dir(getenv("HOME"));
  auto survey = std::make_shared<survey_d>(home_dir.string() + "/devel/ats_data/three_fgs/indi");
  auto station_26 = survey->get_station("S26"); // that is a shared pointer from survey

  // standard windows 512 x 4
  auto run_002 = station_26->get_run(2);
  auto run_003 = station_26->get_run(3);
  auto run_004 = station_26->get_run(4);

  std::vector<std::shared_ptr<channel>> channels;
  auto pool = std::make_shared<BS::thread_pool>();

  std::string channel_type("Ey");

  // shared pointer from survey
  try {
    channels.emplace_back(run_004->get_channel(channel_type));
    channels.emplace_back(run_003->get_channel(channel_type));
    channels.emplace_back(run_002->get_channel(channel_type));

    // a new instance - not a copy/link; the readbuffer is inside the class
    channels.emplace_back(std::make_shared<channel>(run_003->get_channel(channel_type)));
    channels.emplace_back(std::make_shared<channel>(run_003->get_channel(channel_type)));
  } catch (const std::runtime_error &error) {
    std::cerr << error.what() << std::endl;
    return EXIT_FAILURE;
  }
  std::vector<std::shared_ptr<fftw_freqs>> fft_freqs;
  std::vector<std::shared_ptr<raw_spectra>> raws;

  i = 0;
  size_t wl = 1024, fixwl;
  fixwl = 1024;
  // create the fftw interface with individual window length and read length
  for (auto &chan : channels) {
    if (i == lw)
      fft_freqs.emplace_back(std::make_shared<fftw_freqs>(chan->get_sample_rate(), fixwl, fixwl));
    else if (i == zp)
      fft_freqs.emplace_back(std::make_shared<fftw_freqs>(chan->get_sample_rate(), 4096, fixwl)); // first 4096
    else
      fft_freqs.emplace_back(std::make_shared<fftw_freqs>(chan->get_sample_rate(), wl, wl));
    wl *= 4;
    ++i;
    chan->set_fftw_plan(fft_freqs.back());
    // here each channel is treated as single result - by default it would contain 5 channels
    raws.emplace_back(std::make_shared<raw_spectra>(pool, fft_freqs.back()));
  }

  double f_or_s;
  std::string unit;

  // example iterator usage  fft with auto & channels
  auto fft_res_iter = fft_freqs.begin();
  for (const auto &chan : channels) {
    auto fft_fres = *fft_res_iter++;
    mstr::sample_rate_to_str(chan->get_sample_rate(), f_or_s, unit);
    std::cout << "use sample rates of " << f_or_s << " " << unit << " wl:" << fft_fres->get_wl() << "  read length:" << fft_fres->get_rl() << std::endl;
    chan->set_fftw_plan(fft_fres);
  }

  // ******************************** read all fft *******************************************************************************

  try {
    std::vector<std::jthread> threads;
    for (auto &chan : channels) {
      // chan->read_all_fftw();
      threads.emplace_back(std::jthread(&channel::read_all_fftw, chan, false, nullptr));
    }
  } catch (const std::runtime_error &error) {
    std::cerr << error.what() << std::endl;
    return EXIT_FAILURE;
  } catch (const std::exception &e) {
    std::cerr << "error: " << e.what() << std::endl;
    return EXIT_FAILURE;
  } catch (...) {
    std::cerr << "could not read all channels" << std::endl;
    return EXIT_FAILURE;
  }

  fft_res_iter = fft_freqs.begin();
  for (auto &chan : channels) {
    auto fft_fres = *fft_res_iter++;
    // set the range of fft spectra to use here
    fft_fres->set_lower_upper_f(1.0 / 2048.0, 0.011, true); // cut off spectra
  }

  // ******************************** queue to vector *******************************************************************************

  try {
    std::vector<std::jthread> threads;

    fft_res_iter = fft_freqs.begin();
    for (auto &chan : channels) {
      auto fft_fres = *fft_res_iter++;
      // chan->prepare_to_raw_spc(fft_fres, false);
      //  we have a pointer and don't need std::ref
      threads.emplace_back(std::jthread(&channel::prepare_to_raw_spc, chan, fft_fres, false, true));
    }
  } catch (const std::runtime_error &error) {
    std::cerr << error.what() << std::endl;
    return EXIT_FAILURE;

  } catch (const std::exception &e) {
    std::cerr << "error: " << e.what() << std::endl;
    return EXIT_FAILURE;

  } catch (...) {
    std::cerr << "could not dequeue all channels" << std::endl;
    return EXIT_FAILURE;
  }

  i = 0;
  for (auto &chan : channels) {
    raws[i++]->get_raw_spectra(chan->spc, chan->channel_type, chan->bw, chan->is_remote, chan->is_emap);
  }

  std::cout << std::endl;

  // std::vector<double> target_freqs {1./1024.};
  std::vector<double> target_freqs;

  for (size_t j = 1; j < fft_freqs[0]->get_frequencies().size() - 9;) {
    target_freqs.push_back(fft_freqs[0]->get_frequencies().at(j));
    j += 8;
  }

  // ******************************** parzen vectors *******************************************************************************

  try {
    for (auto &fftr : fft_freqs) {
      fftr->set_target_freqs(target_freqs, 0.15);
      fftr->create_parzen_vectors();
    }
    std::vector<std::jthread> threads;

    for (auto &fftr : fft_freqs) {
      threads.emplace_back(std::jthread(&fftw_freqs::create_parzen_vectors, fftr));
      // fftr->create_parzen_vectors();
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

  // ******************************** stack all *******************************************************************************

  try {
    std::vector<std::jthread> threads;
    for (auto &rw : raws) {
      threads.emplace_back(std::jthread(&raw_spectra::advanced_stack_all, rw, std::ref(median_limit)));
      // rw->advanced_stack_all(0.7);
    }
  } catch (const std::runtime_error &error) {
    std::cerr << error.what() << std::endl;
    return EXIT_FAILURE;

  } catch (const std::exception &e) {
    std::cerr << "error: " << e.what() << std::endl;
    return EXIT_FAILURE;

  } catch (...) {
    std::cerr << "could not stack all channels" << std::endl;
    return EXIT_FAILURE;
  }

  // ******************************** parzen stack *******************************************************************************
  try {
    std::vector<std::jthread> threads;
    for (auto &rw : raws) {
      threads.emplace_back(std::jthread(&raw_spectra::parzen_stack_all, rw));
      // rw->parzen_stack_all();
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

  // ******************************** cont *******************************************************************************

  std::string init_err;
  auto gplt = std::make_unique<gnuplotter<double, double>>(init_err);

  if (init_err.size()) {
    std::cout << init_err << std::endl;
    return EXIT_FAILURE;
  }

  gplt->cmd << "set terminal qt size 2048,1600 enhanced" << std::endl;
  gplt->cmd << "set title 'FFT Zero Padding'" << std::endl;
  // gplt->cmd << "set key off" << std::endl;
  gplt->cmd << "set xlabel 'frequency [Hz]'" << std::endl;
  gplt->cmd << "set logscale xy" << std::endl;
  gplt->cmd << "set ylabel 'amplitude [mV/√Hz]'" << std::endl;
  gplt->cmd << "set grid" << std::endl;
  gplt->cmd << "set key font \"Hack, 10\"" << std::endl;
  for (const auto &rws : raws) {
    std::ostringstream label;
    label << "fs:" << rws->fft_freqs->get_sample_rate() << " wl:" << (int)rws->fft_freqs->get_wl() << " rl:" << (int)rws->fft_freqs->get_rl();
    gplt->set_xy_lines(rws->fft_freqs->get_frequencies(), rws->get_abs_sa_spectra(channel_type), label.str(), 1);
  }

  for (const auto &rws : raws) {
    std::ostringstream label;
    label << "fs prz:" << rws->fft_freqs->get_sample_rate() << " wl:" << (int)rws->fft_freqs->get_wl() << " rl:" << (int)rws->fft_freqs->get_rl();
    gplt->set_xy_lines(rws->fft_freqs->get_selected_frequencies(), rws->get_abs_sa_prz_spectra(channel_type), label.str(), 1);
  }

  gplt->plot();

  return 0;
}
