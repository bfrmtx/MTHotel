#include "atss.h"
#include "freqs.h"
#include <algorithm>
#include <chrono>
#include <complex>
#include <fftw3.h>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <survey.h>
#include <vector>

#include "gnuplotter.h"
#include "raw_spectra.h"
#include <random>

int main(int argc, char *argv[]) {

  bool files = false; // change to true for file output

  unsigned l = 1;
  while (argc > 1 && (l < unsigned(argc)) && *argv[l] == '-') {
    std::string marg(argv[l]);

    if (marg.compare("-f") == 0) {
      files = true;
    }
    if (marg.compare("-") == 0) {
      std::cerr << "\nunrecognized option " << argv[l] << std::endl;
      return EXIT_FAILURE;
    }
    ++l;
  }

  size_t file_count = 0;
  std::string basename("spc_ts_basic");

  auto pool = std::make_shared<BS::thread_pool>();

  std::filesystem::path outpath(std::filesystem::temp_directory_path() / "fft_basics");

  if (files) {
    try {
      std::filesystem::create_directory(outpath);
    } catch (...) {
      std::cerr << "could not create " << outpath.string() << std::endl;
      return EXIT_FAILURE;
    }
  }

  for (size_t loop = 0; loop < 3; ++loop) { // for three examples with / wo close frequencies

    size_t i;
    std::vector<std::shared_ptr<channel>> channels;
    std::vector<std::shared_ptr<fftw_freqs>> fft_freqs;
    std::vector<std::shared_ptr<raw_spectra>> raws;
    std::string channel_type("Ex");
    size_t rl(1024);
    size_t wl(rl);
    size_t max_fact = 4; // for longer FFT and for the zero padding

    if (loop == 2)
      max_fact = 16;

    double sample_freq(wl);
    double f_or_s;
    std::string unit;

    // 1 Hz bw
    channels.emplace_back(std::make_shared<channel>(channel_type, sample_freq));
    fft_freqs.emplace_back(std::make_shared<fftw_freqs>(channels.back()->get_sample_rate(), wl, rl));

    // 0.25 Hz bw
    channels.emplace_back(std::make_shared<channel>(channel_type, sample_freq));
    fft_freqs.emplace_back(std::make_shared<fftw_freqs>(channels.back()->get_sample_rate(), max_fact * wl, max_fact * rl));

    // 0.25 Hz bw  zero padding
    channels.emplace_back(std::make_shared<channel>(channel_type, sample_freq));
    fft_freqs.emplace_back(std::make_shared<fftw_freqs>(channels.back()->get_sample_rate(), max_fact * wl, rl));

    auto fft_res_iter = fft_freqs.begin(); // example usage  fft with auto & channels
    try {
      // create the fftw interface with individual window length and read length
      for (auto &chan : channels) {
        auto fft_fres = *fft_res_iter++;
        chan->init_fftw(fft_fres);

        // here each channel is treated as single result - by default it would contain 5 channels
        raws.emplace_back(std::make_shared<raw_spectra>(pool, fft_fres));
        mstr::sample_rate_to_str(chan->get_sample_rate(), f_or_s, unit);
        std::cout << "use sample rates of " << f_or_s << " " << unit << " wl:" << fft_fres->get_wl() << "  read length:" << fft_fres->get_rl() << std::endl;
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

    std::random_device rd{};
    std::mt19937 gen{rd()};
    std::normal_distribution<> dist{0, 2};
    std::vector<double> noise_data(wl * max_fact * 12); // stacks "for fun"
    double sin_freq = 50;
    double sin_freq2 = 50.5;

    double sn = 0;

    if (loop == 0) {
      for (auto &nd : noise_data) {
        nd = dist(gen) / 5. + (sin(sin_freq * (2 * M_PI) * sn / sample_freq));
        sn++;
      }
    }

    else {
      for (auto &nd : noise_data) {
        nd = dist(gen) / 5. + (sin(sin_freq * (2 * M_PI) * sn / sample_freq)) + (sin(sin_freq2 * (2 * M_PI) * sn / sample_freq));
        sn++;
      }
    }

    // **** here I do the FFT
    for (auto &chan : channels) {
      // const bool bdetrend_hanning = true
      chan->read_all_fftw_gussian_noise(noise_data, true);
      std::cout << chan->qspc.size() << " readings" << std::endl;
    }

    std::string init_err;
    auto gplt_ts = std::make_unique<gnuplotter<double, double>>(init_err);

    if (init_err.size()) {
      std::cout << init_err << std::endl;
      return EXIT_FAILURE;
    }

    size_t ts_beg = 0, ts_end(sin_freq * 4);
    std::vector<double> xax(ts_end - ts_beg);
    std::vector<double> yax(ts_end - ts_beg);
    i = 0;
    for (auto &v : yax) {
      xax[i] = ts_beg + i;
      v = noise_data[i++];
    }

    if (!files)
      gplt_ts->set_qt_terminal(basename, ++file_count);
    else
      gplt_ts->set_svg_terminal(outpath, basename, "", 1, ++file_count);

    if (loop)
      gplt_ts->cmd << "set title 'TS Data \\@ 50 + 50.5 Hz + small noise'" << std::endl;
    else
      gplt_ts->cmd << "set title 'TS Data \\@ 50 Hz'" << std::endl;
    gplt_ts->cmd << "set key off" << std::endl;
    gplt_ts->cmd << "set xlabel 'ts [ms]'" << std::endl;
    gplt_ts->cmd << "set ylabel 'amplitude [mV]'" << std::endl;
    gplt_ts->cmd << "set grid" << std::endl;
    gplt_ts->cmd << "set key font \"Hack, 10\"" << std::endl;

    gplt_ts->set_x_range(ts_beg, ts_end);
    gplt_ts->set_xy_lines(xax, yax, "input", 1);
    gplt_ts->plot();
    gplt_ts.reset();

    std::vector<double> max_mins;     // adjust x-axis of gnuplot; otherwise scales in log mode to full decade
    fft_res_iter = fft_freqs.begin(); // auto iterator example
    for (auto &chan : channels) {
      auto fft_fres = *fft_res_iter++;
      auto mm = fft_fres->auto_range(0.01, 0.5); // cut off spectra
      max_mins.push_back(mm.first);
      max_mins.push_back(mm.second);
      // const bool bcal = true, const bool bwincal = true is amplitude spectral density
      chan->prepare_to_raw_spc(fft_fres, true, false);
    }
    std::sort(max_mins.begin(), max_mins.end());

    i = 0;
    for (auto &chan : channels) {
      raws[i++]->set_raw_spectra(chan);
    }

    for (auto &raw : raws) {
      raw->simple_stack_all();
    }

    //
    auto gplt = std::make_unique<gnuplotter<double, double>>(init_err);

    if (init_err.size()) {
      std::cout << init_err << std::endl;
      return EXIT_FAILURE;
    }

    // make a copy for data tests and so on **************************************

    std::vector<std::vector<double>> vs(channels.size());
    for (size_t i = 0; i < raws.size(); ++i) {
      vs[i] = raws.at(i)->get_abs_sa_spectra(channel_type);
    }
    // do some changes *****************************************
    // unscaled
    i = 0;
    for (auto &v : vs) {
      // for (auto &d : v) d /= (0.5*fft_freqs.at(i)->get_rl());
      for (auto &d : v)
        d *= fft_freqs.at(i)->get_fftw_scale();
      ++i;
    }

    if (!files)
      gplt->set_qt_terminal(basename, ++file_count);
    else
      gplt->set_svg_terminal(outpath, basename, "", 1, ++file_count);

    if (loop)
      gplt->cmd << "set title 'FFT \\@ 50 + 50.5 Hz'" << std::endl;
    else
      gplt->cmd << "set title 'FFT 50 Hz'" << std::endl;
    gplt->cmd << "set xlabel 'frequency [Hz]'" << std::endl;
    gplt->cmd << "set ylabel 'amplitude [mV/Hz]'" << std::endl;
    gplt->cmd << "set grid" << std::endl;
    gplt->cmd << "set key font \"Hack, 10\"" << std::endl;
    gplt->set_x_range(45., 55.);
    // gplt->set_y_range(0.01, 2.);

    auto fieldw = field_width(fft_freqs);
    // gplt->cmd << "set logscale y" << std::endl;

    for (size_t i = 0; i < channels.size(); ++i) {

      // formatted output
      std::ostringstream label;
      mstr::sample_rate_to_str(channels.at(i)->get_sample_rate(), f_or_s, unit, true);
      label << "fs " << std::setw(fieldw) << f_or_s << " " << unit << " wl: " << std::setw(fieldw) << fft_freqs.at(i)->get_wl() << " rl: " << std::setw(fieldw) << fft_freqs.at(i)->get_rl() << " Δf: " << std::setw(6) << fft_freqs.at(i)->get_delta_f();

      std::ostringstream pt;
      pt << "pt " << i + 6;
      gplt->set_xy_linespoints(fft_freqs.at(i)->get_frequencies(), vs[i], label.str(), 2, 2, pt.str());
    }

    gplt->plot();
    gplt.reset();
  }

  if (files) {
    std::cout << std::endl;
    std::cout << "files have been written to " << outpath.string() << std::endl;
    std::cout << std::endl;
  }

  return EXIT_SUCCESS;
}
