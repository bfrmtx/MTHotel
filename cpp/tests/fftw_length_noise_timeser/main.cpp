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

#include "atsheader.h"
#include "atsheader_def.h"
#include "atss_to_ats.h"
#include "gnuplotter.h"
#include "raw_spectra.h"
#include <random>

// #include "BS_thread_pool.h"

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

  auto start_time = std::chrono::high_resolution_clock::now();

  // make some individual data for each channel and treat all separately
  // one idea: three different frequencies for one sample frequency

  size_t i, j;
  size_t file_count = 0;

  std::filesystem::path outdir(std::filesystem::temp_directory_path() / "fft_noise_timeser");
  //  try conversion to ats  /
  auto ats_outdir = fs::temp_directory_path() / "converted";

  auto survey = std::make_shared<survey_d>(outdir, false); // anyway create in case
  // std::vector<fs::path> last_stations;

  std::string basename("spc_ts_sine"); // for the plots

  std::vector<std::string> channel_types{"Ex", "Ey", "Hx", "Hy", "Hz"};
  double sample_rate = 1024;
  size_t rl = 1024, wl = 1024;
  size_t dat_sz = 1024 * 640; // * stacks

  size_t num_stations = 4; // I want four stations with different settings, each with all channels, first pure noise, then sine + noise
  std::vector<std::pair<std::string, size_t>> station_names_runs;
  for (i = 0; i < num_stations; ++i) {
    auto station_path = survey->add_station_auto_num("site_");                   // this creates a new station and returns a path
    station_names_runs.emplace_back(station_path.filename().string(), SIZE_MAX); // in our case that will be site_3, site_4, site_5
    // the survey has added the station as shared_ptr<station_d> to its vector; there may be previously created stations
    std::shared_ptr<station_d> station = survey->get_station(station_path);   // that is the freshly created station
    auto run_path = station->add_channel_set(channel_types, sample_rate);     // add all channels to the station -> freshly created run with free number, auto default values for the channels
    std::shared_ptr<run_d> run = station->get_run(run_path);                  // get the run, freshly created, including fresh channels
    station_names_runs.back().second = run->get_run_no();                     // get the run number
    std::vector<std::shared_ptr<channel>> run_channels = run->get_channels(); // get the channels of the run
    for (auto &chan : run_channels) {
      chan->ts_slice.resize(dat_sz); // time series data complete, even though only a slice is used, a slight mis-use of the data structure
    }
  }

  std::vector<double> base_noise_data(dat_sz); // the base noise data for all channels
  auto pool = std::make_shared<BS::thread_pool>();

  // I add 1% noise floor
  std::random_device rd{};
  std::mt19937 gen{rd()};
  std::normal_distribution<> dist{0, 2}; // mean 0, sigma 2

  // create the base noise data without the sine ***************************************************************************************************
  for (auto &v : base_noise_data)
    v = dist(gen) / 100.;

  // add the 3 different sine frequencies to the data
  std::vector<double> sin_freqs(num_stations);
  sin_freqs[0] = 0; // no sine for the first station
  sin_freqs[1] = 256;
  sin_freqs[2] = 180;
  sin_freqs[3] = 64;
  double sn;
  i = 0;
  // same noise all channels
  for (auto &snr : station_names_runs) {
    std::cout << snr.first << " " << snr.second << std::endl;
    auto run = survey->get_run(snr);     // run_d contains the channels
    auto channels = run->get_channels(); // get the channels of the run
    for (auto &chan : channels) {
      chan->ts_slice = base_noise_data;
    }
    // for each station a different sine
    sn = 0;

    for (auto &chan : channels) {
      if (i) {                                                           // no sine for the first station
        for (auto &v : chan->ts_slice) {                                 // same sine for all channels of same station
          v += (sin(sin_freqs.at(i) * (2 * M_PI) * sn++ / sample_rate)); // add the sine to the noise
        }
      }
      chan->write_header(); // write the header
      chan->write_slice();  // write the slice, survey is not written
    }
    ++i; // next sine for next station
  }

  const size_t ts_beg = 0, ts_end(sin_freqs.at(1) / 4); // 1/4 of the sine period - a sub range for better visibility
  std::vector<double> xax(ts_end - ts_beg);             // x-axis for the time series plots
  i = 0;
  for (auto &v : xax)
    v = ts_beg + i++;

  std::vector<std::vector<double>> yaxs; // y-axis for the time series plots, number of sites, only Ex
  for (auto &snr : station_names_runs) {
    std::cout << snr.first << " " << snr.second << std::endl;
    auto run = survey->get_run(snr);    // run_d contains the channels
    auto chan = run->get_channel("Ex"); // get the channel Ex of the run
    auto vec = std::vector<double>(ts_end - ts_beg);
    for (i = ts_beg; i < ts_end; ++i) {
      vec[i] = chan->ts_slice[i];
    }
    yaxs.emplace_back(vec);
    chan->ts_slice.clear(); // free the memory
  }

  // ...
  auto convert_ats = std::make_shared<atss_to_ats>(survey, ats_outdir);
  for (const auto &snr : station_names_runs) {
    convert_ats->convert_run(snr.first, snr.second);
    // create measdoc.xml ....
  }

  std::string init_err;
  auto gplt_ts = std::make_unique<gnuplotter<double, double>>(init_err);
  if (init_err.size()) {
    std::cout << init_err << std::endl;
    return EXIT_FAILURE;
  }

  if (!files)
    gplt_ts->set_qt_terminal(basename, ++file_count);
  else
    gplt_ts->set_svg_terminal(outdir, basename, "", 1, ++file_count);

  gplt_ts->cmd << "set multiplot layout 3, 1" << std::endl;
  i = 0;
  for (const auto &yax : yaxs) {
    if (i) { // skip noise only
      gplt_ts->cmd << "set title 'TS Data, sine \\@ " << sin_freqs[i] << " Hz, f_{sample} " << sample_rate << " Hz'" << std::endl;
      gplt_ts->cmd << "set key off" << std::endl;
      gplt_ts->cmd << "set xlabel 'ts [ms]'" << std::endl;
      gplt_ts->cmd << "set ylabel 'amplitude [mV]'" << std::endl;
      gplt_ts->cmd << "set grid" << std::endl;
      gplt_ts->set_x_range(ts_beg, ts_end);
      gplt_ts->set_y_range(-1.2, 1.2);
      gplt_ts->set_xy_linespoints(xax, yax, "input", 1, 2, "lt rgb \"blue\" pt 6");
      gplt_ts->plot();
    }
    ++i;
  }

  gplt_ts.reset();

  // ********************************+ S P E C T R A *********************************************************************

  // we take the last noise data for the spectra, that was 64 Hz
  // we play around with a single channel, Ex, and duplicate the data for the other channels

  std::vector<std::shared_ptr<run_d>> runs; // the run contains the raw spectra, the channel the fft
  // now add a deep copy to the vector
  runs.push_back(std::make_shared<run_d>(survey->get_run(station_names_runs.front()))); // deep copy creator, not a = b with shared_ptr
  runs.push_back(std::make_shared<run_d>(survey->get_run(station_names_runs.back())));  // deep copy creator
  runs.push_back(std::make_shared<run_d>(survey->get_run(station_names_runs.back())));  // deep copy creator
  runs.push_back(std::make_shared<run_d>(survey->get_run(station_names_runs.back())));  // deep copy creator
  std::vector<std::string> labels = {"noise", "1Hz", "padded", "0.5Hz"};
  i = 0;
  for (auto &run : runs) {
    auto channels = run->get_channels(); // get the channels of the run
    for (auto &chan : channels) {
      chan->init_fftw(nullptr, false, wl, rl); // normal, zero padding, double read length and window length
    }
    if (i == 1)
      wl *= 2;
    if (i == 2)
      rl *= 2;
    run->init_raw_spectra(pool);
    for (auto &chan : channels) {
      // make sure to supply all arguments!; this loop has big workload
      // pool->push_task(&channel::read_all_fftw, chan, false, nullptr); // normal, zero padding, double read length and window length, last chunk = false
      pool->detach_task([&chan]() { chan->read_all_fftw(false, nullptr); });
    }
    ++i;
  }
  pool->wait();

  // now fetch the the raw spectra
  for (auto &run : runs) {
    run->fetch_raw_spectra();
    auto chan = run->ch_first();
    std::cout << chan->qspc.size() << " readings" << std::endl;
  }
  pool->wait();
  // now we have the raw spectra, we can prepare the auto- and cross- spectra
  for (auto &run : runs) {
    std::cout << "setting auto- and cross- spectra ";
    auto channels = run->get_channels();
    for (const auto &chan : channels) {
      std::cout << "preparing " << chan->channel_type << " " << chan->channel_type << std::endl;
      run->raw_spc->sa.add_spectra(chan->channel_type, chan->channel_type);     // add the spectra to the raw_spectra object
      run->raw_spc->sa_prz.add_spectra(chan->channel_type, chan->channel_type); // we may need the parzen spectra later; cost is low
      // pool->push_task(&channel::prepare_raw_spc, chan, true, true);             // make vector from queue, 0, 1 no wincal
      pool->detach_task([&chan]() { chan->prepare_raw_spc(true, true); }); // make vector from queue, 0, 1 no wincal
    }
    std::cout << std::endl;
  }
  pool->wait();
  for (auto &run : runs) {
    run->fetch_raw_spectra(); // fetch the raw spectra from the thread pool; move operation (fast); also initializes the ac_spectra!
    // raw spectra also contains the channel pointer and therewith the channel name and FFT properties
  }

  for (auto &run : runs) {
    // run raw spectra fires up all channels; USE wait_for_tasks() after this
    run->raw_spc->advanced_stack_all(0.5); // stack all auto and cross spectra
  }
  pool->wait(); // wait for all tasks to finish

  // ******************************** F I N I S H E D  R E A D I N G  D A T A *********************************************************************
  // ******************************** F I N I S H E D  S I N G L E  S P E C T R A *********************************************************************
  // ******************************** F I N I S H E D  S T A C K I N G  S P E C T R A *********************************************************************

  auto gplt = std::make_shared<gnuplotter<double, double>>(init_err);

  if (init_err.size()) {
    std::cout << init_err << std::endl;
    return EXIT_FAILURE;
  }

  auto nc = std::make_unique<next_color>("blue");
  auto ncr = std::make_unique<next_color>("red");
  auto nco = std::make_unique<next_color>("orange");
  auto ncg = std::make_unique<next_color>("green");

  std::string all_coils_title("FFT of noise and sine data, all coils");

  gplt->cmd << "set terminal qt size 2048,768 enhanced" << std::endl;
  gplt->cmd << "set title '" << all_coils_title << "'" << std::endl;
  // gplt->cmd << "set key off" << std::endl;

  gplt->cmd << "set xlabel 'frequency [Hz]'" << std::endl;
  gplt->cmd << "set ylabel 'amplitude'" << std::endl;
  gplt->cmd << "set grid" << std::endl;
  // gplt->cmd << "set format y '1E^%+03T'" << std::endl;
  gplt->cmd << "set logscale xy" << std::endl;
  gplt->cmd << "set key font \"Hack, 10\"" << std::endl;
  gplt->set_x_range(0.01 * sample_rate, 0.3 * sample_rate);

  i = 0;
  for (auto &run : runs) {
    if (i == 1) {
      auto ac = std::make_pair("Ex", "Ex");
      auto ac_color = std::make_pair("Hx", "Hx");
      auto color = nc->color_line();
      // std::cout << ">" << color << "< >" << gplt->default_color(ac_color) << "<" << std::endl;
      // if (run->raw_spc->fft_freqs->get_rl() == 2048)
      //   gplt->set_xy_lines(run->raw_spc->fft_freqs->get_frequencies(), run->raw_spc->get_abs_sa_spectra(ac), "Ex Ex", 2, ncr->color_line());
      // else
      gplt->set_xy_lines(run->raw_spc->fft_freqs->get_frequencies(), run->raw_spc->get_abs_sa_spectra(ac), labels.at(i), 1);
      // get the maximum of the spectra
      auto vec = run->raw_spc->get_abs_sa_spectra(ac);
      auto max = *std::max_element(vec.begin(), vec.end());
      std::cout << "max " << max << std::endl;
    }
    ++i;
  }
  gplt->plot();

  return EXIT_SUCCESS;

  /*
  noise_data.clear();
  noise_data.emplace_back(base_noise_data); // normal
  noise_data.emplace_back(base_noise_data); // for zero padding

  channels.clear();
  channels.emplace_back(std::make_shared<channel>(channel_type, sample_rate)); // normal
  channels.emplace_back(std::make_shared<channel>(channel_type, sample_rate)); // zero padding

  std::vector<std::shared_ptr<gnuplotter<double, double>>> vgplt;

  // two different read- and window length   ***************************************************************************************************
  for (size_t iwl = 0; iwl < 2; ++iwl) {

    if (iwl) {
      wl *= 4;
      rl *= 4;
    }
    // auto gplt = std::make_unique<gnuplotter<double, double>>(init_err);

    // T H R E E  PLOTS!                   ***************************************************************************************************
    for (size_t pl = 0; pl < 3; ++pl) {
      i = 0;
      std::vector<std::shared_ptr<fftw_freqs>> fft_freqs;
      std::vector<std::shared_ptr<raw_spectra>> raws;
      for (auto &chan : channels) {
        // last zero padding
        if (i == channels.size() - 1)
          fft_freqs.emplace_back(std::make_shared<fftw_freqs>(chan->get_sample_rate(), wl * 2, rl));
        else
          fft_freqs.emplace_back(std::make_shared<fftw_freqs>(chan->get_sample_rate(), wl, rl));
        chan->init_fftw(fft_freqs.back());
        raws.emplace_back(std::make_shared<raw_spectra>(pool, fft_freqs.back()));
        ++i;
      }

      // **** here I do the FFT of a single vector - e.g. a noise vector
      i = 0;
      for (auto &chan : channels) {
        // const bool bdetrend_hanning = true
        if (pl == 0)
          pool->push_task(&channel::read_all_fftw_gaussian_noise, chan, noise_data[i++], false); // 0 rect window
        else
          pool->push_task(&channel::read_all_fftw_gaussian_noise, chan, noise_data[i++], true); // else hanning
      }
      pool->wait();

      for (auto &chan : channels) {
        std::cout << chan->qspc.size() << " readings" << std::endl;
      }

      std::vector<double> max_mins;          // adjust x-axis of gnuplot; otherwise scales in log mode to full decade
      auto fft_res_iter = fft_freqs.begin(); // auto iterator example
      i = 0;
      for (auto &chan : channels) {
        auto fft_fres = *fft_res_iter++;
        auto mm = fft_fres->auto_range(0.01, 0.4); // cut off spectra low 1% high up to 40%
        max_mins.push_back(mm.first);
        max_mins.push_back(mm.second);
        // const bool bcal = true, const bool bwincal = true is amplitude spectral density
        if (pl < 2)
          chan->prepare_to_raw_spc(fft_fres, true, false); // make vector from queue, 0, 1 no wincal
        else
          chan->prepare_to_raw_spc(fft_fres, true, true);                                // make vector from queue, 2 wincal
        raws[i]->sa.add_spectra(std::make_pair(chan->channel_type, chan->channel_type)); // add the spectra to the raw_spectra object
        raws[i++]->move_raw_spectra(chan);                                               // swap!
      }
      std::sort(max_mins.begin(), max_mins.end()); // of x-axis frequencies

      for (auto &raw : raws) {
        raw->advanced_stack_all();
      }
      pool->wait();

      auto ampl_min_max = min_max_sa_spc(raws, std::make_pair(channel_type, channel_type));
      std::cout << ampl_min_max.first << " " << ampl_min_max.second << std::endl;

      //
      double f_or_s;
      std::string unit;
      init_err.clear();
      vgplt.emplace_back(std::make_shared<gnuplotter<double, double>>(init_err));

      if (init_err.size()) {
        std::cout << init_err << std::endl;
        return EXIT_FAILURE;
      }
      auto gplt = vgplt.back();

      // make a copy for data tests and so on **************************************

      std::vector<std::vector<double>> vs(channels.size());
      for (size_t i = 0; i < raws.size(); ++i) {
        vs[i] = raws.at(i)->get_abs_sa_spectra(std::make_pair(channel_type, channel_type));
      }
      i = 0;
      if (pl < 2) {
        // do some changes *****************************************
        for (auto &v : vs) {
          // for (auto &d : v) d /= (0.5 * fft_freqs.at(i)->get_rl());
          for (auto &d : v)
            d *= fft_freqs.at(i)->get_fftw_scale(); // 0, 1  get wl/2 scale only; 2 has already ampl scale
          ++i;
        }
      }

      if (!files)
        gplt->set_qt_terminal(basename, ++file_count);
      else
        gplt->set_svg_terminal(outdir, basename, "", 1, ++file_count);

      // here the full explanation of 0, 1, 2
      if (pl == 0)
        gplt->cmd << "set title 'FFT length, rectangular Window, div T/2'" << std::endl;
      if (pl == 1)
        gplt->cmd << "set title 'FFT length, Hanning Window, div T/2'" << std::endl;
      if (pl == 2)
        gplt->cmd << "set title 'FFT length, Hanning Window, div √(f_{sample} * T/2)'" << std::endl;

      gplt->cmd << "set xlabel 'frequency [Hz]'" << std::endl;
      gplt->cmd << "set grid" << std::endl;
      if (pl < 2)
        gplt->cmd << "set ylabel 'amplitude [mV]'" << std::endl;
      else
        gplt->cmd << "set ylabel 'amplitude [mV/√Hz]'" << std::endl;

      gplt->cmd << "set key font \"Hack, 14\"" << std::endl;
      gplt->cmd << "set logscale xy" << std::endl;
      gplt->set_x_range(92, 112);
      // gplt->set_y_range(0.0008, 1.5);
      gplt->set_y_range(0.0001, 1.5);
      auto fieldw = field_width(fft_freqs);
      for (size_t i = 0; i < channels.size(); ++i) {

        // formatted output
        std::ostringstream label;
        auto delta_f = (channels.at(i)->get_sample_rate() / ((double)fft_freqs.at(i)->get_rl()));
        mstr::sample_rate_to_str(channels.at(i)->get_sample_rate(), f_or_s, unit, true);
        label << "fs " << std::setw(fieldw) << f_or_s << " " << unit << " wl: " << std::setw(fieldw) << fft_freqs.at(i)->get_wl() << " rl: " << std::setw(fieldw) << fft_freqs.at(i)->get_rl() << " Δf: " << std::setw(3) << delta_f;
        //
        //                if (i == channels.size()-1) gplt->set_xy_linespoints(fft_freqs.at(i)->get_frequencies(), vs[i], label.str(), 2, 2, " lc rgbcolor \"grey\" dashtype 2 pt 12");
        //                else gplt->set_xy_linespoints(fft_freqs.at(i)->get_frequencies(), vs[i], label.str(), 1, 2, "pt 6");

        if (i == channels.size() - 1)
          gplt->set_xy_linespoints(fft_freqs.at(i)->get_frequencies(), vs[i], label.str(), 2, 2, " lc rgbcolor \"blue\" pt 6");
        else
          gplt->set_xy_linespoints(fft_freqs.at(i)->get_frequencies(), vs[i], label.str(), 2, 4, "pt 6");
      }

      pool->push_task(&gnuplotter<double, double>::plot, gplt);
    }
  }
  pool->wait();

  auto stop_time = std::chrono::high_resolution_clock::now();

  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop_time - start_time);

  std::cout << "Time taken by main: " << duration.count() << " ms" << std::endl;

  if (files) {
    std::cout << std::endl;
    std::cout << "files have been written to " << outdir.string() << std::endl;
    std::cout << std::endl;
  }

  return EXIT_SUCCESS;
  */
}
