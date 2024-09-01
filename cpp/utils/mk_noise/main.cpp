// #include "BS_thread_pool.h"
#include "atsheader.h"
#include "atss.h"
#include "freqs.h"
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

int main() {

  size_t i, j;

  // tmp temp dir
  std::filesystem::path filepath(std::filesystem::temp_directory_path() / "aa/noise");
  std::shared_ptr<survey_d> survey;
  if (std::filesystem::exists(filepath)) {
    std::cout << "remove existing directory in order to continue " << filepath.string() << "? y/n ";
    char type('x');
    do {
      std::cout << "remove " << filepath.string() << "? y/n ";
      std::cin >> type;
      std::cout << " " << type << std::endl;
    } while (!std::cin.fail() && (type != 'y') && (type != 'n'));
    if (type == 'y') {
      // recursively delete
      std::uintmax_t n = fs::remove_all(filepath);
      std::cout << "Deleted " << n << " files or directories\n";
    } else {
      std::cout << "no directory created, exit" << std::endl;
      return EXIT_SUCCESS;
    }
  }

  // create survey
  survey = std::make_shared<survey_d>(filepath, false);

  // for new format this survey is empty, use create run with number
  auto last_station = survey->create_station("1");
  // this survey is empty, use create run with number

  // for old ats files
  std::filesystem::create_directories(filepath / "ts/Site_1");
  std::filesystem::path ats_site_path(filepath / "ts/Site_1");

  std::string datetime = "2022-11-30T12:00:00"; // as in JSON file
  auto tt = mstr::time_t_iso_8601_str(datetime);

  std::vector<std::shared_ptr<channel>> channels;
  std::vector<std::shared_ptr<atsheader>> atshs;
  std::vector<std::shared_ptr<ats_header_json>> atsjs; // the json will push into ats
  double max_freq = 5.2428800E+05;
  double min_freq = 9.7656250E-04; // 1024s
  std::vector<double> fsamples;
  std::vector<double> cal_frequencies;

  auto act_freq = max_freq;
  do {
    fsamples.push_back(act_freq);
    act_freq /= 4.0;
  } while (act_freq >= min_freq);

  std::vector<std::string> channel_types{"Ex", "Ey", "Hx", "Hy", "Hz"};
  double sample_freq = 1024;
  size_t run = 1;

  double f_or_s;
  std::string unit;
  // mstr::sample_rate_to_str(chan->get_sample_rate(), f_or_s, unit);

  std::random_device rd{};
  std::mt19937 gen{rd()};
  std::normal_distribution<> dist{5, 2};
  size_t nstacks = 32;
  size_t min_size = nstacks * 1024;
  std::vector<double> noise_data(size_t(max_freq) * nstacks); // at least n stacks
  double sin_freq = sample_freq / 4.;
  double sin_freq2 = sample_freq / 16.;
  double sn = 0;
  // sin(sin_freq * (2 * pi) * i / sample_freq);
  for (auto &nd : noise_data) {
    // nd = dist(gen) + (sin(sin_freq * (2 * M_PI) * sn++ / sample_freq)) / 8.0;
    nd = dist(gen) + ((sin(sin_freq * (2 * M_PI) * sn / sample_freq)) / 8.0) + ((sin(sin_freq2 * (2 * M_PI) * sn / sample_freq)) / 8.0);
    ++sn;
  }

  // ****************** loop for sample frequencies range

  for (size_t k = 0; k < fsamples.size(); ++k) {

    std::vector<double> noise_data_sub;
    size_t act_length = nstacks * fsamples.at(k);
    if (act_length < min_size)
      act_length = min_size;
    noise_data_sub.assign(noise_data.begin(), (noise_data.begin() + act_length));

    std::cout << "try for " << noise_data_sub.size() << " samples" << std::endl;

    for (const auto &channel_type : channel_types) {
      atshs.emplace_back(std::make_shared<atsheader>());
      atsjs.emplace_back(std::make_shared<ats_header_json>(atshs.back()->header, ""));
      atsjs.back()->create_default_header(channel_type);
      atsjs.back()->header["sample_rate"] = fsamples.at(k);
      atsjs.back()->header["start"] = tt;
      if (fsamples.at(k) < 1024)
        atsjs.back()->header["chopper"] = 1;
      else
        atsjs.back()->header["chopper"] = 0;
      channels.emplace_back(std::make_shared<channel>(channel_type, fsamples.at(k)));
    }

    // generate calibration frequencies
    cal_frequencies.clear();
    double act_freq = fsamples.at(k);
    for (i = 0; i < 16; ++i) {
      cal_frequencies.push_back(act_freq);
      act_freq /= 2.0;
    }
    // reverse the vector
    std::reverse(cal_frequencies.begin(), cal_frequencies.end());

    i = 0;
    // create new type of channels with new type of calibration
    for (auto &chan : channels) {
      chan->set_channel_no(i);
      chan->set_system("ADU-08e");
      chan->set_serial(999);
      chan->set_unix_timestamp(tt);

      if (chan->get_channel_type() == "Ex") {

        chan->cal = std::make_shared<calibration>("EFP-06", i + 1, ChopperStatus::off, CalibrationType::mtx);
      }
      if (chan->get_channel_type() == "Ey") {
        chan->angle = 90.0;
        chan->cal = std::make_shared<calibration>("EFP-06", i + 1, ChopperStatus::off, CalibrationType::mtx);
      }
      if (chan->get_channel_type() == "Hx") {
        chan->cal = std::make_shared<calibration>("MFS-06e", i + 1, ChopperStatus::off, CalibrationType::mtx);
        if (fsamples.at(k) < 1024)
          chan->cal->set_chopper(ChopperStatus::on);
        chan->cal->gen_cal_sensor(cal_frequencies);
        chan->cal->set_theo_as_caldata();
      }
      if (chan->get_channel_type() == "Hy") {
        chan->angle = 90.0;
        chan->cal = std::make_shared<calibration>("MFS-06e", i + 1, ChopperStatus::off, CalibrationType::mtx);
        if (fsamples.at(k) < 1024)
          chan->cal->set_chopper(ChopperStatus::on);
        chan->cal->gen_cal_sensor(cal_frequencies);
        chan->cal->set_theo_as_caldata();
      }
      if (chan->get_channel_type() == "Hz") {
        chan->cal = std::make_shared<calibration>("MFS-06e", i + 1, ChopperStatus::off, CalibrationType::mtx);
        if (fsamples.at(k) < 1024)
          chan->cal->set_chopper(ChopperStatus::on);
        chan->cal->gen_cal_sensor(cal_frequencies);
        chan->cal->set_theo_as_caldata();
      }

      ++i;
    }

    // push json to ats header for old data type
    i = 0;
    for (auto &atsj : atsjs) {

      atsj->filename = ats_site_path;
      atsj->set_ats_header();
      atshs[i]->header = atsj->atsh;
      atshs[i]->header.samples = noise_data_sub.size();
      ++i;
    }

    // write ats files
    i = 0;
    for (auto &atsh : atshs) {
      auto outdir(ats_site_path);
      try {
        outdir /= atsjs.at(i)->measdir();
        if (!fs::exists(outdir))
          fs::create_directory(outdir);
        outdir = fs::canonical(outdir);

        atsh->set_new_filename(outdir / atsh->get_ats_filename(run));
        // old data type needs LSB
        atsh->calc_lsb_from_dbl_vec_mV(noise_data_sub);
        std::cout << "starting: writer ats for " << fsamples.at(k) << "Hz" << std::endl;
        atsh->write(false); // write header CHANGE
        // write data
        atsh->ats_write_ints_doubles(atsh->header.lsbval, noise_data_sub, true);
        ++i;
      } catch (const std::runtime_error &error) {
        std::cerr << error.what() << std::endl;
      }
    }

    // for the new file fomat
    i = 0;
    std::filesystem::path last_run_created;
    for (auto &chan : channels) {
      // if (i) last_run_created = survey->add_create_run(last_station, chan);
      // will be added to the same run if same sample rate and different channel number
      survey->add_create_run(last_station, chan);
      chan->write_header(); // we have added a calibration already, no external CAL here
      // chan->write_all_data(noise_data);
      ++i;
    }
    try {
      auto pool = std::make_shared<BS::thread_pool>();
      std::cout << "starting: writer threads for " << fsamples.at(k) << "Hz" << std::endl;
      for (auto &chan : channels) {
        // pool->push_task(&channel::write_all_data, chan, std::ref(noise_data_sub));
        pool->detach_task([&chan, &noise_data_sub]() { chan->write_all_data(noise_data_sub); });
      }
      pool->wait();
    } catch (const std::string &error) {
      std::cerr << error << std::endl;
      return EXIT_FAILURE;
    } catch (std::filesystem::filesystem_error &e) {
      std::cerr << e.what() << std::endl;
    } catch (...) {
      std::cerr << "could not execute all threads" << std::endl;
      return EXIT_FAILURE;
    }

    atshs.clear();
    atsjs.clear();
    channels.clear();

    tt += 1; // add a second for the next job - so have different start times
  }

  return EXIT_SUCCESS;
}
